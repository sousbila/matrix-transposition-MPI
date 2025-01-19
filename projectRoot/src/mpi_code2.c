#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "matrix_operations.h"


int checkSymMPI2(float *matrix, int n)
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



/*
  Row-scatter + Column-gather MPI transpose.
 
         1) Scatter rows from rank 0 to each rank, so rank r gets (n/size) rows.
         2) For each column c, gather partial columns from all ranks
            and assemble them into row c of the transposed matrix on rank 0.
 
         This avoids broadcasting the entire matrix. Each non-zero rank
         only holds (n/size)*n floats from M at a time.
 
 matrix     [IN]  On rank 0, the full n*n input. NULL on other ranks.
 transposed [OUT] On rank 0, the full n*n transposed. NULL on others.
 n          The dimension of the matrix (n x n).
 */
void matTransposeMPI2(float *matrix, float *transposed, int n)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // We assume n % size == 0 so that each rank gets exactly n/size rows
    int localRows = n / size;

    // ----------------------------------------------------------------
    // 1) Scatter rows of M from rank 0 to each process
    //    => localM has (localRows x n) floats (row-major)
    // ----------------------------------------------------------------
    float *localM = (float *)malloc(localRows * n * sizeof(float));
    if (!localM) {
        fprintf(stderr, "Rank %d: Could not allocate localM\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Scatter(
         matrix,         
         localRows * n,  
         MPI_FLOAT,
         localM,
         localRows * n,
         MPI_FLOAT,
         0,
         MPI_COMM_WORLD
    );

    // ----------------------------------------------------------------
    // 2) Column-by-column gather approach:
    //    For each column c in [0..n-1]:
    //      - Each rank extracts the relevant entries from localM[]
    //      - MPI_Gather them back to rank 0
    //      - On rank 0, place them into row c of 'transposed'
    // ----------------------------------------------------------------

    // A small buffer on each rank to hold the column portion of size localRows
    float *sendCol = (float *)malloc(localRows * sizeof(float));
    // On rank 0, we need to gather from all ranks => a buffer of size n
    float *recvCol = NULL;
    if (rank == 0) {
        recvCol = (float *)malloc(n * sizeof(float));
        if (!transposed) {
            fprintf(stderr, "Rank 0: 'transposed' is NULL\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    for (int c = 0; c < n; c++)
    {
        // (a) On each rank, copy column c from localM into 'sendCol'
        //     localM has localRows rows x n cols
        //     localM[r*n + c] is element (r,c) of this local chunk
        for (int r = 0; r < localRows; r++) {
            sendCol[r] = localM[r*n + c];
        }

        // (b) Gather these partial columns on rank 0
        //     => each rank sends 'sendCol' (localRows floats)
        //        rank 0 receives them into 'recvCol' (n floats)
        MPI_Gather(
             sendCol,
             localRows,
             MPI_FLOAT,
             recvCol,   
             localRows,
             MPI_FLOAT,
             0,
             MPI_COMM_WORLD
        );

        // (c) On rank 0, place the gathered column into row c of 'transposed'
        if (rank == 0) {
            // 'recvCol' has n floats => each float is localRows portion from a rank
            // The final transposed row index = c, so transposed[c, row] in row-major:
            //   transposed[c*n + row]
            // But we just fill row from [0..n-1]
            for (int rowGlobal = 0; rowGlobal < n; rowGlobal++) {
                // T[c, rowGlobal] = M[rowGlobal, c]
                // stored as transposed[c*n + rowGlobal]
                transposed[c*n + rowGlobal] = recvCol[rowGlobal];
            }
        }
    }


    free(sendCol);
    free(localM);

    if (rank == 0) {
        free(recvCol);
    }
}
