#include "HeaderFile.h"

#define FIFO_NAME "/tmp/consolefifo" // name of the named pipe



int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "user_console {identificador da consola}");

		return -1;
	}

	bool flag = true;
	int fd;
	mkfifo(FIFO_NAME, 0666); // create the named pipe

	char input[100];
	fd = open(FIFO_NAME, O_WRONLY); // open the named pipe for writing
  

    char msg_text[MAX_MSG_SIZE];
	mqd_t mq = create_queue();

	while (flag)
	{

		printCommands();					// São imprimidos os comandos disponíveis na user console.
		fgets(input, sizeof(input), stdin); // read input from user

		if (strcmp(input, "Exit\n") == 0)
		{

			break;
		}

		write(fd, input, strlen(input) + 1); // write the input to the named pipe

		struct queuemsg my_msg;
		if (mq_receive(mq, (char *) &my_msg, MAX_MSG_SIZE, NULL) == -1) {
			perror("Error receiving message from queue");
			return -1;
    	}



        printf("Received message: %s\n", my_msg.mtext);
	}
	close(fd); // close the file descriptor
	mq_close(mq);


	return 0;
}
