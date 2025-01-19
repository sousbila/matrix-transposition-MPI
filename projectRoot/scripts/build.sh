#!/bin/bash

# This script compiles all sources into executables.


# Navigate to project root (if scripts/ is one level down)
cd "$(dirname "$0")"

# Create a bin directory for output
mkdir -p ../bin

echo "Building HPC Project..."

mpicc -std=c99 -fopenmp -o ../bin/main \
    ../src/main.c \
    ../src/serialblock_code.c \
    ../src/openmp_code.c \
    ../src/mpi_code.c \
    ../src/mpi_code2.c \
    ../src/mpi_code3.c \
    ../src/mpiblock_code1.c \
    ../src/mpiblock_code3.c \
    ../src/utils.c \
    -I ../include -lm

echo "Build completed. Binaries are in ../bin/"
echo -e "export OMP_NUM_THREADS=<number_of_threads>\nmpirun -np <number_of_processes> ./main -n <matrix_size> -m <method> -b <block_size> -d/niente (to print)"