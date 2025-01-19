#!/usr/bin/env bash

################################################################################
# Paths and Files
################################################################################
BIN_PATH="../bin/main"        # Path to your compiled executable
CSV_FILE="../results/results5.csv"
TXT_FILE="../results/results5.txt"

################################################################################
# Configuration
################################################################################
MATRIX_SIZES=(16 32 64 128 256 512 1024 2048 4096)
RUNS=8  # number of runs per configuration

################################################################################
# Init Output Files
################################################################################
echo "Method,MatrixSize,Procs,AvgTime" > "$CSV_FILE"
echo "Performance Results:" > "$TXT_FILE"

################################################################################
# Helper: Remove outliers & compute average
################################################################################
compute_average_without_outliers() {
    local -a all_times=("$@")
    local count="${#all_times[@]}"
    if (( count == 0 )); then
        echo ""
        return
    fi

    # Sort numerically
    local -a sorted
    mapfile -t sorted < <(printf '%s\n' "${all_times[@]}" | sort -n)

    # If only 1 data point, just return it
    if (( count == 1 )); then
        echo "${sorted[0]}"
        return
    fi

    # If exactly 2 data points, average them
    if (( count == 2 )); then
        local sum
        sum=$(awk -v a="${sorted[0]}" -v b="${sorted[1]}" 'BEGIN {printf "%.6f", a+b}')
        local avg
        avg=$(awk -v s="$sum" 'BEGIN {printf "%.6f", s/2.0}')
        echo "$avg"
        return
    fi

    # If >= 3 data points, drop min & max
    local -a dropped=("${sorted[@]:1:$((count-2))}")
    local sum=0
    for val in "${dropped[@]}"; do
        sum=$(awk -v s="$sum" -v v="$val" 'BEGIN {printf "%.6f", s+v}')
    done
    local remaining_count=$((count-2))
    local avg
    avg=$(awk -v s="$sum" -v c="$remaining_count" 'BEGIN {printf "%.6f", s/c}')
    echo "$avg"
}

################################################################################
# Run "serialblock" (1 proc)
################################################################################
run_serialblock_test() {
    local size="$1"
    local -a times=()

    for ((i=1; i<=RUNS; i++)); do
        local output
        output=$("$BIN_PATH" -m serialblock -n "$size" 2>&1)

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
            echo "serialblock,$size,1,$avg_time" >> "$CSV_FILE"
            echo "serialblock | Size: $size | Procs: 1 | Time: $avg_time" >> "$TXT_FILE"
        fi
    fi
}

################################################################################
# Run "mpi3" 
################################################################################
run_mpi3_test() {
    local size="$1"
    local procs="$2"

    # If the matrix is not divisible by procs, skip to keep balanced
    if (( size % procs != 0 )); then
        return
    fi

    local -a times=()
    for ((i=1; i<=RUNS; i++)); do
        local output
        output=$(mpirun -np "$procs" "$BIN_PATH" -m mpi3 -n "$size" 2>&1)

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
            echo "mpi3,$size,$procs,$avg_time" >> "$CSV_FILE"
            echo "mpi3       | Size: $size | Procs: $procs | Time: $avg_time" >> "$TXT_FILE"
        fi
    fi
}

################################################################################
# Run "mpiblocks3"
################################################################################
run_mpiblocks3_test() {
    echo "DEBUG: Trying mpirun -np $2 ... for size $1"
    local size="$1"
    local procs="$2"

    if (( size % procs != 0 )); then
        return
    fi
    
    echo "2DEBUG: Trying mpirun -np $2 ... for size $1"

    local -a times=()
    for ((i=1; i<=RUNS; i++)); do
        local output
        output=$(mpirun -np "$procs" "$BIN_PATH" -m mpi_blocks3 -n "$size" 2>&1)

        local t
        t=$(echo "$output" | grep "Transpose time:" | awk -F': ' '{print $2}' | awk '{print $1}')
        if [[ -n "$t" ]]; then
            times+=("$t")
        fi
    done
    
    echo "3DEBUG: Trying mpirun -np $2 ... for size $1"

    if (( ${#times[@]} > 0 )); then
        local avg_time
        avg_time=$(compute_average_without_outliers "${times[@]}")
        if [[ -n "$avg_time" ]]; then
            echo "mpiblocks3,$size,$procs,$avg_time" >> "$CSV_FILE"
            echo "mpiblocks3 | Size: $size | Procs: $procs | Time: $avg_time" >> "$TXT_FILE"
        fi
    fi
    
    echo "4DEBUG: Trying mpirun -np $2 ... for size $1"
}

################################################################################
# Main loop: 
# For each size, run: 
# 1) serialblock
# 2) mpi3 with an optimal process count (4 if <=512 else 16)
# 3) mpiblocks3 with an optimal process count (4 if <=512 else 16)
################################################################################
for size in "${MATRIX_SIZES[@]}"; do
    run_serialblock_test "$size"

    # pick procs for mpi3
    if (( size <= 64 )); then
        run_mpi3_test "$size" 4
    elif (( size <= 256 )); then
        run_mpi3_test "$size" 8
    elif (( size <= 512 )); then
        run_mpi3_test "$size" 16
    else
        run_mpi3_test "$size" 32
    fi

    # pick procs for mpiblocks3
    if (( size <= 512 )); then
        run_mpiblocks3_test "$size" 4
    else
        run_mpiblocks3_test "$size" 16
    fi
done

echo "Results saved to $CSV_FILE and $TXT_FILE."

