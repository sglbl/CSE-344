#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sg_process.h"
#define CHILD_SIZE 10
#define COORD_DIMENSIONS 3


int main(int argc, char *argv[]){
    // Default range for char[signed] (1 byte) -> -128 to 127
    // range for unsigned char        (1 byte) ->   0  to 255

    char *filePath = argv[2]; // Input file path
    struct stat statOfFile;   // Adress of statOfFile will be sent to stat() function in order to get size information of file.
    int fileDesc;             // Directory stream file descriptor for file reading and writing

    if( stat(filePath, &statOfFile) < 0 || argc != 5 ){
        perror("Error while opening file.\n");
        printErrorAndExit();
    }

    // Opening file in read/write mode
    if( (fileDesc = open(filePath, O_RDONLY, S_IWGRP)) == -1 ){
        perror("Error while opening file to read.\n");
        exit(EXIT_FAILURE);
    }

    reader(fileDesc, argv);

    // every 30 bytes will be sent to R_i as 10 coordinates

     // READING IS COMPLETED. CLOSING THE FILE.
    if( close(fileDesc) == -1 ){   
        perror("Error while closing the file.");
        exit(EXIT_FAILURE);
    }

    return 0;
}