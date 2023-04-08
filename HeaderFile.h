#ifndef HEADER_FILE_H
#define HEADER_FILE_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>


#define MAX_MSG_SIZE 256
#define MAX_MSG_NUM 10
#define QUEUE_NAME "/my_queue"
#define MAX_MESSAGES 10
#define QUEUE_PERMISSIONS 0660

// Define the message structure
struct queuemsg {
    long mtype;  // Message type
    char mtext[MAX_MSG_SIZE]; // Message payload
};

sem_t *logMutex; // Semáforo que será usado para controlar o acesso a um recurso compartilhado com outros processos/threads.
int queusize;
int nworkers;
int maxkeys;
int maxsensors;
int maxalerts;

typedef struct something
{
	char sensorId[32];
	int sendInterval;
	char sensorKey[32];
	int minValue;
	int maxValue;
} something;

something systemSensor; // Inicialização da estrutura sensor.

typedef struct keyStats
{
	char key[32];
    int last;
	int minValue;
	int maxValue;
    int avg;
    int count;
    struct keyStats *next;
} keyStats;
typedef struct sensor
{
	char sensorId[32];
    struct sensor *next;

 
} sensor;
typedef struct alertStruct
{
	char id[32];
	char key[32];
	int minValue;
	int maxValue;
    struct alertStruct *next;

 
} alertStruct;


typedef struct {
    alertStruct *alertList;
    keyStats *keystatsList;
    sensor *sensorList;
    int semwork[5];
    int semid;
    int shmid;
    char *shmaddr;
} SharedMemory;


SharedMemory* shm_ptr;

typedef struct worker {
    int i;
    int status;
    struct worker* next;
} worker_t;

void initializeSemaphore();
void printCommands();

#endif
