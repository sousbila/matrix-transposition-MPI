#!/bin/bash

# Paths
BIN_PATH="../bin/main"

# Parameters
OMP_THREADS=32
MPI_PROCS=64
BLOCK_SIZE=64
MATRIX_SIZES=(4096)         

# Methods
# METHODS=("serial" "serialblock" "omp" "mpi" "mpi2" "mpi3" "mpi4")
# METHODS=("serial" "serialblock" "omp" "mpi_blocks1")
# METHODS=("serial" "omp" "mpi" "mpi2" "mpi3" "mpi_blocks1" "mpi_blocks2" "mpi_blocks3")
# METHODS=("serial" "omp" "mpi" "mpi3" "mpi_blocks3")
# METHODS=("serial" "serialblock" "omp" "mpi" "mpi_blocks3")

METHODS=("omp" "mpi3" "mpi_blocks3") 

# Function to run and format output (with matrix display)
run_and_print_results() {
    local matrix_size=$1

    echo "Matrix size: $matrix_size"

    for method in "${METHODS[@]}"; do
        if [ "$method" == "serial" ]; then
            time_output=$(mpirun -np 1 $BIN_PATH -m serial -n $matrix_size -c 2>&1)
        elif [ "$method" == "serialblock" ]; then
            time_output=$(mpirun -np 1 $BIN_PATH -m serialblock -n $matrix_size -b $BLOCK_SIZE -c 2>&1)
        elif [ "$method" == "omp" ]; then
            export OMP_NUM_THREADS=$OMP_THREADS
            time_output=$(mpirun -np 1 $BIN_PATH -m omp -n $matrix_size -c 2>&1)
        elif [ "$method" == "mpi" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi -n $matrix_size -c 2>&1)
        elif [ "$method" == "mpi2" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi2 -n $matrix_size -c 2>&1)
        elif [ "$method" == "mpi3" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi3 -n $matrix_size -c 2>&1)
        elif [ "$method" == "mpi_blocks1" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi_blocks1 -n $matrix_size -c 2>&1)
        elif [ "$method" == "mpi_blocks3" ]; then
            time_output=$(mpirun -np $MPI_PROCS $BIN_PATH -m mpi_blocks3 -n $matrix_size -c 2>&1)
        fi

        echo "---------------------------------"
        echo "[Method: $method]"
        echo "$time_output"  # Print the full output including matrices
        echo "---------------------------------"
    done
    echo "================"
}

# Run tests for each matrix size
for size in "${MATRIX_SIZES[@]}"; do
    run_and_print_results $size
    echo "================================="
done
