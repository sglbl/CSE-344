#ifndef COMMON_H_
#define COMMON_H_

/*
	@author Suleyman Golbol
	@number 1801042656
*/

#define TRUE 1
#define FALSE 0

#define CLIENT 1
#define SERVANT 2

typedef struct String {
    char *data;
} String;

typedef struct ServantSendingInfo{
    int head, tail, procId, portNoToUseLater;
    int cityName1Size, cityName2Size;
    char *cityName1, *cityName2;
    struct ServantSendingInfo *next;
} ServantSendingInfo;

typedef struct ServantGettingInfo{
    int beginDay, beginMonth, beginYear;
    int endDay, endMonth, endYear;
    int cityHandleType;
    // int estateSize;
    int structSize;
    char estateTypeAndCity[];
} ServantGettingInfo;

typedef struct Date{
    int day, month, year;
} Date;

typedef struct SgLinkedList{
    int fileDesc, isBusy;
    String string;
    struct SgLinkedList *next;
} SgLinkedList;

SgLinkedList get(SgLinkedList *head, int index);

SgLinkedList addValueToList(SgLinkedList *head, char *value);

/* Returns time stamp */
char *timeStamp();
/* Prints error message and exits */
void errorAndExit(char *errorMessage);

#endif