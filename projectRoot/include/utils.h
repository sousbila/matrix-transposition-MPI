#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>


void initializeMatrix(float *matrix, int n, int seed);
void initializeSymmetricMatrix(float *matrix, int n, int seed);

void printMatrix(const float *matrix, int n);

float partialChecksum(const float *matrix, int n);

#endif // UTILS_H
