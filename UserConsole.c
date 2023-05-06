#include "HeaderFile.h"

#define FIFO_NAME "/tmp/consolefifo" // name of the named pipe

void *listen_to_queue(void *arg) {
    int identifier = *(int*) arg;
    mqd_t mq = create_queue();
    struct queuemsg my_msg;

    while (true) {
        if (mq_receive(mq, (char *) &my_msg, MAX_MSG_SIZE, NULL) == -1) {
            perror("Error receiving message from queue");
            return NULL;
        }

        if (my_msg.mtype == identifier) {
            printf("Received message:\n %s\n", my_msg.mtext);
        }
    }

    mq_close(mq);
    return NULL;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "user_console {identificador da consola}");

		return -1;
	}


	int identifier = atoi(argv[1]);

    if (identifier <= 0) {
        fprintf(stderr, "O identificador deve ser maior que 0.\n");
        return -1;
    }
	pthread_t thread;
	int rc = pthread_create(&thread, NULL, listen_to_queue, (void *) &identifier);
	if (rc) {
		fprintf(stderr, "Error creating thread\n");
		return -1;
	}

	bool flag = true;
	int fd;
	mkfifo(FIFO_NAME, 0666); // create the named pipe

	char input[75];  //FIX THIS
	char output[100];
	fd = open(FIFO_NAME, O_WRONLY); // open the named pipe for writing
  

	mqd_t mq = create_queue();
	printCommands();	
	while (flag)
	{

						// São imprimidos os comandos disponíveis na user console.
		fgets(input, sizeof(input), stdin); // read input from user

		if (strcmp(input, "Exit\n") == 0)
		{

			break;
		}
		sprintf(output, "%d %s", identifier, input);

		write(fd, output, strlen(output) + 1); // write the input to the named pipe

		
	}
	close(fd); // close the file descriptor
	mq_close(mq);


	return 0;
}
