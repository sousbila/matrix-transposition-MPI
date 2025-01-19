#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "matrix_operations.h"


int checkSymMPI(float *matrix, int n)
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



void matTransposeMPI(float *matrix, float *transposed, int n)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // -------------------------------------------------
    // 1) Broadcast the entire matrix M to all ranks.
    //    We assume memory can handle it on each rank.
    // -------------------------------------------------
    if (rank != 0) {
        // Non-root ranks allocate space for the entire matrix
        matrix = (float *)malloc(n * n * sizeof(float));
    }
    MPI_Bcast(matrix, n*n, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // -------------------------------------------------
    // 2) Since n % size == 0, each rank gets exactly n/size rows
    // -------------------------------------------------
    int localRows = n / size;
    int localStart = rank * localRows;
    int localEnd   = localStart + localRows;

    // -------------------------------------------------
    // 3) Allocate a local buffer for our portion of transposed
    //    Each rank computes localRows of T, each row has n columns
    // -------------------------------------------------
    float *localTransposed = (float *)malloc(localRows * n * sizeof(float));

    // -------------------------------------------------
    // 4) Compute the local portion of the transpose
    //    T[i,j] = M[j,i], for i in [localStart, localEnd)
    // -------------------------------------------------
    for (int i = localStart; i < localEnd; i++) {
        for (int j = 0; j < n; j++) {
            localTransposed[(i - localStart) * n + j] = matrix[j * n + i];
        }
    }

    // -------------------------------------------------
    // 5) Gather all partial transposed blocks on rank 0
    //    Each rank has localRows*n elements
    // -------------------------------------------------
    int localCount = localRows * n;

    MPI_Gather(
        localTransposed,
        localCount,
        MPI_FLOAT,
        transposed,   
        localCount,
        MPI_FLOAT,
        0,
        MPI_COMM_WORLD
    );

    free(localTransposed);
    if (rank != 0) {
        free(matrix);
    }
}