#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> // For memset of sigaction
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/wait.h> // Wait command
#include <sys/types.h> // Types of signals
#include <signal.h> // Signal handling
#include "sg_process_p.h"

static volatile sig_atomic_t flag = 0; // Flag for signal handling

void printUsageAndExit(){
    write(STDERR_FILENO, "INSTRUCTION: ./processP -i inputFilePath -o outputFilePath\nGoodbye!\n", 69);
    exit(EXIT_FAILURE);
}

void reader(int fileDescriptor, char *argv[], int fileSize){

    int readedByte, status;
    int isFinished = FALSE;

    // Cleaning the output file in the case of it's not empty.
    cleanTheOutputFile(argv);

    write(STDOUT_FILENO, "Process P is reading ", 22);
    write(STDOUT_FILENO, argv[4], sg_strlen(argv[4]) );
    write(STDOUT_FILENO, "\n", 1);

    // Initializing siggnal action for SIGINT signal.
    struct sigaction actionForSigInt;
    memset(&actionForSigInt, 0, sizeof(actionForSigInt)); // Initializing
    actionForSigInt.sa_handler = sg_signalHandler; // Setting the handler function.
    actionForSigInt.sa_flags = 0; // No flag is set.
    sigaction(SIGINT, &actionForSigInt, NULL); // Setting the signal.

    char **buffer = (char**)calloc( fileSize / (CHILD_SIZE*COORD_DIMENSIONS) + 2 , sizeof(char*) );
    for(int i=0; /* Continue until reading all file or error situation */ ; i++){     //Name of the children will be R_i
        // Allocating memory for buffer which will store the content of input file
        buffer[i] = (char*)calloc( CHILD_SIZE * COORD_DIMENSIONS, sizeof(char*) );
        for(int j = 0; j < CHILD_SIZE * COORD_DIMENSIONS; j++){
            if( (readedByte = read(fileDescriptor, &buffer[i][j], 1 /* read 1 byte */)) > 0 ){
                checkIfNonAscii(buffer[i][j]);
                if(flag == 1){
                    killTheKidsAndParent(fileDescriptor, argv);
                }
            }
            else if( readedByte <= 0 && errno == EINTR ){
                perror("File reading error. : ");
                exit(EXIT_FAILURE);
            }
            else{
                isFinished = TRUE;
            }
        }

        // Forking to get ready for creating child process
        pid_t pidCheckIfChild;  // It's for checking if we are in the child process
        
        // Duplicating this process with fork()
        if( isFinished == FALSE && (pidCheckIfChild = fork()) == 0){ 
            // pidCheckForChild is 0 so it's child process.
            spawnChild(argv[4], i, buffer);
        }
        else if( pidCheckIfChild < 0 ){
            perror("Error on fork while creating child process.\n");
            exit(EXIT_FAILURE);
        }
        else{
            // pidCheckForChild is > 0 so it's parent process.
            if(flag == 1)
                killTheKidsAndParent(fileDescriptor, argv);
            if(isFinished == TRUE){
                // Parent process
                if( waitpid(pidCheckIfChild, &status, 0) == -1 ){ // Wait until all children terminate.
                    if(errno != ECHILD){
                        perror("Error on waitpid() command ");
                        exit(EXIT_FAILURE);
                    }
                }
                else{
                    write(STDOUT_FILENO, "All children have done their job!\n", 35);
                    freeingBuffer(buffer, fileSize / (CHILD_SIZE*COORD_DIMENSIONS) + 2 );
                    collectOutputFromChildren(argv[4]);  // argv[4] is the output path
                    return;
                }
            }            
        }

    }   //End of for loop
    
    
}

void spawnChild(char *fileName , int i, char **buffer){
    char iValue = i;
    // write(STDOUT_FILENO, "This is the child.\n", 20);
    char *argList[] = {
        "./processR",
        &iValue,
        "-o",
        fileName, // name of the output file comes from command line argument
        NULL
    };
    
    // Calling other c file to handle the child process.
    execve("./processR", argList, buffer /* environment variables in array */);

    perror("Execve returned so it's an error ");
    exit(EXIT_FAILURE);
}

// Create a signal handler function
void sg_signalHandler(int signalNumber){
    if( signalNumber == SIGINT)
        flag = 1;   // writing to global. If zero make it 1.
}

void killTheKidsAndParent(int fileDescriptor, char *argv[]){
    if (kill(getpid(), SIGINT) == -1) {
        perror("killing child failed.\n");
        _exit(EXIT_FAILURE);
    }

    // Reading is completed. Closing the file.
    if( close(fileDescriptor) == -1 ){   
        perror("Error while closing the file. ");
        exit(EXIT_FAILURE);
    }

    // Removing output file
    if( remove(argv[4]) == -1 ){
        perror("Error while removing file.");
        exit(EXIT_FAILURE);
    }

    write(STDOUT_FILENO, "Terminating.\n", 14);
    _exit(EXIT_SUCCESS); // Child terminated successfully so we can exit
}


void freeingBuffer(char **buffer, int size){
    for(int i=0; i < size; i++)
        free(buffer[i]);
    free(buffer);
}

void collectOutputFromChildren(char *filePath){
    int fileDescOfOutputFile;

    // Opening file in read mode
    if( (fileDescOfOutputFile = open(filePath, O_RDONLY, S_IRGRP)) == -1 ){  
        perror("Error while opening file to read.\n");
        printUsageAndExit();
    }

    // Get size of the file using lseek()
    int fileSize = lseek(fileDescOfOutputFile, 0, SEEK_END);
    lseek(fileDescOfOutputFile, 0, SEEK_SET);
    fileSize /= sizeof(double);

    // // Create an double array to store the output of the children.
    double **output = (double**)calloc( fileSize / 9 , sizeof(double*) );
    for(int i = 0; i < fileSize / 9; i++){
        output[i] = (double*)calloc( 9 , sizeof(double*) );
    }

    int readedByte;
    int ct=0;
    for(int j = 0; j < fileSize / 9; j++){
        for(int k = 0; k < 9; k++){ //Because there are 9 coordiantes in each child process.
            // Reading files/output.dat file with system call with checking return value
            if( ( readedByte = read(fileDescOfOutputFile, &output[j][k] , sizeof(output[j][k])) ) > 0 ){
                ct++;
            }
            else if( readedByte <= 0 && errno == EINTR ){
                perror("File reading error. : ");
                exit(EXIT_FAILURE);
            }
            else{
                // There will be no else because we know exactly the size of file by help of lseek()
            }

        }
    }
    write(STDOUT_FILENO, "Output file reading finished.\n", 31);
    // Reading is completed. Closing the file.
    if( close(fileDescOfOutputFile) == -1 ){   
        perror("Error while closing the file.");
        exit(EXIT_FAILURE);
    }
    
    calcFrobeniusNorm(output, fileSize);

    // Freeing memory.
    for(int i = 0; i < fileSize / 9; i++){
        free(output[i]);
    }
    free(output);

}

void calcFrobeniusNorm(double **output, int fileSize){
    double *sum = (double*)calloc( fileSize / 9 , sizeof(double) ); // 9 because each matrix is 3x3.

    for(int i = 0; i < fileSize / 9 ; i++){
        // Calculating frobenius norm of each child process output.
        for(int j = 0; j < 9; j++)
                sum[i] += pow(output[i][j], 2);
        sum[i] = sqrt(sum[i]);
        // printf("sum[i] is %.3f\n", sum[i]);
    }

    // Comparing the frobenius norm of each child process output to get the closest two.
    double closestDistance = fabs(sum[1] - sum[0]);
    int closestDistanceIndex1 = 0;
    int closestDistanceIndex2 = 0;

    for(int i = 1; i < fileSize / 9; i++){
        for(int j = 0; j < i; j++){
             if( closestDistance > fabs(sum[i] - sum[j]) ){
                closestDistance = fabs(sum[i] - sum[j]);
                closestDistanceIndex1 = i;
                closestDistanceIndex2 = j;
            }
        }
    }

    char *dist1, *dist2, *closestDistanceStr;

    dist1 = itoaForAscii(closestDistanceIndex1 + 1);
    dist2 = itoaForAscii(closestDistanceIndex2 + 1);
    closestDistanceStr = basicFtoa(closestDistance);
    
    write(STDOUT_FILENO ,"The closest 2 matrices are ", 28);
    write(STDOUT_FILENO , dist1 , sg_strlen(dist1) );
    write(STDOUT_FILENO ," and ", 6);
    write(STDOUT_FILENO , dist2 , sg_strlen(dist2) );
    write(STDOUT_FILENO ,", and their distance is ", 25);
    write(STDOUT_FILENO , closestDistanceStr, sg_strlen(closestDistanceStr) );
    write(STDOUT_FILENO ,"\n", 1);

    free(dist1); free(dist2); free(closestDistanceStr);
    free(sum);
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

/* Double to string for basic nonnegative distance values. */
char* basicFtoa(double number){
    if(number > -0.001 && number < 0.001){ // Precision because it's a bad practice to compare double with 0.
        char* string = calloc(2, sizeof(char));
        string[0] = '0';    string[1] = '\0';
        return string;
    }

    int i;
    // Counting digit for integer part of double
    int intDigitCounter = 0;
    int doubleDigitCounter = 0;
    int tempNumber = (int)number;
    while(tempNumber != 0){
        tempNumber /= 10;
        intDigitCounter++;
    }
    if(intDigitCounter == 0)
        intDigitCounter = 1; // Because there is at least 1 digit and that's 0.
    doubleDigitCounter = 3;

    // Number before '.'  to string.
    tempNumber = (int)number;
    char* string = calloc(intDigitCounter + 2 + doubleDigitCounter, sizeof(char) );
    for(i = 0; i < intDigitCounter; i++){
        char temp = (tempNumber % 10) + '0';
        string[intDigitCounter-i-1] = temp;
        tempNumber /= 10;
    }
    string[i++] = '.'; // Adding dot between int and floating point part of the number.

    // Number after '.'  to string.
    tempNumber = (number - (int)number)*1000;
    for(int j = 0; j < doubleDigitCounter; j++){
        char temp = (tempNumber % 10) + '0';
        string[i + doubleDigitCounter - j - 1] = temp;
        tempNumber /= 10;
    }
    string[i + doubleDigitCounter] = '\0';
    return string;
}

char* itoaForAscii(int number){
    if(number == 0){
        char* string = calloc(2, sizeof(char));
        string[0] = '0';    string[1] = '\0';
        return string;
    }
    int digitCounter = 0;
    int temp = number;
    while(temp != 0){
        temp /= 10;
        digitCounter++;
    }
    
    char* string = calloc(digitCounter+1, sizeof(char) );
    for(int i = 0; i < digitCounter; i++){
        char temp = (number % 10) + '0';
        string[digitCounter-i-1] = temp;
        number /= 10;
    }
    string[digitCounter] = '\0';
    return string;
}