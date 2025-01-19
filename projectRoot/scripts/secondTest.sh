#!/usr/bin/env bash

################################################################################
# Paths and Files
################################################################################
BIN_PATH="../bin/main"                    # Path to your compiled executable
CSV_FILE="../results/results2.csv"
TXT_FILE="../results/results2.txt"

################################################################################
# Configuration
################################################################################
MATRIX_SIZES=(16 32 64 128 256 512 1024 2048 4096)

RUNS=8  # Number of runs per configuration

################################################################################
# Init Output Files
################################################################################
echo "Method,Matrix Size,Concurrency,AvgTime" > "$CSV_FILE"
echo "Performance Results:" > "$TXT_FILE"

################################################################################
# Function: compute_average_without_outliers
# Input : array of run times (floats)
# Output: echo the average after discarding min and max if possible
################################################################################
compute_average_without_outliers() {
    local -a all_times
    all_times=("$@")

    local count=${#all_times[@]}
    if (( count == 0 )); then
        echo ""
        return
    fi

    # Sort numerically
    local -a sorted
    mapfile -t sorted < <(printf '%s\n' "${all_times[@]}" | sort -n)
    
    # If only one data point, just return it
    if (( count == 1 )); then
        echo "${sorted[0]}"
        return
    fi
    
    # If exactly 2 data points, average them
    if (( count == 2 )); then
        local sum
        sum=$(awk -v a="${sorted[0]}" -v b="${sorted[1]}" 'BEGIN { printf "%.6f", a+b }')
        local avg
        avg=$(awk -v s="$sum" 'BEGIN { printf "%.6f", s/2.0 }')
        echo "$avg"
        return
    fi

    # If >= 3 data points, drop the smallest and largest
    local -a dropped
    dropped=("${sorted[@]:1:$((count-2))}")

    local sum=0
    for val in "${dropped[@]}"; do
        sum=$(awk -v s="$sum" -v v="$val" 'BEGIN {printf "%.6f", s+v}')
    done
    local remaining_count=$((count-2))

    local avg
    avg=$(awk -v s="$sum" -v c="$remaining_count" 'BEGIN { printf "%.6f", s/c }')
    echo "$avg"
}

################################################################################
# Function: run_mpi3_test
# - method: "mpi3"
# - matrix size
# - processes
################################################################################
run_mpi3_test() {
    local size="$1"
    local procs="$2"

    # If size not divisible by procs, skip
    if (( size % procs != 0 )); then
        return
    fi

    local -a times=()

    for ((i=1; i<=RUNS; i++)); do
        local output
        output=$(mpirun -np "$procs" "$BIN_PATH" -m mpi3 -n "$size" 2>&1)
        
        # Extract transpose time
        local t
        t=$(echo "$output" | grep "Transpose time:" | awk -F': ' '{print $2}' | awk '{print $1}')
        if [[ -n "$t" ]]; then
            times+=("$t")
        fi
    done

    if (( ${#times[@]} > 0 )); then
        local avg_time
        avg_time=$(compute_average_without_outliers "${times[@]}")

        # If avg_time is non-empty
        if [[ -n "$avg_time" ]]; then
            echo "mpi3,$size,$procs,$avg_time" >> "$CSV_FILE"
            echo "mpi3     | Size: $size | Procs: $procs | Time: $avg_time" >> "$TXT_FILE"
        fi
    fi
}

################################################################################
# Function: run_omp_test
# - matrix size
# - threads
################################################################################
run_omp_test() {
    local size="$1"
    local threads="$2"

    local -a times=()

    for ((i=1; i<=RUNS; i++)); do
        local output
        output=$(OMP_NUM_THREADS="$threads" "$BIN_PATH" -m omp -n "$size" 2>&1)
        
        local t
        t=$(echo "$output" | grep "Transpose time:" | awk -F': ' '{print $2}' | awk '{print $1}')
        if [[ -n "$t" ]]; then
            times+=("$t")
        fi
    done

    if (( ${#times[@]} > 0 )); then
        local avg_time
        avg_time=$(compute_average_without_outliers "${times[@]}")

        if [[ -n "$avg_time" ]]; then
            echo "omp,$size,$threads,$avg_time" >> "$CSV_FILE"
            echo "omp      | Size: $size | Threads: $threads | Time: $avg_time" >> "$TXT_FILE"
        fi
    fi
}

################################################################################
# Example: run_serial_test if needed
################################################################################
run_serial_test() {
    local size="$1"

    local -a times=()

    for ((i=1; i<=RUNS; i++)); do
        local output
        output=$("$BIN_PATH" -m serial -n "$size" 2>&1)
        
        local t
        t=$(echo "$output" | grep "Transpose time:" | awk -F': ' '{print $2}' | awk '{print $1}')
        if [[ -n "$t" ]]; then
            times+=("$t")
        fi
    done

    if (( ${#times[@]} > 0 )); then
        local avg_time
        avg_time=$(compute_average_without_outliers "${times[@]}")

        if [[ -n "$avg_time" ]]; then
            echo "serial,$size,1,$avg_time" >> "$CSV_FILE"
            echo "serial   | Size: $size | Procs: 1 | Time: $avg_time" >> "$TXT_FILE"
        fi
    fi
}

################################################################################
# Main loop
################################################################################
for size in "${MATRIX_SIZES[@]}"; do
    # Example serial run (uncomment if needed):
    # run_serial_test "$size"

    # OpenMP scheme:
    #  - up to 512 => 4 threads
    #  - == 1024   => 16 threads
    #  - >= 2048   => 32 threads
    if (( size <= 512 )); then
        run_omp_test "$size" 4
    elif (( size == 1024 )); then
        run_omp_test "$size" 16
    else
        run_omp_test "$size" 32
    fi

    # Always run mpi3 baseline with 1 process
    run_mpi3_test "$size" 1

    # Use your simplified scheme for mpi3 concurrency:
    if   (( size <= 64 )); then
        run_mpi3_test "$size" 4
    elif (( size == 128 || size == 256 )); then
        run_mpi3_test "$size" 8
    elif (( size == 512 )); then
        run_mpi3_test "$size" 16
    else
        run_mpi3_test "$size" 32
    fi
done

echo "Results saved to $CSV_FILE and $TXT_FILE."
