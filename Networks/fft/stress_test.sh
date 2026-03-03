#!/bin/bash

# Compile first
make clean && make
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

mkdir -p data/output

# Function to run test
run_test() {
    FILE=$1
    LOSS=$2
    echo "========================================================"
    echo "Running test: File=$FILE, LossRate=$LOSS"
    echo "========================================================"

    # Start Receiver in background
    ./bin/receiver 9005 $FILE $LOSS > receiver.log 2>&1 &
    REC_PID=$!
    
    sleep 1

    # Start Sender
    ./bin/sender 127.0.0.1 9005 $FILE $LOSS > sender.log 2>&1
    
    # Wait for receiver to finish
    wait $REC_PID
    
    # Check Diff
    BASENAME=$(basename $FILE)
    if diff $FILE data/output/$BASENAME > /dev/null; then
        echo "SUCCESS: Files match."
    else
        echo "FAILURE: Files differ!"
        exit 1
    fi
}

# Generate data if not exists
python3 generate_data.py

# Run Tests
# 1. 1MB file, 0% loss (Sanity)
run_test "test_files/1mb.bin" 0.0

# 2. 1MB file, 5% loss (Robustness)
run_test "test_files/1mb.bin" 0.05

# 3. 10MB file, 1% loss (Scalability + Robustness)
run_test "test_files/10mb.bin" 0.01

echo "All stress tests passed!"
run_test 'data/input/10M.txt' 0.5
