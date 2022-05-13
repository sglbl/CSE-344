#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

    // çç
    // struct stat statOfFile;  //Adress of statOfFile will be sent to stat() function in order to get size information of file.
    // if(stat(filePath, &statOfFile) < 0){
    //     write(STDERR_FILENO, "Error while opening the file.\n",30);
    //     exit(EXIT_FAILURE);
    // }
    // if(statOfFile.st_size != 2*C*N && statOfFile.st_size != 2*C*N-1 && statOfFile.st_size != 2*C*N+1){
    //     printf("Error Size of file is %ld doesn't match 2*C*N %d\n", statOfFile.st_size, C*N);
    //     write(STDERR_FILENO, "Size of file should be equal to 2 times C*N.(One for '1', one for '2')\nPlease put a valid file.\n", 96);
    //     exit(EXIT_FAILURE);
    // }

    signalHandlerInitializer();
    int *fileDescs = openFiles(filePath1, filePath2, outputPath);
    createThreads(n,m, fileDescs);

    return 0;
}
