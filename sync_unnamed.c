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
#include "sync_unnamed.h"

SharedMemory *sharedMemory;
static int lineNumber = 0;
static char (*ingredientsInFile)[2];
// static sem_t signalToChef[NUMBER_OF_CHEFS] = { 0 };
// static sem_t accessing, milk, flour, walnut, sugar;  
// static sem_t milkSignal, flourSignal, walnutSignal, sugarSignal;  
// static sem_t wholesaler;
// static volatile int isMilk = 0, isFlour = 0, isWalnut = 0, isSugar = 0;


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
    // char (*ingredientsInFile)[2];
    ingredientsInFile = calloc(size, sizeof(char)); // Allocating memory for ingredients array
    
    // char ingredients[size][2];
    for(int i = 0; i<size; i++){
        if( read(fileDesc, ingredientsInFile[i], 2) == -1 && errno == EINTR )
            errorAndExit("Error while reading file to array");
        dprintf(STDOUT_FILENO, "Ingredient %d: %s\n", i, ingredientsInFile[i]);
        char garbage;
        if( read(fileDesc, &garbage, 1) == 0 ) // Don't add garbage '\n' value to file and if all file is readed, then break.
            break;
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
    sharedMemory->ingredients[0] = ingredientsInFile[0][0];
    sharedMemory->ingredients[1] = ingredientsInFile[0][1];
    lineNumber = 1;
}

void semaphoreInitializer(){
    // Initializing semaphores
    for(int i = 0; i < NUMBER_OF_CHEFS; i++)
        if( sem_init(&(sharedMemory->signalToChef[i]), 1, 0) < 0 )
            errorAndExit("Semaphore initialize error ");

    if( sem_init(&sharedMemory->accessing, 1 /* Pshare */, 1) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->wholesaler, 1 /* Pshare */, 1) < 0 )
        errorAndExit("Semaphore initialize error ");

    if( sem_init(&sharedMemory->milk, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->flour, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->sugar, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->walnut, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    
    if( sem_init(&sharedMemory->milkSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->flourSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->sugarSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");
    if( sem_init(&sharedMemory->walnutSignal, 1 /* Pshare */, 0) < 0 )
        errorAndExit("Semaphore initialize error ");

}

void problemHandler(){
    semaphoreInitializer();

    pid_t pid;
    for(int i = 0; i < NUMBER_OF_INGREDIENTS; i++){
        pid = fork();
        if( pid == 0 ){
            /********* CHILD i / PUSHER i *********/
            if     (i == 0) pusherFlour();
            else if(i == 1) pusherMilk();
            else if(i == 2) pusherSugar();
            else if(i == 3) pusherWalnut();
            //break;
            return; //çç
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

    if(pid > 0){  // parent
        wholesalerProcess();
    }

}

void wholesalerProcess(){
    int foundFLag1 = FALSE, foundFLag2 = FALSE;
    while(TRUE){
        sem_wait(&sharedMemory->wholesaler);
        switch(sharedMemory->ingredients[0]){
            case 'M':   sem_post(&sharedMemory->milk);   
            int *val;
            sem_getvalue(&sharedMemory->milk, val);
            printf("Val2 is %d\n", *val);
            foundFLag1 = TRUE; break;
            case 'W':   sem_post(&sharedMemory->walnut); foundFLag1 = TRUE; break;
            case 'F':   sem_post(&sharedMemory->flour);  foundFLag1 = TRUE; break;
            case 'S':   sem_post(&sharedMemory->sugar);  foundFLag1 = TRUE; break;
        }
        switch(sharedMemory->ingredients[1]){
            case 'M':   sem_post(&sharedMemory->milk);   foundFLag2 = TRUE; break;
            case 'W':   sem_post(&sharedMemory->walnut); foundFLag2 = TRUE; break;
            case 'F':   sem_post(&sharedMemory->flour);  foundFLag2 = TRUE; break;
            case 'S':   sem_post(&sharedMemory->sugar);  foundFLag2 = TRUE; break;
        }   
        printf("Posted\n");
        if(foundFLag1 == TRUE && foundFLag2 == TRUE){
            sharedMemory->ingredients[0] = ingredientsInFile[lineNumber][0];
            sharedMemory->ingredients[1] = ingredientsInFile[lineNumber][1];
            lineNumber++;
            dprintf(STDOUT_FILENO, "Second line\n");
            
        }
    }

}

void pusherMilk(){ //Wakes up when there is milk by wholesaler
    dprintf(STDOUT_FILENO,"Waiting for milk pusher\n");
    int *val;
    sem_getvalue(&sharedMemory->milk, val);
    printf("Val is %d\n", *val);
    sem_wait(&sharedMemory->milk);
    sem_wait(&sharedMemory->accessing);
    dprintf(STDOUT_FILENO,"!!! MILK ENTRANCE !!!\n");
    if(sharedMemory->isWalnut == 1 && sharedMemory->isFlour == 1){
        sharedMemory->isWalnut = sharedMemory->isFlour = 0;
        sem_post(&sharedMemory->sugarSignal);
    }
    else if(sharedMemory->isWalnut == 1 && sharedMemory->isSugar == 1){
        sharedMemory->isWalnut = sharedMemory->isSugar = 0;
        sem_post(&sharedMemory->flourSignal);
    }
    else if(sharedMemory->isFlour == 1 && sharedMemory->isSugar == 1){
        sharedMemory->isFlour = sharedMemory->isSugar = 0;
        sem_post(&sharedMemory->walnutSignal);
    }
    else{
        dprintf(STDOUT_FILENO, "Milk is 1\n");
        sharedMemory->isMilk = 1;
    }
    sem_post(&sharedMemory->accessing);
}

void pusherFlour(){     //Wakes up when there is flour by wholesaler
    sem_wait(&sharedMemory->flour);
    sem_wait(&sharedMemory->accessing);
    dprintf(STDOUT_FILENO,"!!! FLOUR ENTRANCE !!!\n");
    if(sharedMemory->isWalnut == 1 && sharedMemory->isMilk == 1){
        sharedMemory->isWalnut = sharedMemory->isMilk = 0;
        sem_post(&sharedMemory->sugarSignal);
        dprintf(STDOUT_FILENO, "Walnut and flour is 1\n");
    }
    else if(sharedMemory->isWalnut == 1 && sharedMemory->isSugar == 1){
        sharedMemory->isWalnut = sharedMemory->isSugar = 0;
        sem_post(&sharedMemory->milkSignal);
        dprintf(STDOUT_FILENO, "Walnut and sugar is 1\n");
    }
    else if(sharedMemory->isMilk == 1 && sharedMemory->isSugar == 1){
        sharedMemory->isMilk = sharedMemory->isSugar = 0;
        dprintf(STDOUT_FILENO, "!!!!Signaling walnut from flour [1OUR PURPOSE]!!!!\n");
        sem_post(&sharedMemory->walnutSignal);
    }
    else{
        dprintf(STDOUT_FILENO, "Flour is 1\n");
        sharedMemory->isFlour = 1;
    }
    sem_post(&sharedMemory->accessing);
}

void pusherSugar(){       //Wakes up when there is sugar by wholesaler
    sem_wait(&sharedMemory->sugar);
    dprintf(STDOUT_FILENO,"!!! Sugar entrance !!!\n");
    sem_wait(&sharedMemory->accessing);
    int *val;
    sem_getvalue(&sharedMemory->milk, val);
    printf("Val3 is %d\n", *val);
    if(sharedMemory->isWalnut == 1 && sharedMemory->isFlour == 1){
        sharedMemory->isWalnut = sharedMemory->isFlour = 0;
        sem_post(&sharedMemory->milkSignal);
    }
    else if(sharedMemory->isWalnut == 1 && sharedMemory->isMilk == 1){
        sharedMemory->isWalnut = sharedMemory->isMilk = 0;
        sem_post(&sharedMemory->flourSignal);
    }
    else if(sharedMemory->isFlour == 1 && sharedMemory->isMilk == 1){
        sharedMemory->isFlour = sharedMemory->isMilk = 0;
        sem_post(&sharedMemory->walnutSignal);
    }
    else{
        dprintf(STDOUT_FILENO, "Sugar is 1\n");
        sharedMemory->isSugar = 1;
    }
    sem_post(&sharedMemory->accessing);
}

void pusherWalnut(){    //Wakes up when there is walnut by wholesaler
    dprintf(STDOUT_FILENO, "Walnut pusher is waiting\n");
    sem_wait(&sharedMemory->walnut);
    dprintf(STDOUT_FILENO,"!!! WALNUT ENTRANCE !!!\n");
    sem_wait(&sharedMemory->accessing);
    dprintf(STDOUT_FILENO,"!!! WALNUT PASSED ACCESS !!!\n");
    if(sharedMemory->isMilk == 1 && sharedMemory->isFlour == 1){
        sharedMemory->isMilk = sharedMemory->isFlour = 0;
        sem_post(&sharedMemory->sugarSignal);
    }
    else if(sharedMemory->isMilk == 1 && sharedMemory->isSugar == 1){
        sharedMemory->isMilk = sharedMemory->isSugar = 0;
        dprintf(STDOUT_FILENO, "!!!!Signaling flour from walnut [2OUR PURPOSE]!!!!\n");
        sem_post(&sharedMemory->flourSignal);
    }
    else if(sharedMemory->isFlour == 1 && sharedMemory->isSugar == 1){
        sharedMemory->isFlour = sharedMemory->isSugar = 0;
        sem_post(&sharedMemory->milkSignal);
    }
    else{
        dprintf(STDOUT_FILENO, "isWalnut is 1\n");
        sharedMemory->isWalnut = 1;
    }
    sem_post(&sharedMemory->accessing);
}

void chef(int chefNumber){
    /* 
    chef0 has an endless supply of milk and flour but lacks walnuts and sugar, 
    chef1 has an endless supply of milk and sugar but lacks flour and walnuts, 
    chef2 has an endless supply of milk and walnuts but lacks sugar and flour, 
    chef3 has an endless supply of sugar and walnuts but lacks milk and flour, 
    chef4 has an endless supply of sugar and flour but lacks milk and walnuts, 
    chef5 has an endless supply of flour and walnuts but lacks sugar and milk. */
    switch(chefNumber){
        case 0: // Waiting for WALNUT and SUGAR [has milk and flour]
            dprintf(STDOUT_FILENO, "Chef 0 is waiting for walnut and sugar. and pid is %d\n", getpid());
            // write the one which has
            sem_wait(&sharedMemory->milkSignal);
            sem_wait(&sharedMemory->flourSignal);
            dprintf(STDOUT_FILENO, "Chef1 found both needed ingredients from wholesaler.\n");
            sem_post(&sharedMemory->wholesaler);
            break;
        case 1: // Waiting for FLOUR and WALNIT [has milk and sugar]
            dprintf(STDOUT_FILENO, "Chef 1 is waiting for milk and sugar.\n");
            sem_wait(&sharedMemory->milkSignal);
            sem_wait(&sharedMemory->sugarSignal);
            dprintf(STDOUT_FILENO, "Chef1 found both needed ingredients from wholesaler.\n");
            sem_post(&sharedMemory->wholesaler);
            break;
        case 2: // Waiting for SUGAR and FLOUR [has milk and walnuts]
            dprintf(STDOUT_FILENO, "Chef 2 is waiting for sugar and flour\n");
            sem_wait(&sharedMemory->milkSignal);
            sem_wait(&sharedMemory->walnutSignal);
            dprintf(STDOUT_FILENO, "Chef2 found both needed ingredients from wholesaler.\n");    
            sem_post(&sharedMemory->wholesaler);
            break;
        case 3: // Waiting for MILK and FLOUR [has sugar and walnuts]
            dprintf(STDOUT_FILENO, "Chef 3 is waiting for milk and flour\n");
            sem_wait(&sharedMemory->sugarSignal);
            sem_wait(&sharedMemory->walnutSignal);
            dprintf(STDOUT_FILENO, "Chef3 found both needed ingredients from wholesaler.\n");
            sem_post(&sharedMemory->wholesaler);
            break;
        case 4: // Waiting for  MILK and WALNUTS [has sugar and flour]
            dprintf(STDOUT_FILENO, "Chef 4 is waiting for milk and walnuts\n");
            sem_wait(&sharedMemory->sugarSignal);
            sem_wait(&sharedMemory->flourSignal);
            dprintf(STDOUT_FILENO, "Chef4 found both needed ingredients from wholesaler.\n");
            sem_post(&sharedMemory->wholesaler);
            break;
        case 5: // Waiting for SUGAR and MILK [has flour and walnuts]
            dprintf(STDOUT_FILENO, "Chef 5 is waiting for sugar and milk\n");
            sem_post(&sharedMemory->flour);
            sem_wait(&sharedMemory->walnutSignal);
            dprintf(STDOUT_FILENO, "Still waiting\n");
            sem_post(&sharedMemory->walnut);
            sem_wait(&sharedMemory->flourSignal);
            dprintf(STDOUT_FILENO, "Chef5 found both needed ingredients from wholesaler.\n");
            sem_post(&sharedMemory->wholesaler);
            break;
        default:
            errorAndExit("Chef number is not valid. ");
    }

}

void errorAndExit(char *errorMessage){
    perror(errorMessage);
    exit(EXIT_FAILURE);
}
