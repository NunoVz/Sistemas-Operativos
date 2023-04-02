#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <semaphore.h>

sem_t *logMutex;   //Semáforo que será usado para controlar o acesso a um recurso compartilhado com outros processos/threads.

typedef struct sensor {
	char sensorId[32];
	int sendInterval;
	char sensorKey[32];
	int minValue;
	int maxValue;
} sensor

sensor systemSensor;   //Inicialização da estrutura sensor.

void initializeSemaphore();
void printCommands();
