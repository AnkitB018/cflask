#!/usr/bin/env bash
# Run from the wrk repository directory, where ./wrk exists.
# The cflask server must already be running in another terminal.
#
# Usage:
#   chmod +x run_loadtest.sh
#   ./run_loadtest.sh
#
# Optional:
#   DURATION=30s WRK_THREADS=1 ./run_loadtest.sh

set -u

DURATION="${DURATION:-30s}"
WRK_THREADS="${WRK_THREADS:-1}"
WRK_CPU_SET="4-7"
OUTPUT="loadtest3a.data"
CONNECTIONS=(1 2 4 8 16 32 64 128)

if [[ ! -x ./wrk ]]; then
    echo "Error: ./wrk not found or not executable."
    echo "Run this script from the folder containing the ./wrk executable."
    exit 1
fi

if ! command -v taskset >/dev/null 2>&1; then
    echo "Error: taskset is unavailable. Run this script on Linux/Kali."
    exit 1
fi

if ! taskset -c "$WRK_CPU_SET" true 2>/dev/null; then
    echo "Error: CPU IDs 4-7 are not available to this process."
    echo "Check with: nproc; taskset -pc $$; lscpu"
    exit 1
fi

BASE_URL="http://127.0.0.1:8080"
if ! curl -fsS --max-time 2 "$BASE_URL/square?num=3" >/dev/null 2>&1; then
    echo "Error: cflask is not responding at $BASE_URL."
    echo "Start your server in another terminal first, pinned to CPUs 0-2."
    exit 1
fi

# Fresh output for this run, so results from separate server modes are not mixed.
{
    echo "cflask load test results"
    echo "Date: $(date)"
    echo "Server mode: thread-pool mode(t) with 2 threads -- expected to be running separately"
    echo "Server CPU affinity: manually set to CPUs 0-2"
    echo "wrk CPU affinity: $WRK_CPU_SET"
    echo "wrk threads: $WRK_THREADS"
    echo "Duration per test: $DURATION"
    echo "Connection counts: ${CONNECTIONS[*]}"
    echo
    echo "Each test records the complete wrk report."
    echo "============================================================"
} > "$OUTPUT"

NAMES=("square" "helloworld" "prime")
URLS=(
    "$BASE_URL/square?num=3"
    "$BASE_URL/helloworld?str=Ashwin"
    "$BASE_URL/arithmetic/prime?num=1000000007"
)

for i in "${!NAMES[@]}"; do
    name="${NAMES[$i]}"
    url="${URLS[$i]}"

    {
        echo
        echo "############################################################"
        echo "FUNCTION: $name"
        echo "URL: $url"
        echo "############################################################"
    } | tee -a "$OUTPUT"

    for c in "${CONNECTIONS[@]}"; do
        echo
        echo "[$name] concurrent connections: $c"

        {
            echo
            echo "------------------------------------------------------------"
            echo "Function: $name"
            echo "Connections (-c): $c"
            echo "Command: taskset -c $WRK_CPU_SET ./wrk -t $WRK_THREADS -c $c -d $DURATION --latency \"$url\""
            echo "------------------------------------------------------------"
        } >> "$OUTPUT"

        taskset -c "$WRK_CPU_SET" ./wrk \
            -t "$WRK_THREADS" \
            -c "$c" \
            -d "$DURATION" \
            --latency \
            "$url" 2>&1 | tee -a "$OUTPUT"

        wrk_status=${PIPESTATUS[0]}
        if [[ "$wrk_status" -ne 0 ]]; then
            echo "Error: wrk failed for $name with -c $c. See $OUTPUT." | tee -a "$OUTPUT"
            exit "$wrk_status"
        fi

        echo | tee -a "$OUTPUT"
        sleep 1
    done
done

echo "All tests complete. Results saved to: $OUTPUT"

