#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include "additional.h"

int main(int argc, char *argv[]){
    setvbuf(stdout, NULL, _IONBF, 0); // Disable output buffering
    int option, n, m;
    char *filePath1, *filePath2, *outputPath;

    while ((option = getopt(argc, argv, "i:j:o:n:m:")) != -1) {
        switch (option) {
            case 'i': // there will be c times consumer threads
                filePath1 = optarg;
                break;
            case 'j':
                filePath2 = optarg;
                break;
            case 'o':
                outputPath = optarg;
                break;
            case 'n':
                n = atoi(optarg);
                break;
            case 'm':
                m = atoi(optarg);
                break;
            default:
                write(STDERR_FILENO, "Error with arguments\nUsage: ./hw4 -C 10 -N 5 -F inputfilePath\n", 62);
                exit(EXIT_FAILURE);
        }
    }

    if( n <= 2 || m < 2 ){
        write(STDERR_FILENO, "Error. It should be n > 2, m >= 2\n", 34);
        exit(EXIT_FAILURE);
    }

    // File size checking with stat function.
    struct stat statOfFile[2];  //Adress of statOfFile will be sent to stat() function in order to get size information of file.
    if(stat(filePath1, &statOfFile[0]) < 0){
        write(STDERR_FILENO, "Error while opening the file.\n",30); exit(EXIT_FAILURE);
    }
    if(stat(filePath2, &statOfFile[1]) < 0){
        write(STDERR_FILENO, "Error while opening the file.\n",30); exit(EXIT_FAILURE);
    }
    if( statOfFile[0].st_size < pow(2,n)*pow(2,n) || statOfFile[1].st_size < pow(2,n)*pow(2,n) ){
        write(STDERR_FILENO,"Error. Size of file is less than (2^n)*(2^n)\n", 45);
        exit(EXIT_FAILURE);
    }

    signalHandlerInitializer();
    int *fileDescs = openFiles(filePath1, filePath2, outputPath);

    int twoToN = pow(2,n);
    int matrixA[twoToN][twoToN], matrixB[twoToN][twoToN];
    readMatrices(n, m, twoToN, fileDescs, matrixA, matrixB);

    createThreads(twoToN, matrixA, matrixB);

    free(fileDescs);

    return 0;
}
