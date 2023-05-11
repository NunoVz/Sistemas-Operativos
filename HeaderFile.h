#ifndef HEADER_FILE_H
#define HEADER_FILE_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/sem.h>
#include <mqueue.h>

#define MAX_MSG_SIZE 256
#define MAX_OUTPUT 5000
#define MAX_MSG_NUM 10
#define QUEUE_NAME "/my_queue"
#define MAX_MESSAGES 10
#define QUEUE_PERMISSIONS 0660

// Define the message structure
struct queuemsg {
    long mtype;  // Message type
    char mtext[MAX_MSG_SIZE]; // Message payload
};

int queusize;
int nworkers;
int maxkeys;
int maxsensors;
int maxalerts;




typedef struct keyStats {
	char key[32];
    int last;
	int minValue;
	int maxValue;
    int avg;
    int count;
    bool verified;
    struct keyStats *next;
} keyStats;

typedef struct sensor {
	char sensorId[32];
    struct sensor *next;

 
} sensor;

typedef struct alertStruct {
	char id[32];
	char key[32];
	int minValue;
	int maxValue;
    int myuser;
    struct alertStruct *next;
} alertStruct;


typedef struct {
    int* semwork;
    int semid;
    int shmid;
    char *shmaddr;
    int contKey;
    int contSen;
    int contAlert;
    alertStruct *alertList;
    keyStats *keystatsList;
    sensor *sensorList;
} SharedMemory;


SharedMemory* shm_ptr;
int *shmids;

typedef struct worker {
    int i;
    int status;
    struct worker* next;
} worker_t;

void initializeSemaphore();
void printCommands();
void init_log();
void writelog(char *message);
bool validateSensor(char *inputSensor[]);
void printCommands();
void add_to_queue(char *message);
mqd_t create_queue();
void add_to_queue(char *message);
char* get_from_queue();

#endif
