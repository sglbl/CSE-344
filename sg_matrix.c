#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sg_matrix.h"
#include "sg_process_r.h"

// #define CHILD_SIZE 10
// #define COORD_DIMENSIONS 3

double** matrixMultiplicationFor10x3(double **matrix){
    // Creating a 10x10 matrix full of 1 to multiply with.
    double **matrix10x10 = (double**)calloc(CHILD_SIZE, sizeof(double*));
    for(int i = 0; i < CHILD_SIZE; i++){
        matrix10x10[i] = (double*)calloc(CHILD_SIZE, sizeof(double));
        for(int j = 0; j < CHILD_SIZE; j++){
            matrix10x10[i][j] = 1;
        }
    }

    // If we multiply a 10x10 matrix with 10x3 matrix we get a 10x3 matrix.
    double **result = (double**)calloc(CHILD_SIZE, sizeof(double*));
    for(int i = 0; i < CHILD_SIZE; i++){
        result[i] = (double*)calloc(COORD_DIMENSIONS, sizeof(double));
        for(int j = 0; j < COORD_DIMENSIONS; j++){
            result[i][j] = 0;
            for(int k = 0; k < CHILD_SIZE; k++){
                double mult = matrix10x10[i][k] * matrix[k][j];
                result[i][j] += (mult);
            }
        }
    }

    // free temporary 10x10 matrix full of 1.
    for(int i = 0; i < CHILD_SIZE; i++){
        free(matrix10x10[i]);
    }
    free(matrix10x10);

    return result;
}

void divide10x3MatrixTo10(double **matrix){
    for(int i = 0; i < CHILD_SIZE; i++)
        for(int j = 0; j < COORD_DIMENSIONS; j++)
            matrix[i][j] = matrix[i][j] / (CHILD_SIZE*1.0);
}

void substract10x3Matrices(double **matrix1, double **matrix2){
    for(int i = 0; i < CHILD_SIZE; i++)
        for(int j = 0; j < COORD_DIMENSIONS; j++)
            matrix1[i][j] = matrix1[i][j] - matrix2[i][j];
}

double** multiplyWithItsTranspose(double **matrix){
    double **transpose = (double**)calloc(COORD_DIMENSIONS, sizeof(double*));
    for(int i = 0; i < COORD_DIMENSIONS; i++){
        transpose[i] = (double*)calloc(CHILD_SIZE, sizeof(double));
        for(int j = 0; j < CHILD_SIZE; j++)
            transpose[i][j] = matrix[j][i];
    }

    // Multiply matrix with its transpose
    double **result = (double**)calloc(COORD_DIMENSIONS, sizeof(double*));
    for(int i = 0; i < COORD_DIMENSIONS; i++){
        result[i] = (double*)calloc(COORD_DIMENSIONS, sizeof(double));
        for(int j = 0; j < COORD_DIMENSIONS; j++){
            result[i][j] = 0;
            for(int k = 0; k < CHILD_SIZE; k++)
                result[i][j] += (transpose[i][k] * matrix[k][j]);
        }
    }

    // Freeing transpose and matrix
    for(int i = 0; i < COORD_DIMENSIONS; i++)
        free(transpose[i]);
    free(transpose);

    return result;
}

void divide3x3MatrixTo10(double **matrix){
    for(int i = 0; i < COORD_DIMENSIONS; i++)
        for(int j = 0; j < COORD_DIMENSIONS; j++)
            matrix[i][j] = matrix[i][j] / CHILD_SIZE;
}
 // -------------------------
/* 
double mean(double *array, int size){
    int sum = 0;
    for(int i = 0; i < size; i++){
        sum += array[i];
    }
    return (double)(sum / size);
}

double covariance(double *array1, double *array2){
    double sum = 0;
    double mean1 = mean(array1, CHILD_SIZE);
    double mean2 = mean(array2, CHILD_SIZE);
    for(int i = 0; i < CHILD_SIZE; i++)
        sum += ( ( array1[i] - mean1 ) * ( array2[i] - mean2 ) );
    
    return sum / CHILD_SIZE;
}

double** findCovarianceMatrixOfDataset(double **dataset){
    double **covarianceMatrix = (double**)calloc(COORD_DIMENSIONS, sizeof(double*));
    for(int i = 0; i < COORD_DIMENSIONS; i++){
        covarianceMatrix[i] = (double*)calloc(COORD_DIMENSIONS, sizeof(double));
        for(int j = 0; j < COORD_DIMENSIONS; j++){
            covarianceMatrix[i][j] = covariance(dataset[i], dataset[j]);
            printf("%.3f ", covarianceMatrix[i][j]);
        }
        printf("\n");
    }
    return covarianceMatrix;
}

double** findCovarianceMatrix(){
    double **covarianceMatrix = (double**)calloc( CHILD_SIZE, sizeof(double*) );
    for(int i = 0; i < CHILD_SIZE; i++){
        covarianceMatrix[i] = (double*)calloc( COORD_DIMENSIONS, sizeof(double*) );
    }

    for(int j = 0; j < CHILD_SIZE; j++){
        for(int k = 0; k < COORD_DIMENSIONS; k++){
            covarianceMatrix[j][k] = (double)environ[j][k];
            printf("%.3f ", covarianceMatrix[j][k]);
        }
        printf ("\n");
    }
    findCovarianceMatrixOfDataset(covarianceMatrix);

}

double standardDeviationFounder(int *array, int size){
    double sum = 0;
    double meanValue = mean(array, size);
    for(int i = 0; i < size; i++)
        sum += ( ( array[i] - meanValue ) * ( array[i] - meanValue ) );
    
    return sqrt(sum / size);
}
*/