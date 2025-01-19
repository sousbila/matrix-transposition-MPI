# Matrix Transposition Performance Analysis

This repository contains the code and scripts used to analyze the performance of various matrix transposition implementations. The project focuses on optimizing the transposition of square matrices (where \( n \) is a power of two) using different techniques: a sequential version, MPI-based explicit parallelism (with multiple variants, including block-based approaches), and an OpenMP-based implementation.

For detailed problem statement and performance analysis, please refer to [reportH2_BS_235252.pdf](reportH2_BS_235252.pdf).  
**NB:** MobaXterm was used to establish an SSH connection from a local Windows system to the university cluster.

## Table of Contents

- [Project Structure]
- [Requirements]
- [Instructions]
  - [Compiling and Running via PBS Script]
  - [Output Files]
  - [Manual Execution]
- [Notes]

## Project Structure

```plaintext
matrix-transposition-MPI-main/
├── projectRoot/
│   ├── mpi.pbs                  # PBS job submission script
│   ├── src/                     # Source files (C code for the different implementations)
│   │   ├── main.c
│   │   ├── serialblock_code.c
│   │   ├── openmp_code.c
│   │   ├── mpi_code.c
│   │   ├── mpi_code2.c
│   │   ├── mpi_code3.c
│   │   ├── mpiblock_code1.c
│   │   ├── mpiblock_code3.c
│   │   └── utils.c
│   ├── include/                 # Header files
│   ├── bin/                     # Compiled executable
│   ├── results/                 # Output CSV and TXT files from the tests
│   └── scripts/                 # Bash scripts for building and running experiments
│       ├── build.sh
│       ├── firstTest.sh
│       ├── secondTest.sh
│       ├── thirdTest.sh
│       ├── fourthTest.sh
│       └── fifthTest.sh
└── README.md
```
## Requirements

- **Compiler:** GCC 9.1.0 (or a compatible version with OpenMP support)
- **MPI:** MPICH 3.2.1 (or similar MPI implementation)
- **Bash:** Unix-like environment with Bash shell
- **dos2unix:** To convert Windows line endings if needed

## Instructions

### Compiling and Running via PBS Script

The provided PBS script `mpi.pbs` (located at the project root) compiles and runs all experiment scripts sequentially. It performs the following steps:
- Loads the required GCC and MPI modules.
- Changes to the `scripts/` directory and fixes the line-endings/permissions for all scripts.
- Invokes `./build.sh` to compile the project.
- Runs the test scripts: `firstTest.sh`, `secondTest.sh`, `thirdTest.sh`, `fourthTest.sh`, and `fifthTest.sh`.

To submit the job from the project root follow these steps:
1. Navigate to the Project Directoy
   ```
   cd matrix-transposition-MPI-main/projectRoot
   ```
   Convert to Unix format the `mpi.pbs` file and ensure it's executable
   ```
   dos2unix mpi.pbs
   chmod +x mpi.pbs
   ```
2. Submit the PBS Job:
   ```
   qsub mpi.pbs
   ```
### Scripts and Output Files

The project contains several scripts that automate the testing of different matrix transposition methods. Below is a summary of each script and the corresponding output files:

- **firstTest.sh**  
  This script compares the MPI row‐based implementations:  
  - `mpi` (baseline broadcast), `mpi2` (column-wise gathers), and `mpi3` (row-scatter with pairwise exchanges).  
  The performance results (execution time) are saved in `../results/results.csv` and a human-readable summary is in `../results/results.txt`.

- **secondTest.sh**  
  This script tests the MPI `mpi3` implementation with 1 process (as serial baseline), with multiple processes (for parallel execution), as well as the `OpenMP` implementation.  
  Output is written to `../results/results2.csv` and `../results/results2.txt`.

- **thirdTest.sh**  
  This script evaluates the MPI block-based implementations. In our current configuration, it compares `mpiblocks1` and `mpiblocks3`.  
  The results are saved in `../results/results3.csv` and `../results/results3.txt`.

- **fourthTest.sh**  
  This script specifically compares the serial block-based implementation (`serialblock`) with the MPI block-based method `mpiblocks3`.  
  The output files are `../results/results4.csv` and `../results/results4.txt`.

- **fifthTest.sh**  
  This script performs additional tests for `mpi3` and `mpiblocks3` using our optimal process counts and varying matrix sizes.  
  Results are recorded in `../results/results5.csv` and `../results/results5.txt`.

Each CSV file contains detailed measurement data (e.g., matrix size, number of processes/threads, average execution time) for further analysis and plotting, while each TXT file provides a summarized, human-readable version of the results.

To ensure proper script execution on the cluster, remember to run `dos2unix` and `chmod +x` on all scripts before submitting the job.

## Manual Execution

If you prefer to run the experiments manually, follow these steps:

### Building the Project
1. **Interactive Session:**  
   Connect to the cluster via MobaXterm and open an interactive session
   ```
   qsub -I -q short_cpuQ -l select=1:ncpus=64:mpiprocs=64:mem=1gb
   ```
   
3. **Compile the Code:**  
   Navigate to the `projectRoot/scripts/` directory. Ensure all scripts have Unix line-endings and executable permissions:
   ```
   cd matrix-transposition/projectRoot/scripts/
   dos2unix build.sh
   chmod +x build.sh
   ./build.sh
   ```

### Running the Project

After building the project with `./build.sh`, you can run the different implementations manually from the command line. 

To run the program manually, set the environment variable `OMP_NUM_THREADS` to the desired number of threads and use `mpirun` with the appropriate number of processes. For example, the command:
```
export OMP_NUM_THREADS=<number_of_threads>
mpirun -np <number_of_processes> ./main -n <matrix_size> -m <method> -d (optional- to print transposed) -c (optional- to print checksum)
```
methods:
`serialblock`, `omp`, `mpi`, `mpi2`, `mpi3`, `mpi_blocks1`, `mpi_blocks3`

This manual run command allows you to directly test the various implementations outside the automated scripts.

***Below are the instructions for each method***:

**Serial Block-Based Implementation:**  
Run the serial version by specifying a matrix size (choose from 16, 32, 64, 128, 256, 512, 1024, 2048, or 4096). For example, to run on a 256×256 matrix:
```bash
mpirun -np 1 ./main -m serialblock -n 256 -c
```
This command executes the serial transposition using a single process. The output file \`transpose_times.csv\` (or similar) will contain the execution times for the serial method.

**MPI Row-Based Implementations (mpi, mpi2, mpi3):**  
For MPI row-based methods, ensure that the matrix size is divisible by the number of processes. For instance, to run the best-performing \`mpi3\` method on a 512×512 matrix with 16 processes:
```bash
mpirun -np 16 ./main -m mpi3 -n 512 -c
```

**MPI Block-Based Implementation (`mpiblocks3`):**  
For the block-based approach, the number of processes must be a perfect square (e.g., 4, 16, or 64), and the matrix size must be divisible by \(\sqrt{p}\). For example, to run `mpiblocks3` on a 1024×1024 matrix with 16 processes:
```bash
mpirun -np 16 ./main -m mpi_blocks3 -n 1024 -c
```

**OpenMP Implementation:**  
For OpenMP, set the environment variable `OMP_NUM_THREADS` to the desired number of threads. For example, to execute on a 512×512 matrix with 4 threads:
```bash
export OMP_NUM_THREADS=4
./main -m omp -n 512 -c
```
Select the number of threads based on the matrix size (e.g., 4 threads for matrices up to 512×512, 16 threads for 1024×1024, and 32 threads for larger sizes for optimal performance). 



By following these commands, you can manually run the different methods and verify the results without using the provided test scripts.


