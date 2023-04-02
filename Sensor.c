#include "HeaderFile.h"

int main(int argc, char *argv[]) {
	if (argc != 6) {
		fprintf(stderr, "sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {valor inteiro mínimo a ser enviado} {valor inteiro máximo a ser enviado}");
		
		return -1;
	}
	
	initializeSemaphore();
	
	if (validateSensor(argv) == true) {   //São verificados o Id e a Key do Sensor e é criada uma estrutura sensor.
		strcpy(systemSensor.sensorId, argv[1]);
		systemSensor.sendInterval = atoi(argv[2]);
		strcpy(systemSensor.sensorKey, argv[3]);
		systemSensor.minValue = atoi(argv[4]);
		systemSensor.maxValue = atoi(argv[5]);
	}
	
	return 0;
}
