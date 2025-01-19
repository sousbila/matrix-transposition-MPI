#include "matrix_operations.h"

int checkSymBlock(float *matrix, int n, int blockSize)
{
    for (int ii = 0; ii < n; ii += blockSize) {
        for (int jj = ii; jj < n; jj += blockSize) {
            int imax = (ii + blockSize > n) ? n : (ii + blockSize);
            int jmax = (jj + blockSize > n) ? n : (jj + blockSize);

            for (int i = ii; i < imax; i++) {
                // Only need to compare part above diagonal
                int jStart = (i == ii) ? i + 1 : jj;
                for (int j = jStart; j < jmax; j++) {
                    if (matrix[i * n + j] != matrix[j * n + i]) {
                        return 0;
                    }
                }
            }
        }
    }
    return 1;
}

void matTransposeBlock(float *matrix, float *transposed, int n, int blockSize)
{
    for (int ii = 0; ii < n; ii += blockSize) {
        for (int jj = 0; jj < n; jj += blockSize) {
            int imax = (ii + blockSize > n) ? n : (ii + blockSize);
            int jmax = (jj + blockSize > n) ? n : (jj + blockSize);

            for (int i = ii; i < imax; i++) {
                for (int j = jj; j < jmax; j++) {
                    transposed[j * n + i] = matrix[i * n + j];
                }
            }
        }
    }
}
