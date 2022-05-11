#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "additional.h"

int main(int argc, char *argv[]){
    setvbuf(stdout, NULL, _IONBF, 0); // Disable output buffering
    int option, C, N;
    char *filePath, *strC, *strN;

    while ((option = getopt(argc, argv, "C:N:F:")) != -1) {
        switch (option) {
            case 'C': // there will be c times consumer threads
                strC = optarg;
                C = atoi(strC);
                break;
            case 'N':
                strN = optarg;
                N = atoi(strN);
                break;
            case 'F':
                filePath = optarg;
                break;
            default:
                write(STDERR_FILENO, "Error with arguments\nUsage: ./hw4 -C 10 -N 5 -F inputfilePath\n", 62);
                exit(EXIT_FAILURE);
        }
    }

    if( C <= 4 || N <= 1 ){
        write(STDERR_FILENO, "Error. C should be bigger than 4, N should be bigger than 1\n", 61);
        exit(EXIT_FAILURE);
    }
    signalHandlerInitializer();
    createSemSet();
    createThreads(C,N, filePath);


    return 0;
}
