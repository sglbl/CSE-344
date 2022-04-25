#ifndef PREREQUISITES_H_
#define PREREQUISITES_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

#define TRUE 1
#define FALSE 0
#define CHEF_NUMBER 6


#define TEMP_PID "/tmp/singleton.pid"
/* A template name for client fifo */
#define TEMP_CLIENT_FIFO "/tmp/client_fifo"
/* Client fifo path name space */
#define CLIENT_FIFO_PATH_NAME_SIZE sizeof(TEMP_CLIENT_FIFO) + 10


typedef struct SharedMemory {
    int size;
    // char (*ingredients)[2];
    char ingredients[][2];
} SharedMemory;

#endif