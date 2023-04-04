#include "HeaderFile.h"

#define MAX_QUEUE_SIZE 100

sem_t queue_sem; // semaphore to ensure exclusive access to the internal queue

char internal_queue[MAX_QUEUE_SIZE][100]; // internal queue to store received messages
int queue_front = 0; // index of the front of the queue
int queue_back = 0; // index of the back of the queue

int queusize;
int nworkers;
int maxkeys;
int maxsensors;
int maxalerts;

void add_to_queue(char *message)
{
    sem_wait(&queue_sem); // wait for access to the internal queue

    // add the message to the back of the queue
    strncpy(internal_queue[queue_back], message, sizeof(internal_queue[queue_back]));
    queue_back = (queue_back + 1) % MAX_QUEUE_SIZE;

    sem_post(&queue_sem); // release access to the internal queue
}

char* getqueue()
{
    char *message = NULL;

    sem_wait(&queue_sem); // wait for access to the internal queue

    // check if the queue is empty
    if (queue_front != queue_back) {
        message = internal_queue[queue_front];
        queue_front = (queue_front + 1) % MAX_QUEUE_SIZE;
    }

    sem_post(&queue_sem); // release access to the internal queue

    return message;
}


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
    while (read(fd, buffer, sizeof(buffer)) > 0) { 
        add_to_queue(buffer); 
    }
    close(fd); 

    pthread_exit(NULL);
}
void *dispatcher_thread(void *arg)
{
    char *message;
    writelog("THREAD dispatcher READER UP!");

    while (true) {
        message = getqueue(); // get a message from the internal queue

        if (message != NULL) { // if a message was retrieved
            // process the message (replace with your own processing code)
            printf("Dispatcher thread received message: %s\n", message);
        }

        usleep(100); // sleep for a short time to avoid busy-waiting
    }
}
int read_conf(char *filename)
{
    FILE *config;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int line_id = 0;
    int number;

    char *token;
    if ((config = fopen(filename, "r")) == NULL)
    {
        writelog("[SYSTEM] Failed to open config file");
        exit(1);
    }
    while ((read = getline(&line, &len, config)) != -1)
    {

        switch (line_id)
        {
        case (0):
            number = atoi(line);
            if (number < 1)
            {
                writelog("[SYSTEM] Number of max queue size is too low");

                exit(1);
            }
            queusize = number;
            line_id++;
            break;

        case (1):
            number = atoi(line);
            if (number < 1)
            {
                writelog("[SYSTEM] Number of max workers is too low");

                exit(1);
            }
            nworkers = number;
            line_id++;
            break;
        case (2):
            number = atoi(line);
            maxkeys = number;

            if (number < 1)
            {
                writelog("[SYSTEM] Number of max keys is too low");

                exit(1);
            }
            line_id++;
            break;
        case (3):
            number = atoi(line);
           if (number < 1)
            {
                writelog("[SYSTEM] Number of max sensores is too low");

                exit(1);
            }
           maxsensors= number;
            line_id++;

           break;
        
        case (4):
            number = atoi(line);

           if (number < 1)
            {
                writelog("[SYSTEM] Number of max alerts is too low");

                exit(1);
            }
           maxalerts= number;
            line_id++;

           break;
        }
    }


    fclose(config);
    if (line)
        free(line);
    return 1;
}
void create_proc(void (*function)(), void *arg)
{
    if (fork() == 0)
    {
        // tem argumentos?
        if (arg)
            function(arg);
        else
            function();
    }
}
void worker()
{
    	writelog("WORKER UP!");
        exit(0);

}
void alert()
{
    	writelog("ALERT WATCHER UP!");

        exit(0);

}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "system {ficheiro de configuração}");
	}
	init_log();
	initializeSemaphore();
    read_conf(argv[1]);
    

	writelog("SYSTEM MANAGER UP!");
	pthread_t ConsoleReaderID, SensorReaderID,DispatcherID;

    bool readConsole = true;
    bool readSensor = false;
    sem_init(&queue_sem, 0, 1);

     for (int i = 0; i < nworkers; i++)
    {
        create_proc(worker, NULL);
    }

    create_proc(alert,NULL);

    pthread_create(&ConsoleReaderID, NULL, read_thread, (void *)&readConsole);
    pthread_create(&SensorReaderID, NULL, read_thread, (void *)&readSensor);
    pthread_create(&DispatcherID, NULL, dispatcher_thread, NULL);


   

    pthread_join(ConsoleReaderID, NULL); // wait for the threads to finish
    pthread_join(SensorReaderID, NULL);
    pthread_join(DispatcherID, NULL);


	return 0;
}
