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

sem_t *logMutex; // Semáforo que será usado para controlar o acesso a um recurso compartilhado com outros processos/threads.

typedef struct sensor
{
	char sensorId[32];
	int sendInterval;
	char sensorKey[32];
	int minValue;
	int maxValue;
} sensor;

sensor systemSensor; // Inicialização da estrutura sensor.
SharedMemory shm ;

void initializeSemaphore();
void printCommands();

#endif
