#!/bin/sh

# Get the directory of this script
SCRIPT_DIR="$(dirname "$(realpath "$0")")"

# Set up database
export PGDATABASE=test_hashset_send_recv
dropdb --if-exists "$PGDATABASE"
createdb

# Define directories
EXPECTED_DIR="$SCRIPT_DIR/../expected"
RESULTS_DIR="$SCRIPT_DIR/../results"

# Create the results directory if it doesn't exist
mkdir -p "$RESULTS_DIR"

# Run the C test and save its output to the results directory
"$SCRIPT_DIR/test_send_recv" > "$RESULTS_DIR/test_send_recv.out"

printf "test test_send_recv               ... "

# Compare the actual output with the expected output
if diff -q "$RESULTS_DIR/test_send_recv.out" "$EXPECTED_DIR/test_send_recv.out" > /dev/null 2>&1; then
    echo "ok"
    # Clean up by removing the results directory if the test passed
    rm -r "$RESULTS_DIR"
else
    echo "failed"
    git diff --no-index --color "$EXPECTED_DIR/test_send_recv.out" "$RESULTS_DIR/test_send_recv.out"
fi
