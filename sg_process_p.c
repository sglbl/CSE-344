#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/wait.h>
#include "sg_process_p.h"

void printUsageAndExit(){
    write(STDERR_FILENO, "INSTRUCTION: ./processP -i inputFilePath -o outputFilePath\nGoodbye!\n", 69);
    exit(EXIT_FAILURE);
}

void reader(int fileDescriptor, char *argv[], int fileSize){
    int readedByte;

    // Cleaning the output file in the case of it's not empty.
    cleanTheOutputFile(argv);

    write(STDOUT_FILENO, "Process P is reading ", 22);
    write(STDOUT_FILENO, argv[4], sg_strlen(argv[4]) );
    write(STDOUT_FILENO, "\n", 1);

    char ***buffer = (char***)calloc( fileSize / (CHILD_SIZE*COORD_DIMENSIONS) + 1 , sizeof(char**) );
    for(int i=0; /* Continue until reading all file or error situation */ ; i++){     //Name of the children will be R_i
        // Allocating memory for buffer which will store the content of input file
        buffer[i] = (char**)calloc( CHILD_SIZE + 1, sizeof(char*) );
        for(int j = 0; j < CHILD_SIZE; j++){

            buffer[i][j] = (char*)calloc( COORD_DIMENSIONS, sizeof(char*) );
            for(int k = 0; k < COORD_DIMENSIONS; k++){
                if( (readedByte = read(fileDescriptor, &buffer[i][j][k], 1 /* read 1 byte */)) > 0 ){
                    checkIfNonAscii(buffer[i][j][k]);
                    // printf("Charbuffer is %c, and int value of it is %d\n", buffer[i][j][k], buffer[i][j][k]);
                }
                else if( readedByte <= 0 && errno == EINTR ){
                    perror("File reading error. : ");
                    exit(EXIT_FAILURE);
                }
                else{
                    buffer[i] = NULL;
                    write(STDOUT_FILENO, "File reading finished.\n", 24);
                    spawn(argv, buffer);    // Spawning children 
                    return;
                }
                
            }
        }

    }   //End of for loop
    
}

void spawn(char** argv, char ***buffer){
    int status;
    pid_t pidCheckIfChild[5];

    // Checking if we are in the child process for every pid in the array.
    for(int i = 0; buffer[i] != NULL; i++){
         // Duplicating this process with fork()
        if( (pidCheckIfChild[i] = fork()) == 0){    // pidCheckForChild is 0 so it's child process.
            char iValue = i+1;
            // write(STDOUT_FILENO, "This is the child.\n", 20);
            char *argList[] = {
                "./processR",
                &iValue,
                "-o",
                argv[4], // name of the output file comes from command line argument
                NULL
            };

            // Calling other c file to handle the child process.
            execve("./processR", argList, buffer[i] /* environment variables in array */);

            perror("Execve returned so it's an error ");
            exit(EXIT_FAILURE);
        }
        if( pidCheckIfChild[i] < 0){
            perror("Error on fork while creating child process.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Checking if we are in the parent process for every pid in the array.
    for(int i = 0; buffer[i] != NULL; i++){
        if( pidCheckIfChild[i] != 0){  // pidCheckForChild is not 0, so it's parent process.
            // write(STDOUT_FILENO, "This is the parent.\n", 21);
            
            if( waitpid(pidCheckIfChild[i], &status, 0) == -1 ){ // Wait until all children terminate.
                if(errno != ECHILD){
                    perror("Error on wait() command ");
                    exit(EXIT_FAILURE);
                }
                else{
                    write(STDOUT_FILENO, "All children are terminated!\n", 30);
                    exit(EXIT_SUCCESS);
                }
            }
            
        }
    }
    write(STDOUT_FILENO, "Reached EOF, collecting outputs from ", 39);
    write(STDOUT_FILENO, argv[4], sg_strlen(argv[4]) );
    write(STDOUT_FILENO, "\n", 1);

    // Collecting outputs from children.
    collectOutputFromChildren(argv[4]);  // argv[4] is the output path


}

void collectOutputFromChildren(char *filePath){

    double *value = (double*)malloc(sizeof(double));
    int fileDescOfOutputFile;

    // Opening file in read mode
    if( (fileDescOfOutputFile = open(filePath, O_RDONLY, S_IRGRP)) == -1 ){  
        perror("Error while opening file to read.\n");
        printUsageAndExit();
    }

    int readedByte;
    for(int i=0; /* Con*/ ; i++){
        for(int j = 0; j < CHILD_SIZE; j++){
            for(int k = 0; k < COORD_DIMENSIONS; k++){
                // Reading files/output.dat file with system call with checking return value
                if( (readedByte = read(fileDescOfOutputFile, value , sizeof(double*))) > 0 ){
                    // printf("Value is %f\n", *value);
                    // printf("Charbuffer is %c, and int value of it is %d\n", buffer[i][j][k], buffer[i][j][k]);
                }
                else if( readedByte <= 0 && errno == EINTR ){
                    perror("File reading error. : ");
                    exit(EXIT_FAILURE);
                }
                else{
                    write(STDOUT_FILENO, "Output file reading finished.\n", 31);
                    return;
                }


            }
        }
    }

    // Reading is completed. Closing the file.
    if( close(fileDescOfOutputFile) == -1 ){   
        perror("Error while closing the file.");
        exit(EXIT_FAILURE);
    }
}

void cleanTheOutputFile(char *argv[]){
    // Clean output file and if doesn't exist create new one.
    int fileDesc;
    if( (fileDesc = open(argv[4], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1 ){
        perror("Error while opening file to write.\n");
        printUsageAndExit();
    }

    // Now our output file is empty. (And if doesn't exist it was created.)
    if( close(fileDesc) == -1 ){   
        perror("Error while closing the file. ");
        exit(EXIT_FAILURE);
    }

}

void checkIfNonAscii(char character){
    if( character < 0 || character > 255){  // Assuming extended ascii by the example in the homework assignment pdf.
        write(STDOUT_FILENO, "File contains non-ASCII character. Please provide an ascii file.\n", 66);
        exit(EXIT_FAILURE);
    }
}

int sg_strlen(char* string){
    int counter = 0;
    while( string[counter] != '\0' )
        counter++;
    return counter;
}
