#include "HeaderFile.h"

void initializeSemaphore() {   //Método responsável por inicializar um semáforo.
	sem_unlink("MUTEX");
	
	logMutex = sem_open("Mutex", O_CREAT | O_EXCL, 0700, 1);
}

bool validateSensor(char *inputSensor[]) {   //Método responsável por validar os inputs do Sensor.
	bool validateId = false;
	bool validateKey = false;

	if (strlen(inputSensor[1]) >= 3 && strlen(inputSensor[1]) <= 32)
		validateId = true;
	
	if (strlen(inputSensor[3]) >= 3 && strlen(inputSensor[3]) <= 32)
		validateKey = true;
	
	if (validateId == true && validateKey == true)
		return true;
		
	else
		return false;
}

void printCommands() {   //Método responsável por imprimir os comandos na user console.
	printf("\nAvailable commands:" +
			"\nexit" +
			"\nstats" +
			"\nreset" +
			"\nsensors" + 
			"\nadd_alert [id] [chave] [min] [max]" + 
			"\nremove_alert [id]" +
			"\nlist_alerts\n");
}
