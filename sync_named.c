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

SharedMemory *sharedMemory;
sem_t *wholesaler = 1;
sem_t *accessing = 0, *milk = 0, *flour = 0, *walnut = 0, *sugar = 0;  


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
    char (*ingredientsInFile)[2];
    ingredientsInFile = calloc(size, sizeof(char)); // Allocating memory for ingredients array
    
    // char ingredients[size][2];
    for(int i = 0; i<size; i++){
        if( read(fileDesc, ingredientsInFile[i], 2) == -1 && errno == EINTR )
            errorAndExit("Error while reading file to array");
        char garbage;
        if( read(fileDesc, &garbage, 1) == 0 ) // Don't add garbage '\n' value to file and if all file is readed, then break.
            break;
        dprintf(STDOUT_FILENO, "Ingredient %d: %s\n", i, ingredientsInFile[i]);
    }
    close(fileDesc);

    dprintf(STDOUT_FILENO,"Size of statOfFile.st_size is %ld\n", statOfFile.st_size);
    
    // Initialization of shared memory
    int shmId = shm_open("sharedMemory", O_CREAT | O_RDWR, 0666);
    if( shmId == -1 ){
        errorAndExit("Error while opening shared memory ");
    }
    if( ftruncate(shmId, sizeof(ingredientsInFile)) == -1 ){
        errorAndExit("Error while truncating shared memory ");
    }

    void *sharedMemoryPtr = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmId, 0);
    sharedMemory = (SharedMemory*) sharedMemoryPtr;
    if( sharedMemory == MAP_FAILED )
        errorAndExit("Error while mapping shared memory ");
    
    // Writing array to shared memory
    sharedMemory->numberOfLines = size;
    // for(int i=0; i<size; i++){
        sharedMemory->ingredients[0] = ingredientsInFile[0][0];
        sharedMemory->ingredients[1] = ingredientsInFile[0][1];
    // }

}

void problemHandler(){   
    wholesaler = sem_open("/wholesaler_semaphore", O_CREAT,  0644, 0);



    pid_t pid;
    int i;
    for(i = 0; i < CHEF_NUMBER; i++){
        pid = fork();
        if( pid == 0 ){
            /********* CHILD i / CHEF i *********/
            chefHandler(i);
            break;
        }
        else if( pid > 0 ){
            /********* PARENT / WHOLESALER *********/
            int status;
            waitpid(pid, &status, WNOHANG/* Don't wait for child process to finish */);
        }
        else{ // pid < 0 
            errorAndExit("Error while creating child process. ");
        }
    }

    if(pid == 0){ 
        // child
        
    }else{ 
        // parent
        dprintf(STDOUT_FILENO, "Parent continue\n");
        wholesalerProcess();

    }

}

void pusher(){
    
}


void wholesalerProcess(){
    


}

void chef(int chefNumber){
    /* 
    chef0 has an endless supply of milk and flour but lacks walnuts and sugar, chef1 has an endless supply of milk and sugar but lacks flour and walnuts, chef2 has an endless supply of milk and walnuts but lacks sugar and flour, chef3 has an endless supply of sugar and walnuts but lacks milk and flour, chef4 has an endless supply of sugar and flour but lacks milk and walnuts, chef5 has an endless supply of flour and walnuts but lacks sugar and milk. */
    switch(chefNumber){
        case 0: // Waiting for WALNUT and SUGAR [has milk and flour]
            dprintf(STDOUT_FILENO, "Chef 0 is waiting for walnut and sugar.\n");
            sem_wait(walnut);
            if( (sharedMemory->ingredients[0] == 'W' && sharedMemory->ingredients[1] == 'S')
                || (sharedMemory->ingredients[0] == 'S' && sharedMemory->ingredients[1] == 'W') ){
                dprintf(STDOUT_FILENO, "Chef0 found both needed ingredients from wholesaler.\n");
                sem_wait(sugar);
                // TODO
                sem_post(walnut);
                sem_post(sugar);
            }
            else{ // If 2 ingredients doesn't match
                sem_post(walnut);
            }

            break;
        case 1: // Waiting for FLOUR and WALNIT
            dprintf(STDOUT_FILENO, "Chef 1 is waiting for milk and sugar.\n");
            sem_wait(flour);
            if( (sharedMemory->ingredients[0] == 'M' && sharedMemory->ingredients[1] == 'S')
                || (sharedMemory->ingredients[0] == 'S' && sharedMemory->ingredients[1] == 'M') ){
                dprintf(STDOUT_FILENO, "Chef1 found both needed ingredients from wholesaler.\n");
                sem_wait(walnut);
                // TODO
                // handle array


                //////
                sem_post(flour);
                sem_post(walnut);
            }
            else{ // If 2 ingredients doesn't match
                sem_post(flour);
            }
            break;
        case 2: // Waiting for SUGAR and FLOUR
            dprintf(STDOUT_FILENO, "Chef 2 is waiting for sugar and flour\n");
            sem_wait(flour);
            if( (sharedMemory->ingredients[0] == 'M' && sharedMemory->ingredients[1] == 'S')
                || (sharedMemory->ingredients[0] == 'S' && sharedMemory->ingredients[1] == 'M') ){
                dprintf(STDOUT_FILENO, "Chef2 found both needed ingredients from wholesaler.\n");
                sem_wait(sugar); 
                // TODO
                sem_post(flour);
                sem_post(sugar);
            }
            else{ // If 2 ingredients doesn't match
                sem_post(flour);
            }
            break;
        case 3: // Waiting for MILK and FLOUR
            dprintf(STDOUT_FILENO, "Chef 3 is waiting for milk and flour\n");
            sem_wait(milk);
            if( (sharedMemory->ingredients[0] == 'M' && sharedMemory->ingredients[1] == 'S')
                || (sharedMemory->ingredients[0] == 'S' && sharedMemory->ingredients[1] == 'M') ){
                dprintf(STDOUT_FILENO, "Chef3 found both needed ingredients from wholesaler.\n");
                sem_wait(flour);
                // TODO
                sem_post(milk);
                sem_post(flour);
            }
            else{ // If 2 ingredients doesn't match
                sem_post(milk);
            }
            break;
        case 4: // Waiting for  MILK and WALNUTS
            dprintf(STDOUT_FILENO, "Chef 4 is waiting for milk and walnuts\n");
            sem_wait(milk);
            if( (sharedMemory->ingredients[0] == 'M' && sharedMemory->ingredients[1] == 'S')
                || (sharedMemory->ingredients[0] == 'S' && sharedMemory->ingredients[1] == 'M') ){
                dprintf(STDOUT_FILENO, "Chef4 found both needed ingredients from wholesaler.\n");
                sem_wait(walnut);
                // TODO
                sem_post(milk);
                sem_post(walnut);
            }
            else{ // If 2 ingredients doesn't match
                sem_post(milk);
            }
            break;
        case 5: // Waiting for SUGAR and MILK
            dprintf(STDOUT_FILENO, "Chef 5 is waiting for sugar and milk\n");
            sem_wait(milk);
            if( (sharedMemory->ingredients[0] == 'M' && sharedMemory->ingredients[1] == 'S')
                || (sharedMemory->ingredients[0] == 'S' && sharedMemory->ingredients[1] == 'M') ){
                dprintf(STDOUT_FILENO, "Chef5 found both needed ingredients from wholesaler.\n");
                sem_wait(sugar);
                // TODO
                sem_post(milk);
                sem_post(sugar);
            }
            else{ // If 2 ingredients doesn't match
                sem_post(milk);
            }
            break;
        default:
            errorAndExit("Chef number is not valid. ");
    }


}


// void pusherHandler(char pusherValue){
//     switch(pusherValue){
//         case 'M':
//             // has milk and flour but lacks walnuts and sugar,
//             dprintf(STDOUT_FILENO, "Chef 0 is ready to cook.\n");
//             sem_wait(walnut);
//             sem_wait(accessing);
//             if(accessing==0){
//                 if(generated && generated_item[0] != i && generated_item[1]!=i){
//                     printf("smoker%d completed his smoking\n",i);
//                     table_used = 1;
//                     generated=0;
//                 }
//             }
//             sem_post(accessing);

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
// }

void errorAndExit(char *errorMessage){
    perror(errorMessage);
    exit(EXIT_FAILURE);
}
