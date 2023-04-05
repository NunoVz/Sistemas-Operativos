#include "HeaderFile.h"

#define FIFO_NAME "/tmp/consolefifo" // name of the named pipe

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
	strcpy(systemSensor.sensorId, argv[1]);
	systemSensor.sendInterval = atoi(argv[2]);
	strcpy(systemSensor.sensorKey, argv[3]);
	systemSensor.minValue = atoi(argv[4]);
	systemSensor.maxValue = atoi(argv[5]);
	int fd;
	mkfifo(FIFO_NAME, 0666); // create the named pipe

	fd = open(FIFO_NAME, O_WRONLY); // open the named pipe for writing

	while(true){
		char input[]="Mensagem toda randolas do sensor";
		write(fd, input, strlen(input) + 1);
		sleep(3);
	}
	close(fd);

	




	return 0;
}
