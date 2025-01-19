#include <omp.h>
#include "matrix_operations.h"

/**
 * @brief Checks if an n x n matrix is symmetric using a block-based approach with OpenMP.
 */
int checkSymOMP(float *matrix, int n)
{
    int blockSize = 64;
    int symmetric = 1;

#pragma omp parallel for default(none) shared(matrix, n, blockSize) reduction(&& : symmetric)
    for (int ii = 0; ii < n; ii += blockSize) {
        for (int jj = ii; jj < n; jj += blockSize) {
            int imax = (ii + blockSize > n) ? n : (ii + blockSize);
            int jmax = (jj + blockSize > n) ? n : (jj + blockSize);

            for (int i = ii; i < imax; i++) {
                if (!symmetric) continue; // short-circuit
                int jStart = (i == ii) ? (i + 1) : jj;
                int limit = jmax - ((jmax - jStart) % 4);

                for (int j = jStart; j < limit; j += 4) {
                    if (matrix[i * n + j]       != matrix[j * n + i]       ||
                        matrix[i * n + (j + 1)] != matrix[(j + 1) * n + i] ||
                        matrix[i * n + (j + 2)] != matrix[(j + 2) * n + i] ||
                        matrix[i * n + (j + 3)] != matrix[(j + 3) * n + i]) {
                        symmetric = 0;
                    }
                }
                // leftover elements
                for (int j = limit; j < jmax; j++) {
                    if (matrix[i * n + j] != matrix[j * n + i]) {
                        symmetric = 0;
                    }
                }
            }
        }
    }
    return symmetric;
}

/**
 * @brief Transposes an n x n matrix using a block-based approach with OpenMP.
 */
void matTransposeOMP(float *matrix, float *transposed, int n)
{
    int blockSize = 64;

#pragma omp parallel for default(none) shared(matrix, transposed, n, blockSize)
    for (int ii = 0; ii < n; ii += blockSize) {
        for (int jj = 0; jj < n; jj += blockSize) {
            int imax = (ii + blockSize > n) ? n : (ii + blockSize);
            int jmax = (jj + blockSize > n) ? n : (jj + blockSize);

            for (int i = ii; i < imax; i++) {
                int limit = jmax - ((jmax - jj) % 4);
                for (int j = jj; j < limit; j += 4) {
                    transposed[j * n + i]         = matrix[i * n + j];
                    transposed[(j + 1) * n + i]   = matrix[i * n + (j + 1)];
                    transposed[(j + 2) * n + i]   = matrix[i * n + (j + 2)];
                    transposed[(j + 3) * n + i]   = matrix[i * n + (j + 3)];
                }
                // leftover columns
                for (int j = limit; j < jmax; j++) {
                    transposed[j * n + i] = matrix[i * n + j];
                }
            }
        }
    }
}
