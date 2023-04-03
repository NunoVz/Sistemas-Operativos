#include "HeaderFile.h"


void *read_thread(void *arg)
{
    bool read_from_pipe = *(bool*) arg;
    char *fifo_name = read_from_pipe ? "/tmp/consolefifo" : "/tmp/sensorfifo";
    int fd;
    char buffer[100];
    mkfifo(fifo_name, 0666);
	if(read_from_pipe){
		writelog("THREAD CONSOLE READER UP!");

	}else{
		writelog("THREAD SENSOR READER UP!");
	}

    fd = open(fifo_name, O_RDONLY);
    read(fd, buffer, sizeof(buffer)); 
    printf("Received command: %s\n", buffer); 
    close(fd); 

    pthread_exit(NULL);
}
int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "system {ficheiro de configuração}");
	}
	init_log();
	initializeSemaphore();

	writelog("SYSTEM MANAGER UP!");
	pthread_t thread_id1, thread_id2;

    bool read_from_pipe1 = true;
    bool read_from_pipe2 = false;

    pthread_create(&thread_id1, NULL, read_thread, (void *)&read_from_pipe1);
    pthread_create(&thread_id2, NULL, read_thread, (void *)&read_from_pipe2);

    // rest of your code goes here

    pthread_join(thread_id1, NULL); // wait for the threads to finish
    pthread_join(thread_id2, NULL);

	return 0;
}
