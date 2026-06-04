#!/bin/bash

# ==============================================================================
# Basic Configuration
# ==============================================================================
WEBSERV_BIN="./Webserv"
DEFAULT_CONF="./testFiles/test_webserv.conf"
TEST_SUITE_FILE="./testFiles/test_suite.csv"
REQUESTS_DIR="./testFiles/test_requests"
TEMP_RES_FILE="/tmp/webserv_res.txt"
TIMEOUT_SEC=0.3
DEBUG_MODE=0  # Set to 1 to view Raw Response on errors

# ==============================================================================
# Colors for output
# ==============================================================================
RESET="\033[0m"
BOLD="\033[1m"
GREEN="\033[32m"/home/paradari/Desktop/github/42webserv/webserver_new/wwwroot

RED="\033[31m"
YELLOW="\033[33m"
CYAN="\033[36m"
GRAY="\033[90m"

TOTAL=0
PASSED=0
FAILED=0
WEBSERV_PID=0

cleanup() {
    echo -e "\n${YELLOW}[!] Shutting down Webserv (PID: $WEBSERV_PID)...${RESET}"
    if kill -0 $WEBSERV_PID 2>/dev/null; then
        kill -9 $WEBSERV_PID
        wait $WEBSERV_PID 2>/dev/null
    fi
    rm -f $TEMP_RES_FILE
    echo -e "${GREEN}[✓] Shutdown complete${RESET}"
}
trap cleanup EXIT INT TERM

check_prerequisites() {
    if [ ! -f "$WEBSERV_BIN" ]; then echo -e "${RED}[ERROR] Not found: $WEBSERV_BIN${RESET}"; exit 1; fi
    if [ ! -f "$DEFAULT_CONF" ]; then echo -e "${RED}[ERROR] Not found: $DEFAULT_CONF${RESET}"; exit 1; fi
    if [ ! -f "$TEST_SUITE_FILE" ]; then echo -e "${RED}[ERROR] Not found: $TEST_SUITE_FILE${RESET}"; exit 1; fi
}

start_webserv() {
    echo -e "${CYAN}====================================================${RESET}"
    echo -e "${BOLD}${CYAN}🚀 Starting Webserv Tester 42 (Debug Mode)${RESET}"
    echo -e "${CYAN}====================================================${RESET}"
    
    # Run webserv in the background
    $WEBSERV_BIN $DEFAULT_CONF > /tmp/webserv_output.log 2>&1 &
    WEBSERV_PID=$!
    
    echo -e "${GRAY}>> Webserv is running with PID: $WEBSERV_PID${RESET}"
    sleep 1
}

run_test() {
    local test_name="$1"
    local req_file="$2"
    local expect_code="$3"
    local host="$4"
    local port="$5"

    TOTAL=$((TOTAL + 1))
    local full_req_path="${REQUESTS_DIR}/${req_file}"
    
    if [ ! -f "$full_req_path" ]; then
        echo -e "  ${RED}[ERROR] Request file not found: $full_req_path${RESET}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Clear old response file
    > "$TEMP_RES_FILE"

    # Use Bash Built-in /dev/tcp instead of nc to avoid version issues
    # Open TCP Socket (File Descriptor 3)
    exec 3<>/dev/tcp/$host/$port 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "  ${RED}[KO] ${test_name} -> CONNECTION REFUSED (Cannot connect to server)${RESET}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Send Raw Request from file directly to Socket
    cat "$full_req_path" >&3 2>/dev/null

    # Read Response with timeout (from FD 3)
    timeout $TIMEOUT_SEC cat <&3 > "$TEMP_RES_FILE" 2>/dev/null
    local exit_status=$?

    # Close Socket
    exec 3<&- 2>/dev/null

    # Check if timed out and no data received, meaning server hung (if data exists, it's due to Keep-Alive)
    if [ $exit_status -eq 124 ] && [ ! -s "$TEMP_RES_FILE" ]; then
        echo -e "  ${RED}[KO] ${test_name} -> TIMEOUT (Server did not respond in ${TIMEOUT_SEC}s)${RESET}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Read the first line to check, e.g., HTTP/1.1 200 OK
    local first_line=$(head -n 1 "$TEMP_RES_FILE" | tr -d '\r')
    local actual_code=$(echo "$first_line" | awk '{print $2}')

    if [ "$actual_code" == "$expect_code" ]; then
        echo -e "  ${GREEN}[OK] ${test_name} (Got $actual_code as expected)${RESET}"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}[KO] ${test_name} -> Expected: $expect_code but got: ${actual_code:-EMPTY}${RESET}"
        
        # If no data returned, check if Webserv died
        if ! kill -0 $WEBSERV_PID 2>/dev/null; then
            echo -e "      ${YELLOW}-> [!] Warning: Webserv (PID: $WEBSERV_PID) crashed or stopped!${RESET}"
        fi

        # If Debug mode is enabled, show Raw Response
        if [ $DEBUG_MODE -eq 1 ]; then
            echo -e "      ${GRAY}--- Raw Response Start ---${RESET}"
            if [ -s "$TEMP_RES_FILE" ]; then
                head -n 5 "$TEMP_RES_FILE" | while read -r line; do echo -e "      ${GRAY}$line${RESET}"; done
            else
                echo -e "      ${GRAY}(No response data from server)${RESET}"
            fi
            echo -e "      ${GRAY}--------------------------${RESET}"
        fi
        FAILED=$((FAILED + 1))
    fi
}

execute_test_suite() {
    echo -e "\n${BOLD}--- Starting Test Cases ---${RESET}"
    while IFS=';' read -r test_name req_file expect_code host port; do
        [[ -z "$test_name" || "$test_name" =~ ^#.*$ ]] && continue
        test_name=$(echo "$test_name" | xargs)
        req_file=$(echo "$req_file" | xargs)
        expect_code=$(echo "$expect_code" | xargs)
        host=$(echo "$host" | xargs)
        port=$(echo "$port" | xargs)
        run_test "$test_name" "$req_file" "$expect_code" "$host" "$port"
    done < "$TEST_SUITE_FILE"
}

print_summary() {
    echo -e "\n${CYAN}====================================================${RESET}"
    echo -e "${BOLD}📊 Test Summary${RESET}"
    echo -e "Total:   ${BOLD}$TOTAL${RESET} cases"
    echo -e "Passed:  ${GREEN}${BOLD}$PASSED${RESET} cases"
    if [ $FAILED -gt 0 ]; then
        echo -e "Failed:  ${RED}${BOLD}$FAILED${RESET} cases"
    else
        echo -e "Failed:  ${GRAY}0${RESET} cases"
    fi
    echo -e "${CYAN}====================================================${RESET}\n"
}

check_prerequisites
start_webserv
execute_test_suite
print_summary

if [ $FAILED -gt 0 ]; then exit 1; fi
exit 0