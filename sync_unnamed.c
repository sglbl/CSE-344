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
#include "sync_unnamed.h"

SharedMemory *sharedMemory;
static sem_t signalToChef[NUMBER_OF_CHEFS] = { 0 };
static sem_t accessing, milk, flour, walnut, sugar;  
static sem_t milkSignal, flourSignal, walnutSignal, sugarSignal;  
static sem_t wholesaler;
static volatile int isMilk = 0, isFlour = 0, isWalnut = 0, isSugar = 0;


void arrayStorer(char* inputFilePath){
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
    // Initializing semaphores
    dprintf(STDOUT_FILENO, "0\n");
    for(int i = 0; i < NUMBER_OF_CHEFS; i++)
        if( sem_init(&signalToChef[i], 1 /* No pthread */, 0) < 0 )
            errorAndExit("Semaphore initialize error ");

    dprintf(STDOUT_FILENO, "1\n");
    if( sem_init(&accessing, 0 /* No pthread */, 1) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&wholesaler, 0 /* No pthread */, 1) < 0 )
        errorAndExit("Semaphore initialize error ");
    dprintf(STDOUT_FILENO, "2\n");
    if( sem_init(&milk, 0 /* No pthread */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&flour, 0 /* No pthread */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sugar, 0 /* No pthread */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&walnut, 0 /* No pthread */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    
    if( sem_init(&milkSignal, 0 /* No pthread */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&flourSignal, 0 /* No pthread */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sugarSignal, 0 /* No pthread */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&walnutSignal, 0 /* No pthread */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");

    dprintf(STDOUT_FILENO, "Print1\n");
    fprintf(stderr, "Print2\n");

    pid_t pid;
    for(int i = 0; i < NUMBER_OF_INGREDIENTS; i++){
        pid = fork();
        if( pid == 0 ){
            /********* CHILD i / PUSHER i *********/
            printf("i is %d\n", i);
            if     (i == 0) pusherFlour();
            else if(i == 1) pusherMilk();
            else if(i == 2) pusherSugar();
            else if(i == 3){ printf("Enter\n"); pusherWalnut(); }
            break;
        }
        else if( pid > 0 ){
            /********* PARENT / WHOLESALER *********/
            int status;
            waitpid(pid, &status, WNOHANG/* Don't wait for child process to finish */);
        }
        else{ // pid < 0 
            errorAndExit("Error while creating child process for pushers. ");
        }
    }

    int i;
    for(i = 0; i < NUMBER_OF_CHEFS; i++){
        pid = fork();
        if( pid == 0 ){
            /********* CHILD i / CHEF i *********/
            chef(i);
            break;
        }
        else if( pid > 0 ){
            /********* PARENT / WHOLESALER *********/
            int status;
            waitpid(pid, &status, WNOHANG/* Don't wait for child process to finish */);
        }
        else{ // pid < 0 
            errorAndExit("Error while creating child process for chefs. ");
        }
    }

    // if(pid == 0){ 
    //     // child
        
    // }else{ 
    if(pid > 0){  // parent
        dprintf(STDOUT_FILENO, "Parent continue\n");
        wholesalerProcess();
    }

}

void pusherMilk(){ //Wakes up when there is milk by wholesaler
    sem_wait(&milk);
    sem_wait(&accessing);
    if(isWalnut == 1 && isFlour == 1){
        isWalnut = isFlour = 0;
        sem_post(&sugarSignal);
    }
    else if(isWalnut == 1 && isSugar == 1){
        isWalnut = isSugar = 0;
        sem_post(&flourSignal);
    }
    else if(isFlour == 1 && isSugar == 1){
        isFlour = isSugar = 0;
        sem_post(&walnutSignal);
    }
    else
        isMilk = 1;
    sem_post(&accessing);
}

void pusherFlour(){
    sem_wait(&flour);
    sem_wait(&accessing);
    if(isWalnut == 1 && isMilk == 1){
        isWalnut = isMilk = 0;
        sem_post(&sugarSignal);
    }
    else if(isWalnut == 1 && isSugar == 1){
        isWalnut = isSugar = 0;
        sem_post(&milkSignal);
    }
    else if(isMilk == 1 && isSugar == 1){
        isMilk = isSugar = 0;
        sem_post(&walnutSignal);
    }
    else
        isFlour = 1;
    sem_post(&accessing);
}

void pusherSugar(){
    sem_wait(&sugar);
    sem_wait(&accessing);
    if(isWalnut == 1 && isFlour == 1){
        isWalnut = isFlour = 0;
        sem_post(&milkSignal);
    }
    else if(isWalnut == 1 && isMilk == 1){
        isWalnut = isMilk = 0;
        sem_post(&flourSignal);
    }
    else if(isFlour == 1 && isMilk == 1){
        isFlour = isMilk = 0;
        sem_post(&walnutSignal);
    }
    else
        isSugar = 1;
    sem_post(&accessing);
}

void pusherWalnut(){
    dprintf(STDOUT_FILENO,"In pusher3\n");
    sem_wait(&walnut);
    sem_wait(&accessing);
    if(isMilk == 1 && isFlour == 1){
        isMilk = isFlour = 0;
        sem_post(&sugarSignal);
    }
    else if(isMilk == 1 && isSugar == 1){
        isMilk = isSugar = 0;
        sem_post(&flourSignal);
    }
    else if(isFlour == 1 && isSugar == 1){
        isFlour = isSugar = 0;
        sem_post(&walnutSignal);
    }
    else
        isWalnut = 1;
    sem_post(&accessing);
}

void wholesalerProcess(){
    sem_wait(&wholesaler);
    switch(sharedMemory->ingredients[0]){
        case 'M':   sem_post(&milk);   break;
        case 'W':   sem_post(&walnut); break;
        case 'F':   sem_post(&flour);  break;
        case 'S':   sem_post(&sugar);  break;
    }
    switch(sharedMemory->ingredients[1]){
        case 'M':   sem_post(&milk);   break;
        case 'W':   sem_post(&walnut); break;
        case 'F':   sem_post(&flour);  break;
        case 'S':   sem_post(&sugar);  break;
    }   


}

void chef(int chefNumber){
    /* 
    chef0 has an endless supply of milk and flour but lacks walnuts and sugar, chef1 has an endless supply of milk and sugar but lacks flour and walnuts, chef2 has an endless supply of milk and walnuts but lacks sugar and flour, chef3 has an endless supply of sugar and walnuts but lacks milk and flour, chef4 has an endless supply of sugar and flour but lacks milk and walnuts, chef5 has an endless supply of flour and walnuts but lacks sugar and milk. */
    switch(chefNumber){
        case 0: // Waiting for WALNUT and SUGAR [has milk and flour]
            dprintf(STDOUT_FILENO, "Chef 0 is waiting for walnut and sugar.\n");
            // write the one which has
            sem_wait(&milkSignal);
            sem_wait(&flourSignal);
            dprintf(STDOUT_FILENO, "Chef1 found both needed ingredients from wholesaler.\n");
            sem_post(&wholesaler);
            break;
        case 1: // Waiting for FLOUR and WALNIT [has milk and sugar]
            dprintf(STDOUT_FILENO, "Chef 1 is waiting for milk and sugar.\n");
            sem_wait(&milkSignal);
            sem_wait(&sugarSignal);
            dprintf(STDOUT_FILENO, "Chef1 found both needed ingredients from wholesaler.\n");
            sem_post(&wholesaler);
            break;
        case 2: // Waiting for SUGAR and FLOUR [has milk and walnuts]
            dprintf(STDOUT_FILENO, "Chef 2 is waiting for sugar and flour\n");
            sem_wait(&milkSignal);
            sem_wait(&walnutSignal);
            dprintf(STDOUT_FILENO, "Chef2 found both needed ingredients from wholesaler.\n");    
            sem_post(&wholesaler);
            break;
        case 3: // Waiting for MILK and FLOUR [has sugar and walnuts]
            dprintf(STDOUT_FILENO, "Chef 3 is waiting for milk and flour\n");
            sem_wait(&sugarSignal);
            sem_wait(&walnutSignal);
            dprintf(STDOUT_FILENO, "Chef3 found both needed ingredients from wholesaler.\n");
            sem_post(&wholesaler);
            break;
        case 4: // Waiting for  MILK and WALNUTS [has sugar and flour]
            dprintf(STDOUT_FILENO, "Chef 4 is waiting for milk and walnuts\n");
            sem_wait(&sugarSignal);
            sem_wait(&flourSignal);
            dprintf(STDOUT_FILENO, "Chef4 found both needed ingredients from wholesaler.\n");
            sem_post(&wholesaler);
            break;
        case 5: // Waiting for SUGAR and MILK [has flour and walnuts]
            dprintf(STDOUT_FILENO, "Chef 5 is waiting for sugar and milk\n");
            sem_wait(&walnutSignal);
            dprintf(STDOUT_FILENO, "Still waiting\n");
            sem_wait(&flourSignal);
            dprintf(STDOUT_FILENO, "Chef5 found both needed ingredients from wholesaler.\n");
            sem_post(&wholesaler);
            break;
        default:
            errorAndExit("Chef number is not valid. ");
    }

}

void errorAndExit(char *errorMessage){
    perror(errorMessage);
    exit(EXIT_FAILURE);
}
