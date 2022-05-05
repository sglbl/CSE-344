#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
// gcc -Wall -Wextra -pthread  main.c -lm && ./a.out -lpthread

#define SIZE 6

typedef struct mult_info {
	int base;
    int remaining;
} mult_info;

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numArrived = 0;       /* number who have arrived */

int binaryToDecimal(int binaryNum){
    int remainingBit;
    int decimalNum = 0;

    for(int base = 1; binaryNum > 0; base *= 2, binaryNum /= 10){
        remainingBit = binaryNum % 10;
        decimalNum += remainingBit * base;
    }
    return decimalNum;
}

/* a reusable counter barrier */
void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == SIZE) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else
    pthread_cond_wait(&go, &barrier);
  pthread_mutex_unlock(&barrier);
}

void* masterRoutine(void *arg) {
    int *table = malloc(SIZE * sizeof(int)); //Allocating memory so after function finishes we still have it.
    int myid = (long) arg; // long because sizeof(long) == sizeof(void *) so no warning: cast from pointer to integer of different size

    printf("master %d (pthread id %ld) has started\n", myid, pthread_self());
    
    srand(time(NULL));

    for(int i=0; i<SIZE; i++){
        int randDigit = rand() % 2;
        table[i] = randDigit; 
    } //Master created random binary table digits. At the end will return.

    return (void*)table;  
}

void *workerRoutine(void *arg) {
    mult_info *info = arg;
    printf("Thread ");
    printf("base:%d " , info->base);
    printf("remaining:%d\n", info->remaining);
    int multiplication = pow(2,info->base) * info->remaining;
    int *result = malloc(100*SIZE*sizeof(int));
    *result = multiplication;
    // Barrier();

    return (void*)result;
}


int main(){
    // int number = 10101;
    // printf("%d\n", binaryToDecimal(number) );
    
    int decimalNumber;
    int *table;
    pthread_attr_t attr;
    pthread_t workerid[SIZE];
    pthread_t master;

    /* set global thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    /* initialize mutex and condition variable */
    pthread_mutex_init(&barrier, NULL);
    pthread_cond_init(&go, NULL);
    
    if( pthread_create(&master, NULL, masterRoutine, (void*)0) != 0 ){ //if returns 0 it's okay.
            printf("ERROR from pthread_create()");
            return 1;
    }
    if( pthread_join(master, (void**) &table) != 0 ){
        printf("ERROR from pthread_join()");
        return 2;
    }
    int* randomNum = table;
    
    printf("Binary number is ");
    for(int i=0; i< SIZE; i++)
        printf("%d", randomNum[i]);
    printf("\n");
    
    /********************/

    decimalNumber = 0;
    /* create the workers, then exit */
    for (int i = 0; i < SIZE; i++){  
        int *mult;
        mult_info info;// = malloc(2*sizeof(int));  // or // mult_info *info with ->
        info.base = (int)(SIZE-i-1); 
        info.remaining = randomNum[i];
        if( pthread_create(&workerid[i], &attr, workerRoutine, (void*)&info) != 0 ){ //if returns 0 it's okay.
            printf("ERROR from pthread_create()");
            return 1;
        }
        
        if( pthread_join(workerid[i], (void**) &mult) != 0 ){
            printf("ERROR from pthread_join()");
            return 2;
        }
        
        int intMult = *mult;
        free(mult);
        decimalNumber += intMult;
    }
    // pthread_exit(NULL);
    printf("Decimal number is %d\n", decimalNumber);
    free(table);

    return 0;
}
