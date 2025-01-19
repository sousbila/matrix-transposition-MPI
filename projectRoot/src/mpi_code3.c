#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "matrix_operations.h"

int checkSymMPI3(float *matrix, int n)
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



void matTransposeMPI3(float *matrix, float *transposed, int n)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // We assume n is divisible by size
    int localRows = n / size;

    // -------------------------------------------------
    // 1) Scatter original matrix M to all ranks
    //    Each rank gets localRows*n floats
    // -------------------------------------------------
    float *localM = (float *)malloc(localRows * n * sizeof(float));
    if (rank == 0 && matrix == NULL) {
        fprintf(stderr,"matTransposeMPI: rank=0 has a null matrix pointer!\n");
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

    // -------------------------------------------------
    // 2) Allocate space for local portion of T
    //    store rows [rank*localRows .. (rank+1)*localRows) of T
    //    => also localRows*n floats
    // -------------------------------------------------
    float *localT = (float *)calloc(localRows * n, sizeof(float));
    if (!localT) {
        fprintf(stderr, "Rank %d: Could not allocate localT\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // exchange sub-blocks of dimension localRows x localRows
    // with each other rank 'k'. 
    float *sendBuf = (float *)malloc(localRows * localRows * sizeof(float));
    float *recvBuf = (float *)malloc(localRows * localRows * sizeof(float));
    if (!sendBuf || !recvBuf) {
        fprintf(stderr, "Rank %d: Could not allocate sendBuf/recvBuf\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // -------------------------------------------------
    // 3) Pairwise exchange sub-blocks
    // -------------------------------------------------
    for (int k = 0; k < size; k++)
    {
        if (k == rank)
        {
            for (int r = 0; r < localRows; r++) {
                for (int c = 0; c < localRows; c++) {

                    int globalCol = rank * localRows + c;
                    float val = localM[r * n + globalCol];
                    
                    float transVal = localM[c * n + (rank*localRows + r)];

                    localT[r * n + globalCol] = transVal;
                }
            }
        }
        else
        {

            int colStart = k * localRows;
            for (int r = 0; r < localRows; r++) {
                for (int c = 0; c < localRows; c++) {
                    int globalCol = colStart + c;
                    sendBuf[r*localRows + c] = localM[r*n + globalCol];
                }
            }

            MPI_Request req[2];
            MPI_Status  stat[2];

            int tag = 999;
            MPI_Isend(sendBuf, localRows*localRows, MPI_FLOAT, k, tag, MPI_COMM_WORLD, &req[0]);
            MPI_Irecv(recvBuf, localRows*localRows, MPI_FLOAT, k, tag, MPI_COMM_WORLD, &req[1]);

            // Wait for completion
            MPI_Waitall(2, req, stat);



            // Off-diagonal block exchange
            for (int r = 0; r < localRows; r++) {
              for (int c = 0; c < localRows; c++) {
                  float val = recvBuf[r*localRows + c];
          
                  // Global indices in T:
                  //   T[ rank*localRows + c,   k*localRows + r ] = val
                  int globalRow = rank * localRows + c; 
                  int globalCol = k    * localRows + r;
          
                  // Convert globalRow -> localRow for localT
                  int localRow = c; 
                  // The column in localT is the same as globalCol
                  localT[ localRow*n + globalCol ] = val;
              }
            }

        }
    }

    free(sendBuf);
    free(recvBuf);

    MPI_Gather(
         localT,
         localRows * n,
         MPI_FLOAT,
         transposed,  
         localRows * n,
         MPI_FLOAT,
         0,
         MPI_COMM_WORLD
    );

    free(localM);
    free(localT);
}