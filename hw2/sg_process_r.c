#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h> // Types of signals
#include <string.h> // For memset() function for the locking mechanism
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include "sg_process_r.h"
#include "sg_matrix.h"

static volatile sig_atomic_t flag = 0; // Flag for signal handling.
extern char **environ;  // Environment variables that will be passed to child process and will come from "sg_process_p.c"

int main(int argc, char *argv[]){

    // Initializing siggnal action for SIGINT signal.
    struct sigaction actionForSigInt;
    memset(&actionForSigInt, 0, sizeof(actionForSigInt)); // Initializing
    actionForSigInt.sa_handler = sg_signalHandler; // Setting the handler function.
    actionForSigInt.sa_flags = 0; // No flag is set.
    sigaction(SIGINT, &actionForSigInt, NULL); // Setting the signal.

    int i = atoi(argv[1]);       // Special number of this child.
    char *filePath = argv[3]; // Output file path
    int fileDesc;             // Directory stream file descriptor for file writing
    struct flock lock;        // Lock structure of the file.

    // Opening file in write mode
    if( (fileDesc = open(filePath, O_WRONLY | O_APPEND, S_IWUSR | S_IWGRP | S_IWOTH)) == -1 ){
        // if flag is 1 then succesfully caaught
        exit(EXIT_SUCCESS);
    }

    // Locking
    memset(&lock, 0, sizeof(lock)); //Initing structure of lock to 0.
    lock.l_type = F_WRLCK;  // F_WRLCK: Field of the structure of type flock for write lock.
    if( fcntl(fileDesc, F_SETLKW, &lock) == -1 ){ // putting write lock on file. 
        // F_SETLKW: If a signal is caught while waiting, then the call is interrupted and (after signal handler returned) returns immediately.
        if(flag == 1){
            // No need to unlock because lock is already didn't work.
            // Closing file
            if( close(fileDesc) == -1 ){   
                perror("Error while closing the file.");
                _exit(EXIT_FAILURE);
            }
            // write(STDOUT_FILENO, "I am a child and I am killing myself\n", 40);
            _exit(EXIT_SUCCESS);
        }
        perror("Error while locking fcntl(F_SETLK mode).\n");
        exit(EXIT_FAILURE);
    }
    
    // Printing child info
    printChildInfo(i);
    double **covarianceMatrix = findCovarianceMatrix(i, fileDesc, lock);

    // Writing to file
    writeToFile(fileDesc, covarianceMatrix);

    for(int i = 0; i < COORD_DIMENSIONS; i++)
            free(covarianceMatrix[i]);
    free(covarianceMatrix);

    // Unlocking
    lock.l_type = F_UNLCK;
    if ( fcntl(fileDesc, F_SETLKW, &lock) == -1) {
        perror("Error while unlocking with fcntl(F_SETLKW)");
        exit(EXIT_FAILURE);
    }

    // Closing file
    if( close(fileDesc) == -1 ){   
        perror("Error while closing the file.");
        exit(EXIT_FAILURE);
    }

    sleep(1);
    if(flag == 1){
        // write(STDOUT_FILENO, "I am a child and I am killing myself\n", 40);
        _exit(EXIT_SUCCESS); // Child terminated successfully so we can exit
    }
    return 0;
}

// Create a signal handler function
void sg_signalHandler(int signalNumber){
    if( signalNumber == SIGINT)
        flag = 1;   // writing to global. If zero make it 1.
}

double** findCovarianceMatrix(int i, int fileDesc, struct flock lock){
    // Formula:        a = A – 1*A*( 1 / n )
    // Covariance matrix = a‘ * a / n
    // Reference: https://stattrek.com/matrix-algebra/covariance-matrix.aspx

    double **dataset = (double**)calloc( CHILD_SIZE, sizeof(double*) );
    for(int j = 0; j < CHILD_SIZE; j++){
        dataset[j] = (double*)calloc( COORD_DIMENSIONS, sizeof(double*) );
        for(int k = 0; k < COORD_DIMENSIONS; k++){
            dataset[j][k] = (double)environ[i][j*COORD_DIMENSIONS+ k];
            if(flag == 1){
                for(int j = 0; j < CHILD_SIZE; j++)
                    free(dataset[j]);
                free(dataset);
                // Unlocking
                lock.l_type = F_UNLCK;
                if ( fcntl(fileDesc, F_SETLKW, &lock) == -1) {
                    perror("Error while unlocking with fcntl(F_SETLKW)");
                    _exit(EXIT_FAILURE);
                }
                // Closing file
                if( close(fileDesc) == -1 ){   
                    perror("Error while closing the file.");
                    _exit(EXIT_FAILURE);
                }
                // write(STDOUT_FILENO, "I am a child and I am killing myself\n", 40);
                _exit(EXIT_SUCCESS); // Child terminated successfully so we can exit
            }
        }
    }
    
    double **tempMatrix = matrixMultiplicationFor10x3(dataset);
    divide10x3MatrixTo10(tempMatrix);
    substract10x3Matrices(dataset, tempMatrix);

    // Interruption signal handling
    if(flag == 1){
        for(int j = 0; j < CHILD_SIZE; j++){
            free(tempMatrix[j]);
            free(dataset[j]);
        }
        free(tempMatrix);   free(dataset);
        // Unlocking
        lock.l_type = F_UNLCK;
        if ( fcntl(fileDesc, F_SETLKW, &lock) == -1) {
            perror("Error while unlocking with fcntl(F_SETLKW)");
            _exit(EXIT_FAILURE);
        }
        // Closing file
        if( close(fileDesc) == -1 ){   
            perror("Error while closing the file.");
            _exit(EXIT_FAILURE);
        }
        // write(STDOUT_FILENO, "I am a child and I am killing myself\n", 38);
        _exit(EXIT_SUCCESS); // Child terminated successfully so we can exit
        
    }

    double **covarianceMatrix;
    covarianceMatrix = multiplyWithItsTranspose(dataset);
    divide3x3MatrixTo10(covarianceMatrix);

    // Freeing tempMatrix because not needed anymore.
    for(int j = 0; j < CHILD_SIZE; j++){
        free(tempMatrix[j]);
    }
    free(tempMatrix);

    // Freeing dataset because not needed anymore.
    for(int j = 0; j < CHILD_SIZE; j++){
        free(dataset[j]);
    }
    free(dataset);

    return covarianceMatrix;
}

void writeToFile(int fileDesc, double **covarianceMatrix){
    // Size of covariance matrix is 3x3. 
    for(int j = 0; j < COORD_DIMENSIONS; j++){
        for(int k = 0; k < COORD_DIMENSIONS; k++){
            //Writing as binary.
            while( write(fileDesc, &covarianceMatrix[j][k], sizeof(covarianceMatrix[j][k]) ) == -1 && errno == EINTR ){}
        }
    }
}

int sg_strlen(char* string){
    int counter = 0;
    while( string[counter] != '\0' )
        counter++;
    return counter;
}

/* Printing child information */
void printChildInfo(int i){
    // Because of printf and snprintf are not signal safe, I used write(). 
    // For formatting from int to string I used itaaForAscii(int) function.

    printf("Created R_%d with (%d,%d,%d), (%d,%d,%d), ..., (%d,%d,%d)\n" ,i+1, environ[i][0], environ[i][1], environ[i][2],
        environ[i][3], environ[i][4], environ[i][5], environ[i][CHILD_SIZE*COORD_DIMENSIONS-3], environ[i][CHILD_SIZE*COORD_DIMENSIONS-2], environ[i][CHILD_SIZE*COORD_DIMENSIONS-1] );

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
    
    char* string = calloc((digitCounter+1), sizeof(char));
    for(int i = 0; i < digitCounter; i++){
        char temp = (number % 10) + '0';
        string[digitCounter-i-1] = temp;
        number /= 10;
    }
    string[digitCounter] = '\0';
    
    return string;
}
