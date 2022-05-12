#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
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

    struct stat statOfFile;  //Adress of statOfFile will be sent to stat() function in order to get size information of file.
    if(stat(filePath, &statOfFile) < 0){
        write(STDERR_FILENO, "Error while opening the file.\n",30);
        exit(EXIT_FAILURE);
    }
    if(statOfFile.st_size != 2*C*N && statOfFile.st_size != 2*C*N-1){
        printf("Error Size of file is %ld doesn't match 2*C*N %d\n", statOfFile.st_size, C*N);
        write(STDERR_FILENO, "Size of file should be equal to 2 times C*N.(One for '1', one for '2')\nPlease put a valid file.\n", 96);
        exit(EXIT_FAILURE);
    }

    signalHandlerInitializer();
    createSemSet();
    createThreads(C,N, filePath);


    return 0;
}
