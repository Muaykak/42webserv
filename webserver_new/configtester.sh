#!/usr/bin/env bash
#!/bin/bash

# ==============================================================================
# CONFIGURATION
# ==============================================================================

# Path to your program executable
PROGRAM_PATH="./Webserv"

# Directory containing all the .conf files
TEST_DIR="./testFiles/test_configs"

# How long to wait (in seconds) for the program to parse the config.
# If the program is still running after this time, we assume it parsed successfully.
TIMEOUT_SECONDS=0.5

# The signal to send to the program to terminate it gracefully.
# Common signals: SIGTERM (15), SIGINT (2)
TERMINATION_SIGNAL="SIGINT"

# Naming convention: If a config filename starts with this prefix, 
# the test runner expects the program to FAIL parsing and exit immediately.
# Otherwise, it expects the program to PASS.
EXPECT_FAIL_PREFIX="fail_"

# ==============================================================================
# INTERNAL SETUP (Do not edit below unless you know what you are doing)
# ==============================================================================

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED_COUNT=0
FAILED_COUNT=0
TOTAL_COUNT=0

# Check if program exists and is executable
if [ ! -x "$PROGRAM_PATH" ]; then
    echo -e "${RED}Error: Program '$PROGRAM_PATH' not found or is not executable.${NC}"
    exit 1
fi

# Check if test directory exists
if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}Error: Test directory '$TEST_DIR' not found.${NC}"
    exit 1
fi

echo -e "${YELLOW}Starting Configuration Tests...${NC}"
echo "---------------------------------------------------"

# Suppress background job termination messages in bash
set +m 

# Loop through all .conf files in the test directory
for conf_file in "$TEST_DIR"/*.conf; do
    # Skip if no .conf files are found (glob fails)
    [ -e "$conf_file" ] || continue

    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    filename=$(basename "$conf_file")
    
    # Determine expected result based on filename
    EXPECT_FAIL=false
    if [[ "$filename" == "$EXPECT_FAIL_PREFIX"* ]]; then
        EXPECT_FAIL=true
        expected_status="Expected to FAIL"
    else
        expected_status="Expected to PASS"
    fi

    # Run the program in the background
    # Redirecting output to /dev/null so it doesn't clutter our test results.
    # Change > /dev/null 2>&1 to a log file if you want to inspect output later.
    "$PROGRAM_PATH" "$conf_file" > /dev/null 2>&1 &
    PID=$!

    # Wait to give the program time to process the config
    sleep "$TIMEOUT_SECONDS"

    # Check if the process is still running
    if kill -0 "$PID" 2>/dev/null; then
        # Process is alive! It parsed the config and entered its main loop.
        PROGRAM_STAYED_ALIVE=true
        
        # Send termination signal to shut it down
        kill -s "$TERMINATION_SIGNAL" "$PID"
        
        # Wait for it to finish shutting down and grab the exit code
        wait "$PID" 2>/dev/null
        EXIT_CODE=$?
    else
        # Process died before the timeout. Config parsing failed.
        PROGRAM_STAYED_ALIVE=false
        
        # Wait just to harvest the exit code safely
        wait "$PID" 2>/dev/null
        EXIT_CODE=$?
    fi

    # Evaluate the result
    TEST_PASSED=false

    if [ "$EXPECT_FAIL" = true ]; then
        # We expected it to fail.
        if [ "$PROGRAM_STAYED_ALIVE" = false ] && [ "$EXIT_CODE" -ne 0 ]; then
            TEST_PASSED=true
            reason="Exited immediately with code $EXIT_CODE"
        else
            reason="Program stayed alive or returned 0 unexpectedly"
        fi
    else
        # We expected it to pass.
        if [ "$PROGRAM_STAYED_ALIVE" = true ] && [ "$EXIT_CODE" -eq 0 ]; then
            TEST_PASSED=true
            reason="Stayed alive, terminated gracefully with code 0"
        else
            reason="Died early or returned code $EXIT_CODE"
        fi
    fi

    # Print result
    if [ "$TEST_PASSED" = true ]; then
        echo -e "[ ${GREEN}PASS${NC} ] $filename ($expected_status) - $reason"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "[ ${RED}FAIL${NC} ] $filename ($expected_status) - $reason"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi

done

echo "---------------------------------------------------"
echo -e "Tests Completed: $TOTAL_COUNT total."
echo -e "Passed: ${GREEN}$PASSED_COUNT${NC}"
echo -e "Failed: ${RED}$FAILED_COUNT${NC}"

if [ "$FAILED_COUNT" -gt 0 ]; then
    exit 1
else
    exit 0
fi