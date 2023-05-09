#include "HeaderFile.h"

// Variaveis Globais
FILE *log_file;
sem_t *mutex;

void initializeSemaphore()
{
	sem_unlink("MUTEX");
	mutex = sem_open("MUTEX", O_CREAT | O_EXCL, 0766, 1);
}

void init_log()
{
	if ((log_file = fopen("files//log.txt", "a+")) == NULL)
	{
		puts("[CONSOLE] Failed to open log file");
		exit(1);
	}
}
void writelog(char *message)
{
	// Get current time
	char time_s[10];
	time_t time_1 = time(NULL);
	struct tm *time_2 = localtime(&time_1);
	strftime(time_s, sizeof(time_s), "%H:%M:%S ", time_2);

	// Print and write to log file
	sem_wait(mutex);
	fprintf(log_file, "%s %s\n", time_s, message);
	printf("%s %s\n", time_s, message);
	fflush(log_file);
	fflush(stdout);
	sem_post(mutex);
}

bool validateSensor(char *inputSensor[]) {   //Método responsável por validar os inputs do Sensor.
	bool validateId = false;
	bool validateKey = false;

	if (strlen(inputSensor[1]) >= 3 && strlen(inputSensor[1]) <= 32)   //Verifica o tamanho do ID.
		validateId = true;
	

	if (strlen(inputSensor[3]) >= 3 && strlen(inputSensor[3]) <= 32)   //Verifica o tamanho da chave.
		validateKey = true;
		
	for (int i = 0; i < strlen(inputSensor[3]); i++) {   //Verifica se os caracteres da chave são válidos.
		if (!isalnum(inputSensor[3][i]) && inputSensor[3][i] != '_') {   
			validateKey = false;
		}
	}
	
	if (validateId == true && validateKey == true)
		return true;
		
	else
		return false;
}

void printCommands()
{ // Método responsável por imprimir os comandos na user console.
	printf("\nAvailable commands:\n");
	printf("exit\n");
	printf("stats\n");
	printf("reset\n");
	printf("sensors\n");
	printf("add_alert [id] [chave] [min] [max]\n");
	printf("remove_alert [id]\n");
	printf("list_alerts\n");
}




mqd_t create_queue() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
	//Meter este size a vir das configs
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    mqd_t mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, QUEUE_PERMISSIONS, &attr);
    if (mq == -1) {
        perror("mq_open");
        exit(1);
    }
    return mq;
}
