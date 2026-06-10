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

# Toggles for Verbose Output & Debugging
DEBUG_MODE=${DEBUG_MODE:-1}          # Set to 1 to view Raw Headers ONLY on failures
SHOW_ALL_RESPONSES=${SHOW_ALL_RESPONSES:-1}  # Set to 1 to print the COMPLETE raw response & body for ALL tests (pass or fail)
SHOW_PASSED=${SHOW_PASSED:-0}         # Set to 0 to hide successful tests from output (failures only)

# Temp files for validation
HEADERS_FILE="/tmp/webserv_headers_parsed.txt"
BODY_FILE="/tmp/webserv_body_parsed.bin"

# ==============================================================================
# Colors for output (Evaluated using Bash ANSI-C Quoting to protect raw streams)
# ==============================================================================
RESET=$'\e[0m'
BOLD=$'\e[1m'
GREEN=$'\e[32m'
RED=$'\e[31m'
YELLOW=$'\e[33m'
CYAN=$'\e[36m'
GRAY=$'\e[90m'

TOTAL=0
PASSED=0
FAILED=0
WEBSERV_PID=0

# Global tracking variables for detailed reporting
CHECKED_STATUS_INFO=""
CHECKED_HEADERS_INFO=""

cleanup() {
    echo -e "\n${YELLOW}[!] Shutting down Webserv (PID: $WEBSERV_PID)...${RESET}"
    if kill -0 $WEBSERV_PID 2>/dev/null; then
        kill -9 $WEBSERV_PID
        wait $WEBSERV_PID 2>/dev/null
    fi
    rm -f $TEMP_RES_FILE $HEADERS_FILE $BODY_FILE
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
    echo -e "${BOLD}${CYAN}🚀 Starting Webserv Tester 42 (Detailed Header Check)${RESET}"
    echo -e "${CYAN}====================================================${RESET}"
    
    # Run webserv in the background
    $WEBSERV_BIN $DEFAULT_CONF > /tmp/webserv_output.log 2>&1 &
    WEBSERV_PID=$!
    
    echo -e "${GRAY}>> Webserv is running with PID: $WEBSERV_PID${RESET}"
    sleep 1
}

# ==============================================================================
# Response Parsing & Validating Logic
# ==============================================================================
validate_response() {
    local expect_code="$1"
    local actual_code="${2:-EMPTY}"
    local expect_headers="$3"
    local errors=()
    local checked_list=()

    # 1. Base status code match
    CHECKED_STATUS_INFO="Expected: $expect_code | Got: $actual_code"
    if [ "$actual_code" != "$expect_code" ]; then
        errors+=("Status Code Mismatch: Expected $expect_code but got $actual_code")
    fi

    # Feature: If expect_headers is set to "SKIP", bypass all header validations
    if [ "$expect_headers" = "SKIP" ]; then
        CHECKED_HEADERS_INFO="None (SKIP mode)"
        if [ ${#errors[@]} -ne 0 ]; then
            for err in "${errors[@]}"; do
                echo -e "      ${RED}↳ [FAIL] $err${RESET}"
            done
            return 1
        fi
        return 0
    fi

    # Extract clean headers (excluding status line) and body preserving bytes
    # Find how many header lines there are before the CRLF CRLF delimiter
    local header_lines_count=$(awk 'BEGIN {RS="\r\n"; c=0} {if ($0 == "") exit; c++} END {print c}' "$TEMP_RES_FILE")
    
    # Extract headers (removing carriage returns)
    awk 'BEGIN {RS="\r\n"} NR>1 {if ($0 == "") exit; print}' "$TEMP_RES_FILE" | tr -d '\r' > "$HEADERS_FILE"
    
    # Extract body in binary format
    tail -n +$((header_lines_count + 2)) "$TEMP_RES_FILE" > "$BODY_FILE" 2>/dev/null

    # 2. Basic Header Syntax & Specification Check
    local parsed_headers_count=0
    while read -r header_line || [ -n "$header_line" ]; do
        [ -z "$header_line" ] && continue
        parsed_headers_count=$((parsed_headers_count + 1))
        
        # RFC 9110 constraint check: space before colon is strictly forbidden
        if [[ "$header_line" =~ ^[^:]+[[:space:]]: ]]; then
            errors+=("RFC Violation: Space found before colon in header: '$header_line'")
        fi
        
        # Valid header must contain a colon separator
        if [[ ! "$header_line" =~ : ]]; then
            errors+=("Syntax Error: Invalid header format: '$header_line'")
        fi
    done < "$HEADERS_FILE"

    if [ $parsed_headers_count -gt 0 ]; then
        checked_list+=("Syntax ($parsed_headers_count headers verified)")
    fi

    # 3. Content-Length Accuracy Check
    local content_length_val=$(grep -i "^Content-Length:" "$HEADERS_FILE" | awk -F: '{print $2}' | xargs)
    if [ -n "$content_length_val" ]; then
        checked_list+=("Content-Length Match")
        # Check if the Content-Length is a valid positive integer
        if [[ ! "$content_length_val" =~ ^[0-9]+$ ]]; then
            errors+=("Invalid Header value: Content-Length must be an integer, got: '$content_length_val'")
        else
            # Calculate actual body size
            local actual_body_size=$(wc -c < "$BODY_FILE" | xargs)
            if [ "$content_length_val" -ne "$actual_body_size" ]; then
                errors+=("Body Mismatch: Content-Length header is $content_length_val bytes, but actual body received is $actual_body_size bytes")
            fi
        fi
    fi

    # 4. Standard Required Headers per specific Status Codes
    if [ "$actual_code" == "405" ]; then
        checked_list+=("Allow (Mandatory for 405)")
        # 405 Method Not Allowed MUST include an "Allow" header specifying route capabilities
        if ! grep -q -i "^Allow:" "$HEADERS_FILE"; then
            errors+=("RFC Violation: 405 Method Not Allowed responses MUST include an 'Allow' header")
        fi
    fi

    if [ "$actual_code" == "201" ]; then
        checked_list+=("Location (Mandatory for 201)")
        # 201 Created should include a "Location" header for resource orientation
        if ! grep -q -i "^Location:" "$HEADERS_FILE"; then
            errors+=("Specification Missing: 201 Created response should provide a 'Location' header")
        fi
    fi

    # 5. User-Defined Custom Header assertions (CSV parameters)
    if [ -n "$expect_headers" ]; then
        IFS=',' read -ra ADDR <<< "$expect_headers"
        for assertion in "${ADDR[@]}"; do
            # Trim whitespace and guard against empty array values from trailing commas
            assertion=$(echo "$assertion" | xargs)
            [ -z "$assertion" ] && continue

            local expected_key=$(echo "$assertion" | cut -d':' -f1 | xargs)
            local expected_val=$(echo "$assertion" | cut -d':' -f2- | xargs)
            
            checked_list+=("$expected_key (Value Match)")

            # Retrieve the actual header value (case-insensitive key match)
            local actual_val=$(grep -i "^${expected_key}:" "$HEADERS_FILE" | cut -d':' -f2- | xargs)
            
            if [ -z "$actual_val" ]; then
                errors+=("Assertion Failed: Expected header '${expected_key}' was not returned by the server")
            elif [ -n "$expected_val" ] && [ "$actual_val" != "$expected_val" ]; then
                errors+=("Assertion Failed: Header '${expected_key}' expected value '${expected_val}' but got '${actual_val}'")
            fi
        done
    fi

    # Format the checked headers string for display
    if [ ${#checked_list[@]} -eq 0 ]; then
        CHECKED_HEADERS_INFO="None"
    else
        CHECKED_HEADERS_INFO=$(IFS=', '; echo "${checked_list[*]}")
    fi

    # Return errors if any found
    if [ ${#errors[@]} -ne 0 ]; then
        for err in "${errors[@]}"; do
            echo -e "      ${RED}↳ [FAIL] $err${RESET}"
        done
        return 1
    fi
    return 0
}

run_test() {
    local test_name="$1"
    local req_file="$2"
    local expect_code="$3"
    local host="$4"
    local port="$5"
    local expect_headers="$6"

    TOTAL=$((TOTAL + 1))
    local full_req_path="${REQUESTS_DIR}/${req_file}"
    
    if [ ! -f "$full_req_path" ]; then
        echo -e "  ${RED}[ERROR] Request file not found: $full_req_path${RESET}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Clear old response files
    > "$TEMP_RES_FILE"
    > "$HEADERS_FILE"
    > "$BODY_FILE"
    CHECKED_STATUS_INFO=""
    CHECKED_HEADERS_INFO=""

    # Use Bash Built-in /dev/tcp to open Socket
    exec 3<>/dev/tcp/$host/$port 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "  ${RED}[KO] ${test_name} -> CONNECTION REFUSED (Cannot connect to server)${RESET}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Send Raw Request
    cat "$full_req_path" >&3 2>/dev/null

    # Read Response with timeout
    timeout $TIMEOUT_SEC cat <&3 > "$TEMP_RES_FILE" 2>/dev/null
    local exit_status=$?

    # Close Socket
    exec 3<&- 2>/dev/null

    # Check for Server Hang/Timeout
    if [ $exit_status -eq 124 ] && [ ! -s "$TEMP_RES_FILE" ]; then
        echo -e "  ${RED}[KO] ${test_name} -> TIMEOUT (Server did not respond in ${TIMEOUT_SEC}s)${RESET}"
        FAILED=$((FAILED + 1))
        return
    fi

    # Read the first line to check, e.g., HTTP/1.1 200 OK
    local first_line=$(head -n 1 "$TEMP_RES_FILE" | tr -d '\r')
    local actual_code=$(echo "$first_line" | awk '{print $2}')

    # Validate the structure, syntax, and content constraints of response
    validate_response "$expect_code" "$actual_code" "$expect_headers"
    local test_res=$?

    if [ $test_res -eq 0 ]; then
        if [ $SHOW_PASSED -eq 1 ]; then
            echo -e "  ${GREEN}[OK] ${test_name}${RESET}"
            echo -e "       ${GRAY}↳ Status Code : ${CYAN}${CHECKED_STATUS_INFO}${RESET}"
            echo -e "       ${GRAY}↳ Verifications: ${YELLOW}${CHECKED_HEADERS_INFO}${RESET}"
        fi
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}[KO] ${test_name} -> Validation checks failed!${RESET}"
        
        # Check if Webserv crashed
        if ! kill -0 $WEBSERV_PID 2>/dev/null; then
            echo -e "      ${YELLOW}-> [!] Warning: Webserv (PID: $WEBSERV_PID) crashed or stopped!${RESET}"
        fi

        # Debug mode prints raw parsed headers & code response
        if [ $DEBUG_MODE -eq 1 ] && [ $SHOW_ALL_RESPONSES -ne 1 ]; then
            echo -e "      ${GRAY}--- Raw Response Header Block ---${RESET}"
            if [ -f "$HEADERS_FILE" ] && [ -s "$HEADERS_FILE" ]; then
                cat "$HEADERS_FILE" | while read -r line; do echo -e "      ${GRAY}$line${RESET}"; done
            else
                echo -e "      ${GRAY}(Empty headers, or connection dropped early)${RESET}"
            fi
            echo -e "      ${GRAY}---------------------------------${RESET}"
        fi
        FAILED=$((FAILED + 1))
    fi

    # Global Toggle Block: Displays the FULL raw response if SHOW_ALL_RESPONSES=1
    if [ $SHOW_ALL_RESPONSES -eq 1 ] && { [ $test_res -ne 0 ] || [ $SHOW_PASSED -eq 1 ]; }; then
        echo -e "      ${GRAY}┌─── RAW HTTP RESPONSE STREAM ──────────────────────────────────────────┐${RESET}"
        if [ -s "$TEMP_RES_FILE" ]; then
            # Uses sed to safely prefix raw data lines, preserving carriage returns and non-ASCII chars
            sed "s/^/      ${GRAY}│${RESET} /" "$TEMP_RES_FILE"
        else
            echo -e "      ${GRAY}│ (No data received from socket - connection dropped cleanly)${RESET}"
        fi
        echo -e "      ${GRAY}└─── END OF STREAM ──────────────────────────────────────────────────────┘${RESET}"
    fi
}

execute_test_suite() {
    echo -e "\n${BOLD}--- Starting Test Cases ---${RESET}"
    # Read CSV fields (6th field is the optional expect_headers assertions)
    while IFS=';' read -r test_name req_file expect_code host port expect_headers || [ -n "$test_name" ]; do
        # 1. Clean carriage returns and whitespaces first
        test_name=$(echo "$test_name" | tr -d '\r' | xargs)
        
        # 2. Robustly check if empty or a comment line AFTER cleanup
        [[ -z "$test_name" || "$test_name" =~ ^#.*$ ]] && continue
        
        # 3. Clean and populate remaining parameters safely
        req_file=$(echo "$req_file" | tr -d '\r' | xargs)
        expect_code=$(echo "$expect_code" | tr -d '\r' | xargs)
        host=$(echo "$host" | xargs)
        port=$(echo "$port" | xargs)
        expect_headers=$(echo "$expect_headers" | tr -d '\r' | xargs)
        
        run_test "$test_name" "$req_file" "$expect_code" "$host" "$port" "$expect_headers"
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