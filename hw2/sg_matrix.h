#ifndef SGMATRIX_H_
#define SGMATRIX_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#define TRUE 1
#define FALSE 0
#define CHILD_SIZE 10
#define COORD_DIMENSIONS 3


double** matrixMultiplicationFor10x3(double **matrix);

void divide10x3MatrixTo10(double **matrix);

void substract10x3Matrices(double **matrix1, double **matrix2);

double** multiplyWithItsTranspose(double **matrix);

void divide3x3MatrixTo10(double **matrix);

// double** findCovarianceMatrixOfDataset(double **dataset, int size);

#endif