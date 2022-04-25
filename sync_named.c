#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/wait.h> // Wait command
#include <sys/stat.h> // Stat command
#include <sys/mman.h> // Memory mapping
#include <semaphore.h> // Semaphore
#include "prerequisites.h"
#include "sync_named.h"

// static char (*ingredients)[2]; // Array of ingredients
sem_t *wholesaler = 1;
sem_t *access = 0, *milk = 0, *flour = 0, *walnut = 0, *sugar = 0;  


void arrayStorer(char* inputFilePath, char *name){
    struct stat statOfFile;                 // Adress of statOfFile will be sent to stat() function in order to get size information of file.
    int fileDesc;
    if( stat(inputFilePath, &statOfFile) < 0){
        dprintf(STDERR_FILENO, "Usage: ./hw3named -i inputFilePath -n name\n");
        errorAndExit("Error while opening file to read. ");
    }

    // Opening file in read mode
    if( (fileDesc = open(inputFilePath, O_RDONLY, 0666)) == -1 ){
        dprintf(STDERR_FILENO, "Usage: ./hw3named -i inputFilePath -n name\n");
        errorAndExit("Error while opening file to read. ");
    }

    // Understanding how many lines are there in input file
    if( statOfFile.st_size % 2 == 0 ){
        statOfFile.st_size = (statOfFile.st_size + 1) / 3 * 2; // Removing new line character from the size of file
    }
    else{
        statOfFile.st_size = statOfFile.st_size / 3 * 2; // Removing new line character from the size of file
    }

    int size = statOfFile.st_size/2;
    char (*ingredients)[2];
    ingredients = calloc(size, sizeof(char)); // Allocating memory for ingredients array
    
    // char ingredients[size][2];
    for(int i = 0; i<size; i++){
        if( read(fileDesc, ingredients[i], 2) == -1 && errno == EINTR )
            errorAndExit("Error while reading file to array");
        char garbage;
        if( read(fileDesc, &garbage, 1) == 0 ) // Don't add garbage '\n' value to file and if all file is readed, then break.
            break;
        dprintf(STDOUT_FILENO, "Ingredient %d: %s\n", i, ingredients[i]);
    }
    close(fileDesc);

    dprintf(STDOUT_FILENO,"Size of statOfFile.st_size is %ld\n", statOfFile.st_size);
    
    // Initialization of shared memory
    int shmId = shm_open("sharedMemory", O_CREAT | O_RDWR, 0666);
    if( shmId == -1 ){
        errorAndExit("Error while opening shared memory ");
    }
    if( ftruncate(shmId, sizeof(ingredients)) == -1 ){
        errorAndExit("Error while truncating shared memory ");
    }

    void *sharedMemoryPtr = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmId, 0);
    SharedMemory *sharedMemory = (SharedMemory*) sharedMemoryPtr;
    if( sharedMemory == MAP_FAILED )
        errorAndExit("Error while mapping shared memory ");
    
    // Writing array to shared memory
    sharedMemory->size = size;
    for(int i=0; i<size; i++){
        sharedMemory->ingredients[i][0] = ingredients[i][0];
        sharedMemory->ingredients[i][1] = ingredients[i][1];
    }

}

void problemHandler(){   
    wholesaler = sem_open("/wholesaler_semaphore", O_CREAT,  0644, 0);


    for(int i = 0; i < CHEF_NUMBER; i++){
        pid_t pid = fork();
        if( pid == 0 ){
            /********* CHILD i / CHEF i *********/
            chefHandler(i);
            break;
        }
        else if( pid > 0 ){
            /********* PARENT / WHOLESALER *********/
            int status;
            waitpid(pid, &status, WNOHANG/* Don't wait for child process to finish */);
            
            while(TRUE){
                // P( lock );
                // randNum = rand( 1, 3 ); // Pick a random number from 1-3
                // if ( randNum == 1 ) {
                // // Put tobacco on table
                // // Put paper on table
                // V( smoker_match );  // Wake up smoker with match
                // } else if ( randNum == 2 ) {
                // // Put tobacco on table
                //     // Put match on table
                //     V( smoker_paper );  // Wake up smoker with paper
                // } else {
                //     // Put match on table
                //     // Put paper on table
                //     V( smoker_tobacco ); 
                // } // Wake up smoker with tobacco
                // V( lock );
                // P( agent );  //  Agent sleeps
             }  // end while loop


        }
        else{
            errorAndExit("Error while creating child process. ");
        }
    }

}

void chefHandler(int chefNumber){
    switch(chefNumber){
        case 0:
            // has milk and flour but lacks walnuts and sugar,
            dprintf(STDOUT_FILENO, "Chef 0 is ready to cook.\n");
            

            sem_wait(walnut);
            sem_wait(access);
            if(access==0){
                if(generated && generated_item[0] != i && generated_item[1]!=i){
                    printf("smoker%d completed his smoking\n",i);
                    table_used = 1;
                    generated=0;
                }
            }
            sem_post(access);


            break;
        case 1:
            dprintf(STDOUT_FILENO, "Chef 1 is ready to cook.\n");
            break;
        case 2:
            dprintf(STDOUT_FILENO, "Chef 2 is ready to cook.\n");
            break;
        case 3:
            dprintf(STDOUT_FILENO, "Chef 2 is ready to cook.\n");
            break;
        case 4:
            dprintf(STDOUT_FILENO, "Chef 2 is ready to cook.\n");
            break;
        case 5:
            dprintf(STDOUT_FILENO, "Chef 2 is ready to cook.\n");
            break;
        default:
            errorAndExit("Chef number is not valid. ");
    }

    // sem_wait(wholesalerSem);    
}


// void pusherHandler(char pusherValue){
//     switch(pusherValue){
//         case 'M':
//             // has milk and flour but lacks walnuts and sugar,
//             dprintf(STDOUT_FILENO, "Chef 0 is ready to cook.\n");
//             sem_wait(walnut);
//             sem_wait(access);
//             if(access==0){
//                 if(generated && generated_item[0] != i && generated_item[1]!=i){
//                     printf("smoker%d completed his smoking\n",i);
//                     table_used = 1;
//                     generated=0;
//                 }
//             }
//             sem_post(access);

//             break;
//         case 'F':
//             dprintf(STDOUT_FILENO, "Chef 1 is ready to cook.\n");
//             break;
//         case 'W':
//             dprintf(STDOUT_FILENO, "Chef 2 is ready to cook.\n");
//             break;
//         case 'S':
//             dprintf(STDOUT_FILENO, "Chef 2 is ready to cook.\n");
//             break;
//         default:
//             errorAndExit("Chef number is not valid. ");
//     }

    // sem_wait(wholesalerSem);    
}

void errorAndExit(char *errorMessage){
    perror(errorMessage);
    exit(EXIT_FAILURE);
}
