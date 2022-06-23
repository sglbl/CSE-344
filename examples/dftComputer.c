#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>

// cc dft.c -lm && ./a.out

double radianConverter(double argument){
    return argument * M_PI / 180;
}

int dftComputer(int twoToN, int matrixC[][twoToN], double complex outputMatrix[][twoToN]){
    for(int newRow = 0; newRow < 3; ++newRow){
        for(int newCol = 0; newCol < twoToN; ++newCol){
            double complex dftIndexValue = 0 + 0 * I;
            for(int row = 0; row < 3; ++row){
                for(int col = 0; col < twoToN; ++col){
                    double radian = ( 2 * M_PI * ( (newRow * row)/3.0 + (newCol * col)/(twoToN*1.0) ) );
                    dftIndexValue += ( matrixC[row][col] * ccos(radian)) - I * (matrixC[row][col] * csin(radian) );
                    /* other way */
                    // double complex number = I * (-2 * M_PI * ((newRow * row)/3.0 + (newCol * col) / (twoToN*1.0)));
                    // double complex matrixIndexValue = matrixC[row][col] + 0 * I;
                    // dftIndexValue += ( matrixIndexValue * cexp(number));
                }
            }

            outputMatrix[newRow][newCol] = dftIndexValue;
        }
    }
}

int main(){
    int matrix[3][5] = {
        {1,5,3,2,5},
        {1,5,6,2,4},
        {4,5,2,1,4}
    };

    double complex outPutMatrix[3][5];
    dftComputer(5, matrix, outPutMatrix);
    for(int i = 0; i < 3; ++i){
        for(int j = 0; j < 5; ++j){
            printf("%.1f + %.1fi\t", crealf(outPutMatrix[i][j]), cimagf(outPutMatrix[i][j]));
        }
        printf("\n");
    }

    return 0;
}