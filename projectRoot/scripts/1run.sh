#!/bin/bash

# Paths
BIN_PATH="../bin/main"

# Parameters
OMP_THREADS=32
MPI_PROCS=64
BLOCK_SIZE=64
#MATRIX_SIZES=(64 128 256 512 1024 2048 4096)
MATRIX_SIZES=(256 512 1024 2048 4096)

# Methods
# METHODS=("serialblock" "omp" "mpi" "mpi2" "mpi3" "mpi_blocks1" "mpi_blocks2" "mpi_blocks3")
METHODS=("serialblock" "omp" "mpi" "mpi2" "mpi3" "mpi_blocks1" "mpi_blocks3")

# Function to run and format output
run_and_print_results() {
    local matrix_size=$1

    echo "Matrix size: $matrix_size"

    for method in "${METHODS[@]}"; do
        if [ "$method" == "serial" ]; then
            time_output=$(mpirun -np 1 $BIN_PATH -m serial -n $matrix_size 2>&1)
        elif [ "$method" == "serialblock" ]; then
            time_output=$(mpirun -np 1 $BIN_PATH -m serialblock -n $matrix_size -b $BLOCK_SIZE 2>&1)
        elif [ "$method" == "omp" ]; then
            export OMP_NUM_THREADS=$OMP_THREADS
            time_output=$(mpirun -np 1 $BIN_PATH -m omp -n $matrix_size 2>&1)
        elif [ "$method" == "mpi" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi -n $matrix_size 2>&1)
        elif [ "$method" == "mpi2" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi2 -n $matrix_size 2>&1)
        elif [ "$method" == "mpi3" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi3 -n $matrix_size 2>&1)
        elif [ "$method" == "mpi4" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi4 -n $matrix_size 2>&1)
        elif [ "$method" == "mpi_blocks1" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi_blocks1 -n $matrix_size 2>&1)
        elif [ "$method" == "mpi_blocks2" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi_blocks2 -n $matrix_size 2>&1)
        elif [ "$method" == "mpi_blocks3" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi_blocks3 -n $matrix_size 2>&1)
        fi

        # Extract time value for "Transpose time:"
        time_value=$(echo "$time_output" | grep "Transpose time:" | awk -F': ' '{print $2}')

        if [ -z "$time_value" ]; then
            time_value="N/A"
        fi

        printf "%-15s: %s\n" "$method" "$time_value"
    done
    echo "================"
}

# Run tests for each matrix size
for size in "${MATRIX_SIZES[@]}"; do
    run_and_print_results $size
    echo "================================="
done
