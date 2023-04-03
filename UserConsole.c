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

	while (flag)
	{

		printCommands();					// São imprimidos os comandos disponíveis na user console.
		fgets(input, sizeof(input), stdin); // read input from user

		if (strcmp(input, "Exit\n") == 0)
		{

			break;
		}

		write(fd, input, strlen(input) + 1); // write the input to the named pipe
	}
	close(fd); // close the file descriptor

	return 0;
}
