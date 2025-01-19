#!/bin/bash

# Paths
BIN_PATH="../bin/main"

# Parameters
OMP_THREADS=16
# Arrays for MPI process counts
MPI_PROCS_BLOCK=(4 16 64)
# Matrix sizes, ensuring each size is a power of two for divisibility
MATRIX_SIZES=(16 32 64 128 256 512 1024 2048 4096)

# Output files
CSV_FILE="../results/results4.csv"
TXT_FILE="../results/results4.txt"

# Initialize files
echo "Method,Matrix Size,Processors,Time" > "$CSV_FILE"
echo "Performance Results:" > "$TXT_FILE"

# Function to run and format output
run_and_record_results() {
    local method=$1
    local matrix_size=$2
    local procs=$3

    # Ensure matrix size is divisible by number of processors for MPI runs
    if (( matrix_size % procs == 0 )); then
        # Run the method and capture full output for debugging
        local time_output=$(mpirun -np $procs $BIN_PATH -m $method -n $matrix_size 2>&1)
        echo "Output for $method with size $matrix_size and procs $procs:"
        echo "$time_output"
        echo "------------------------------------------------------"

        # Attempt to extract the execution time for transposing
        local time_value=$(echo "$time_output" | grep "Transpose time:" | awk -F': ' '{print $2}' | awk '{print $1}')
        if [ -z "$time_value" ]; then
            time_value="N/A"
        fi

        # Save to CSV
        echo "$method,$matrix_size,$procs,$time_value" >> "$CSV_FILE"

        # Save to text file
        echo "$method     | Size: $matrix_size     | Procs: $procs   | Time: $time_value" >> "$TXT_FILE"
    fi
}

# Run for each matrix size
for size in "${MATRIX_SIZES[@]}"; do
    # Run serial and MPI methods
    run_and_record_results "serialblock" "$size" "1"
    for procs in "${MPI_PROCS_BLOCK[@]}"; do
        run_and_record_results "mpi_blocks3" "$size" "$procs"
    done
done

echo "Results saved to $CSV_FILE and $TXT_FILE"
