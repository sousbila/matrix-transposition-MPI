#ifndef MATRIX_OPERATIONS_H
#define MATRIX_OPERATIONS_H

/*****************************************************************************
 * Serial Methods
 *****************************************************************************/

int checkSym(float *matrix, int n);
void matTranspose(float *matrix, float *transposed, int n);

/*****************************************************************************
 * Block-Based Serial Methods
 *****************************************************************************/

int checkSymBlock(float *matrix, int n, int blockSize);

void matTransposeBlock(float *matrix, float *transposed, int n, int blockSize);

/*****************************************************************************
 * OpenMP Block-Based Methods
 *****************************************************************************/

int checkSymOMP(float *matrix, int n);
void matTransposeOMP(float *matrix, float *transposed, int n);

/*****************************************************************************
 * MPI Methods (Regular)
 *****************************************************************************/

int checkSymMPI(float *matrix, int n);
int checkSymMPI2(float *matrix, int n);
int checkSymMPI3(float *matrix, int n);


void matTransposeMPI(float *matrix, float *transposed, int n);
void matTransposeMPI2(float *matrix, float *transposed, int n);
void matTransposeMPI3(float *matrix, float *transposed, int n);

/*****************************************************************************
 * MPI Methods (Block-Based)
 *****************************************************************************/

int checkSymBlockMPI1(float *matrix, int n);
void matTransposeBlockMPI1(float *matrix, float *transposed, int n);

int checkSymBlockMPI3(float *matrix, int n);
void matTransposeBlockMPI3(float *matrix, float *transposed, int n);

#endif // MATRIX_OPERATIONS_H
