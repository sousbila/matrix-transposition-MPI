#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "matrix_operations.h"


int checkSymBlockMPI1(float *matrix, int n)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // --- 1) Broadcast the entire matrix ---
    // Rank 0 already allocated and filled 'matrix'.
    // Other ranks need to allocate memory for the full matrix.
    if (rank != 0) {
        matrix = (float *)malloc(n * n * sizeof(float));
    }

    double t0 = MPI_Wtime();
    MPI_Bcast(matrix, n*n, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // --- 2) Local check for symmetry ---
    // We'll divide rows among processes in a simple contiguous chunk manner:
    int rowsPerProc = n / size;            
    int remainder   = n % size;            // leftover rows if not divisible
    int localStart, localEnd;

    if (rank < remainder) {
        // ranks [0..remainder-1] handle (rowsPerProc + 1) rows each
        localStart = rank * (rowsPerProc + 1);
        localEnd   = localStart + (rowsPerProc + 1);
    } else {
        // ranks [remainder..size-1] handle rowsPerProc each
        localStart = remainder * (rowsPerProc + 1) 
                     + (rank - remainder) * rowsPerProc;
        localEnd   = localStart + rowsPerProc;
    }

    int localSym = 1;
    // For each row i in [localStart, localEnd), check columns j > i
    for (int i = localStart; i < localEnd && localSym; i++) {
        for (int j = i + 1; j < n; j++) {
            if (matrix[i * n + j] != matrix[j * n + i]) {
                localSym = 0;
                break;
            }
        }
    }

    // --- 3) Combine results ---
    int globalSym = 1;
    MPI_Allreduce(&localSym, &globalSym, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    double t1 = MPI_Wtime();

    // Print stats from rank 0
    if (rank == 0) {
        if (globalSym)
            printf("Broadcast MPI: The matrix is symmetric.\n");
        else
            printf("Broadcast MPI: The matrix is NOT symmetric.\n");
        printf("Time taken: %f s\n", (t1 - t0));
    }

    // If rank != 0, we allocated matrix for the broadcast
    if (rank != 0) {
        free(matrix);
    }
    return globalSym;
}



void matTransposeBlockMPI1(float *matrix, float *transposed, int n)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 1) We need p to be a perfect square
    int sqrtP = (int)(sqrt((double)size));
    if (sqrtP * sqrtP != size) {
        if (rank == 0) {
            fprintf(stderr,
                "[Error] The number of processes (%d) is not a perfect square!\n", size);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // 2) Check that n is divisible by sqrtP
    if (n % sqrtP != 0) {
        if (rank == 0) {
            fprintf(stderr,
                "[Error] n=%d not divisible by sqrtP=%d\n", n, sqrtP);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // 3) Each process will have a local block of size blockSize x blockSize
    int blockSize = n / sqrtP;

    // 4) Compute 2D coordinates of this rank in the process grid
    int rowCoord = rank / sqrtP;  // integer division
    int colCoord = rank % sqrtP;

    // 5) Allocate local block of size blockSize x blockSize
    float *localBlock = (float *)malloc(blockSize * blockSize * sizeof(float));
    if (!localBlock) {
        fprintf(stderr, "Rank %d: Could not allocate localBlock\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // 6) Rank 0 sends the appropriate block to each process; or self-copy if rank=0
    if (rank == 0) {
        for (int i = 0; i < sqrtP; i++) {
            for (int j = 0; j < sqrtP; j++) {
                int destRank = i * sqrtP + j;
                float *tempBuf = (float *)malloc(blockSize * blockSize * sizeof(float));
                for (int r = 0; r < blockSize; r++) {
                    for (int c = 0; c < blockSize; c++) {
                        int globalRow = i * blockSize + r;
                        int globalCol = j * blockSize + c;
                        tempBuf[r * blockSize + c] =
                            matrix[globalRow * n + globalCol];
                    }
                }

                if (destRank == 0) {
                    memcpy(localBlock, tempBuf, blockSize * blockSize * sizeof(float));
                } else {
                    MPI_Send(tempBuf, blockSize*blockSize, MPI_FLOAT,
                             destRank, 0, MPI_COMM_WORLD);
                }
                free(tempBuf);
            }
        }
    } 
    else {
        MPI_Recv(localBlock, blockSize*blockSize, MPI_FLOAT,
                 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // 7) Each process transposes its local block in place
    //    localBlock[r,c] => localBlock[c,r]
    //    Easiest is to do an out-of-place transpose if you want clarity
    float *tempTranspose = (float *)malloc(blockSize * blockSize * sizeof(float));
    for (int r = 0; r < blockSize; r++) {
        for (int c = 0; c < blockSize; c++) {
            tempTranspose[c * blockSize + r] = localBlock[r * blockSize + c];
        }
    }
    memcpy(localBlock, tempTranspose, blockSize * blockSize * sizeof(float));
    free(tempTranspose);

    if (rank == 0) {
        for (int i = 0; i < sqrtP; i++) {
            for (int j = 0; j < sqrtP; j++) {
                int sourceRank = i*sqrtP + j;
                int transposeBlockRow = j;  
                int transposeBlockCol = i; 

                if (sourceRank == 0) {
                    for (int r = 0; r < blockSize; r++) {
                        for (int c = 0; c < blockSize; c++) {
                            int globalRow = transposeBlockRow * blockSize + r;
                            int globalCol = transposeBlockCol * blockSize + c;
                            transposed[globalRow * n + globalCol] =
                                localBlock[r * blockSize + c];
                        }
                    }
                } else {
                    float *tempBuf = (float *)malloc(blockSize * blockSize * sizeof(float));
                    MPI_Recv(tempBuf, blockSize*blockSize, MPI_FLOAT,
                             sourceRank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    for (int r = 0; r < blockSize; r++) {
                        for (int c = 0; c < blockSize; c++) {
                            int globalRow = transposeBlockRow*blockSize + r;
                            int globalCol = transposeBlockCol*blockSize + c;
                            transposed[globalRow * n + globalCol] =
                                tempBuf[r * blockSize + c];
                        }
                    }
                    free(tempBuf);
                }
            }
        }
    } 
    else {
        MPI_Send(localBlock, blockSize*blockSize, MPI_FLOAT,
                 0, 1, MPI_COMM_WORLD);
    }

    free(localBlock);
}
