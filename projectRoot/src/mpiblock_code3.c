#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "matrix_operations.h"

int checkSymBlockMPI3(float *matrix, int n)
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


////////////////////////////////////////////////////////////////////////////////////////////

void matTransposeBlockMPI3(float *matrix, float *transposed, int n)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // -------------------------
    // 1) Check sqrt(p) divides n
    // -------------------------
    int sqrtP = (int) sqrt((double) size);
    if (sqrtP * sqrtP != size) {
        if (rank == 0) {
            fprintf(stderr,
                    "[Error] #Processes=%d not a perfect square!\n", size);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (n % sqrtP != 0) {
        if (rank == 0) {
            fprintf(stderr,
                    "[Error] n=%d not divisible by sqrtP=%d!\n", n, sqrtP);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }


    int blockSize = n / sqrtP;

    int rowCoord = rank / sqrtP;  
    int colCoord = rank % sqrtP;

    // local buffer for storing one block (blockSize x blockSize)
    float *localBlock = (float*)malloc(blockSize * blockSize * sizeof(float));
    if (!localBlock) {
        fprintf(stderr, "Rank %d: could not allocate localBlock!\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // ------------------------------------------------------------------
    // 2) Distribution: rank 0 extracts each (i,j) block and sends it out
    // ------------------------------------------------------------------
    if (rank == 0) {
        for (int i = 0; i < sqrtP; i++) {
            for (int j = 0; j < sqrtP; j++) {
                int destRank = i * sqrtP + j;
                float *tempBuf = (float*)malloc(blockSize * blockSize * sizeof(float));
                if (!tempBuf) {
                    fprintf(stderr, "Rank 0: could not allocate tempBuf!\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }

                for (int r = 0; r < blockSize; r++) {
                    int globalRow = i*blockSize + r;
                    memcpy(
                        &tempBuf[r*blockSize],                 // destination
                        &matrix[globalRow * n + (j*blockSize)],// source
                        blockSize * sizeof(float)
                    );
                }

                if (destRank == 0) {
                    memcpy(localBlock, tempBuf, blockSize*blockSize*sizeof(float));
                } else {
                    MPI_Send(tempBuf, blockSize*blockSize, MPI_FLOAT,
                             destRank, 0, MPI_COMM_WORLD);
                }

                free(tempBuf);
            }
        }
    } else {
        MPI_Recv(localBlock, blockSize*blockSize, MPI_FLOAT,
                 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // ------------------------------------------
    // 3) Each rank transposes its local block
    // ------------------------------------------
    float *tmpTranspose = (float*)malloc(blockSize * blockSize * sizeof(float));
    for (int r = 0; r < blockSize; r++) {
        for (int c = 0; c < blockSize; c++) {
            tmpTranspose[c*blockSize + r] = localBlock[r*blockSize + c];
        }
    }
    memcpy(localBlock, tmpTranspose, blockSize*blockSize*sizeof(float));
    free(tmpTranspose);

    // ------------------------------------------------------------------
    // 4) Gathering: rank 0 collects the transposed blocks from each rank
    //               placing them in "transposed" array in the correct spot
    // ------------------------------------------------------------------

    if (rank == 0) {
        for (int i = 0; i < sqrtP; i++) {
            for (int j = 0; j < sqrtP; j++) {
                int sourceRank = i*sqrtP + j;
                if (sourceRank == 0) {
                    for (int r = 0; r < blockSize; r++) {
                        int globalRow = j*blockSize + r;
                        memcpy(
                            &transposed[globalRow * n + (i*blockSize)], // dest
                            &localBlock[r*blockSize],                   // src
                            blockSize * sizeof(float)
                        );
                    }
                } else {
                    float *tempBuf = (float*)malloc(blockSize * blockSize * sizeof(float));
                    MPI_Recv(tempBuf, blockSize*blockSize, MPI_FLOAT,
                             sourceRank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      
                    for (int r = 0; r < blockSize; r++) {
                        int globalRow = j*blockSize + r;
                        memcpy(
                            &transposed[globalRow * n + (i*blockSize)],
                            &tempBuf[r*blockSize],
                            blockSize*sizeof(float)
                        );
                    }
                    free(tempBuf);
                }
            }
        }
    } else {
        MPI_Send(localBlock, blockSize*blockSize, MPI_FLOAT,
                 0, 1, MPI_COMM_WORLD);
    }
    free(localBlock);
}