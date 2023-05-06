#include "HeaderFile.h"

#define FIFO_NAME "/tmp/sensorfifo" // name of the named pipe

int main(int argc, char *argv[])
{
	if (argc != 6)
	{
		fprintf(stderr, "sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {valor inteiro mínimo a ser enviado} {valor inteiro máximo a ser enviado}");

		return -1;
	}

	if (validateSensor(argv) != true)
	{ // São verificados o Id e a Key do Sensor e é criada uma estrutura sensor.
		fprintf(stderr, "Valores Incorretos\n");
		exit(1);
		
	}
	char sensorId[32];
	int sendInterval;
	char sensorKey[32];
	int minValue;
	int maxValue;
	strcpy(sensorId, argv[1]);
	sendInterval = atoi(argv[2]);
	strcpy(sensorKey, argv[3]);
	minValue = atoi(argv[4]);
	maxValue = atoi(argv[5]);
	int fd;
	mkfifo(FIFO_NAME, 0666); // create the named pipe

	fd = open(FIFO_NAME, O_WRONLY); // open the named pipe for writing

	srand(time(NULL));

	while(true){
		
        char input[100];

		int value =  minValue + ((float) rand() / RAND_MAX) * (maxValue  - minValue);
		snprintf(input, sizeof(input), "S %s#%s#%d", sensorId, sensorKey, value);
		write(fd, input, strlen(input) + 1);
		sleep(sendInterval);
	}
	close(fd);


	return 0;
}
