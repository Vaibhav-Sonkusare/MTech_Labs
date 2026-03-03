#!/bin/bash
# Protocol Comparison Benchmark
# Runs all protocol implementations and compares performance

set -e

# Configuration
TEST_FILE="test_files/1mb.bin"
PORT_BASE=9100
LOSS_RATES=(0.0 0.10 0.30 0.50)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create output directory
mkdir -p data/output

# Build all protocols
echo -e "${BLUE}Building all protocol variants...${NC}"
make protocols
echo ""

# Generate test file if needed
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${YELLOW}Generating test file...${NC}"
    python3 generate_data.py 2>/dev/null || dd if=/dev/urandom of="$TEST_FILE" bs=1M count=1
fi

FILE_SIZE=$(stat -c%s "$TEST_FILE")
echo -e "${BLUE}Test file: $TEST_FILE ($FILE_SIZE bytes)${NC}"
echo ""

# Function to run a single protocol test
run_test() {
    local PROTOCOL=$1
    local SENDER=$2
    local RECEIVER=$3
    local LOSS=$4
    local PORT=$5
    
    # Clean previous output
    rm -f data/output/$(basename "$TEST_FILE")
    
    # Start receiver in background
    $RECEIVER $PORT output.bin $LOSS > /tmp/receiver_${PROTOCOL}.log 2>&1 &
    local REC_PID=$!
    sleep 0.5
    
    # Run sender with timing
    local START=$(date +%s.%N)
    $SENDER 127.0.0.1 $PORT "$TEST_FILE" $LOSS > /tmp/sender_${PROTOCOL}.log 2>&1
    local END=$(date +%s.%N)
    
    # Wait for receiver
    wait $REC_PID 2>/dev/null || true
    
    # Calculate time
    local TIME=$(echo "$END - $START" | bc)
    
    # Check file integrity
    local STATUS="FAIL"
    local OUTPUT_FILE="data/output/$(basename "$TEST_FILE")"
    if [ -f "$OUTPUT_FILE" ]; then
        if diff -q "$TEST_FILE" "$OUTPUT_FILE" > /dev/null 2>&1; then
            STATUS="PASS"
        fi
    fi
    
    # Extract memory from /proc (approximate peak RSS)
    local MEM_KB="N/A"
    
    # Get packet stats from sender log
    local PKTS_SENT=$(grep "Sent packets:" /tmp/sender_${PROTOCOL}.log | awk '{print $NF}' || echo "0")
    
    echo "$PROTOCOL,$LOSS,$TIME,$STATUS,$PKTS_SENT"
}

# Run benchmarks
echo "========================================================================"
echo "                    PROTOCOL BENCHMARK RESULTS                          "
echo "========================================================================"
echo ""

declare -A RESULTS

for LOSS in "${LOSS_RATES[@]}"; do
    echo -e "${YELLOW}Testing at ${LOSS} loss rate...${NC}"
    
    PORT=$PORT_BASE
    
    # Stop-and-Wait
    SAW_RESULT=$(run_test "SAW" "./bin/sender_saw" "./bin/receiver_saw" $LOSS $((PORT++)))
    
    # Go-Back-N
    GBN_RESULT=$(run_test "GBN" "./bin/sender_gbn" "./bin/receiver_gbn" $LOSS $((PORT++)))
    
    # Selective Repeat
    SR_RESULT=$(run_test "SR" "./bin/sender_sr" "./bin/receiver_sr" $LOSS $((PORT++)))
    
    # Blast Protocol (original)
    BLAST_RESULT=$(run_test "BLAST" "./bin/sender" "./bin/receiver" $LOSS $((PORT++)))
    
    RESULTS["$LOSS,SAW"]=$SAW_RESULT
    RESULTS["$LOSS,GBN"]=$GBN_RESULT
    RESULTS["$LOSS,SR"]=$SR_RESULT
    RESULTS["$LOSS,BLAST"]=$BLAST_RESULT
    
    PORT_BASE=$((PORT_BASE + 10))
done

echo ""
echo "========================================================================"
echo "                         COMPARISON TABLE                               "
echo "========================================================================"
printf "%-20s %10s %12s %10s %12s\n" "Protocol" "Loss Rate" "Time (s)" "Status" "Pkts Sent"
echo "------------------------------------------------------------------------"

for LOSS in "${LOSS_RATES[@]}"; do
    for PROTO in SAW GBN SR BLAST; do
        RESULT=${RESULTS["$LOSS,$PROTO"]}
        IFS=',' read -r NAME LRATE TIME STATUS PKTS <<< "$RESULT"
        
        if [ "$STATUS" == "PASS" ]; then
            STATUS_COLOR="${GREEN}PASS${NC}"
        else
            STATUS_COLOR="${RED}FAIL${NC}"
        fi
        
        printf "%-20s %10s %12.3f %b %12s\n" "$NAME" "$LRATE" "$TIME" "$STATUS_COLOR" "$PKTS"
    done
    echo "------------------------------------------------------------------------"
done

echo ""
echo "========================================================================"
echo "                           SUMMARY                                      "
echo "========================================================================"

# Find fastest for each loss rate
for LOSS in "${LOSS_RATES[@]}"; do
    echo -e "\n${BLUE}At ${LOSS} loss rate:${NC}"
    
    FASTEST=""
    FASTEST_TIME=999999
    
    for PROTO in SAW GBN SR BLAST; do
        RESULT=${RESULTS["$LOSS,$PROTO"]}
        IFS=',' read -r NAME LRATE TIME STATUS PKTS <<< "$RESULT"
        
        if [ "$STATUS" == "PASS" ]; then
            COMPARE=$(echo "$TIME < $FASTEST_TIME" | bc -l)
            if [ "$COMPARE" == "1" ]; then
                FASTEST_TIME=$TIME
                FASTEST=$NAME
            fi
        fi
    done
    
    if [ -n "$FASTEST" ]; then
        echo -e "  Fastest: ${GREEN}$FASTEST${NC} (${FASTEST_TIME}s)"
        
        # Show relative speeds
        for PROTO in SAW GBN SR BLAST; do
            RESULT=${RESULTS["$LOSS,$PROTO"]}
            IFS=',' read -r NAME LRATE TIME STATUS PKTS <<< "$RESULT"
            
            if [ "$STATUS" == "PASS" ] && [ "$NAME" != "$FASTEST" ]; then
                SLOWDOWN=$(echo "scale=2; $TIME / $FASTEST_TIME" | bc)
                echo "  $NAME: ${SLOWDOWN}x slower"
            fi
        done
    fi
done

echo ""
echo -e "${GREEN}Benchmark complete!${NC}"
