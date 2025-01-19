#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>

#include "utils.h"
#include "matrix_operations.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);  // Initialize MPI

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Default parameters
    int n = 4096;
    int blockSize = 64;
    char method[50] = "serialblock";  // serial, serialblock, omp, mpi, mpi_blocks
    int display = 0;             // Whether to print the final transposed matrix
    int doChecksum = 1;          // Whether to compute partial checksums

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            n = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            blockSize = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            strncpy(method, argv[++i], 49);
        } else if (strcmp(argv[i], "-d") == 0) {
            display = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            doChecksum = 1;
        }
    }

    // Allocate memory (rank 0 for MPI has the full matrix)
    float *matrix     = NULL;
    float *transposed = NULL;
    if (rank == 0) {
        matrix     = (float *)malloc(n * n * sizeof(float));
        transposed = (float *)malloc(n * n * sizeof(float));
        initializeMatrix(matrix, n, 321); // fixed seed
        //initializeSymmetricMatrix(matrix, n, 123); // symmetric matrix
    }

    // 1) Check if the matrix is symmetric BEFORE transposition
    double symStart = MPI_Wtime();  
    int isSymBefore = -1; // valid on rank 0 or distributed result

    // Decide which check method to call
    if (strcmp(method, "serialblock") == 0 && rank == 0) {
        isSymBefore = checkSymBlock(matrix, n, blockSize);
    } 
    else if (strcmp(method, "omp") == 0 && rank == 0) {
        isSymBefore = checkSymOMP(matrix, n);
    } 
    else if (strcmp(method, "mpi") == 0) {
        isSymBefore = checkSymMPI(matrix, n);
    } 
    else if (strcmp(method, "mpi2") == 0) {
        isSymBefore = checkSymMPI2(matrix, n);
    } 
    else if (strcmp(method, "mpi3") == 0) {
        isSymBefore = checkSymMPI3(matrix, n);
    }
    else if (strcmp(method, "mpi_blocks1") == 0) {
        //if (rank == 0) {printf("Entrato check sym");}
        isSymBefore = checkSymBlockMPI1(matrix, n);
        //if (rank == 0) {printf("Uscito check sym");}
    }
    else if (strcmp(method, "mpi_blocks3") == 0) {
        //if (rank == 0) {printf("Entrato check sym");}
        isSymBefore = checkSymBlockMPI3(matrix, n);
        //if (rank == 0) {printf("Uscito check sym");}
    }
    double symEnd = MPI_Wtime();
    double symTimeBefore = symEnd - symStart;

    // 2) compute partial checksum of original matrix
    float originalCheck = 0.0f;
    if (doChecksum && rank == 0) {
        originalCheck = partialChecksum(matrix, n);
    }

    // 3) Transpose timing
    double transposeTime = 0.0;  

    // If the matrix is symmetric, skip transposition
    if (isSymBefore == 0) {

        double transposeStart = MPI_Wtime();

        if (strcmp(method, "serialblock") == 0) {
            if (rank == 0) {
                matTransposeBlock(matrix, transposed, n, blockSize);
            }
        } 
        else if (strcmp(method, "omp") == 0) {
            if (rank == 0) {
                matTransposeOMP(matrix, transposed, n);
            }
        } 
        else if (strcmp(method, "mpi") == 0) {
            matTransposeMPI(matrix, transposed, n);
        } 
        else if (strcmp(method, "mpi2") == 0) {
            matTransposeMPI2(matrix, transposed, n); 
        }
        else if (strcmp(method, "mpi3") == 0) {
            matTransposeMPI3(matrix, transposed, n);
        }
        else if (strcmp(method, "mpi_blocks1") == 0) {
            matTransposeBlockMPI1(matrix, transposed, n); 
        }
        else if (strcmp(method, "mpi_blocks3") == 0) {
            matTransposeBlockMPI3(matrix, transposed, n); 
        }  
        else {
            if (rank == 0) {
                fprintf(stderr, "Unknown method '%s'.\n", method);
            }
        }

        double transposeEnd = MPI_Wtime();
        transposeTime = transposeEnd - transposeStart;
    }
    else {
        // If it's already symmetric, skip transpose
        if (rank == 0) {
            printf("Matrix is already symmetric, skipping transpose.\n");
        }
    }

    // 4) If transposition happened, compute checksum of transposed
    float transposedCheck = 0.0f;
    if (doChecksum && rank == 0 && isSymBefore == 0) {
        transposedCheck = partialChecksum(transposed, n);
    }

    // 5) rank 0 prints results
    if (rank == 0) {
        //printf("\n[Method: %s]\n", method);

        printf("   Symmetry check: %.6f s\n", symTimeBefore);

        if (isSymBefore == 0) {
            // Only relevant if we actually did the transpose
            printf("   Transpose time: %.6f s\n", transposeTime);
        }

        // Print checksums if requested
        if (doChecksum) {
            printf("   Partial checksum (original)   = %f\n", originalCheck);
            if (isSymBefore == 0) {
                printf("   Partial checksum (transposed) = %f\n", transposedCheck);
            }
        }

        // Print matrices if -d was specified
        if (display) {
            //printf("Original matrix:\n");
            //printMatrix(matrix, n);

            if (isSymBefore == 0) {
                // We only have a transposed matrix if we actually transposed
                printf("Transposed matrix:\n");
                printMatrix(transposed, n);
            }
        }
        free(matrix);
        free(transposed);
    }

    MPI_Finalize();
    return 0;
}
