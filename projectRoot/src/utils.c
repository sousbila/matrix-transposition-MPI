#include "utils.h"

void initializeMatrix(float *matrix, int n, int seed) {
    srand(seed > 0 ? seed : 1234);
    for (int i = 0; i < n * n; i++) {
        matrix[i] = (float)rand() / (float)(RAND_MAX / 100.0f);
    }
}

void initializeSymmetricMatrix(float *matrix, int n, int seed) {
    srand(seed);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j <= i; j++) {
            float value = (float)rand() / (float)(RAND_MAX / 100.0f);
            matrix[i * n + j] = value;  // Fill lower triangle
            matrix[j * n + i] = value;  // Mirror to upper triangle
        }
    }
}


void printMatrix(const float *matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%8.2f ", matrix[i * n + j]);
        }
        printf("\n");
    }
    printf("\n");
}

double getWallTime() {
    // For serial or OpenMP timing
    struct timeval time;
    gettimeofday(&time, NULL);
    return (double)time.tv_sec + (double)time.tv_usec * 1.0e-6;
}

/**
 * @brief Computes a partial weighted checksum for selected rows and columns,
 *        ensuring that the checksum of M vs. M^T differs if M is not symmetric.
 *
 * We pick rows = {0, n/2, n-1} and columns = {0, n/2, n-1}, if they exist.
 * For row i, we add (i+1) * matrix[i, j].
 * For column j, we add (j+1) * matrix[i, j].
 *
 * This sums less than the entire matrix, but enough to detect non-symmetry
 * because of the row vs. column weighting.
 *
 * @param matrix Pointer to the n x n matrix
 * @param n      Matrix dimension
 * @return       A partial weighted checksum
 */
float partialChecksum(const float *matrix, int n)
{
    float sum = 0.0f;
    if (n <= 0) return sum;  // Edge case: no size

    // Identify up to 3 selected rows/columns
    int mid = n / 2;
    int selectedRows[3] = {0, mid, n - 1};
    int selectedCols[3] = {0, mid, n - 1};

    // 1) Sum the selected rows, weighting by (i+1)
    for (int r = 0; r < 3; r++) {
        int i = selectedRows[r];
        // Validate index in range
        if (i >= 0 && i < n) {
            float rowWeight = (float)(i + 1);
            for (int j = 0; j < n; j++) {
                sum += rowWeight * matrix[i * n + j];
            }
        }
    }

    // 2) Sum the selected columns, weighting by (j+1)
    for (int c = 0; c < 3; c++) {
        int j = selectedCols[c];
        // Validate index in range
        if (j >= 0 && j < n) {
            float colWeight = (float)(j + 1);
            for (int i = 0; i < n; i++) {
                sum += colWeight * matrix[i * n + j];
            }
        }
    }

    return sum;
}