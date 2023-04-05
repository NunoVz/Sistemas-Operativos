#include "HeaderFile.h"

#define SEM_KEY 1234

sem_t queue_sem; // Sem da queue interna
mqd_t mq;
int queue_front = 0;
int queue_back = 0; 

int queusize;
int nworkers;
int maxkeys;
int maxsensors;
int maxalerts;

int (*worker_pipes)[2];   //worker_pipes
char (*internal_queue)[100]; //internalqueue

mqd_t mq;





int create_sem(int key) {
    int semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget");
        exit(1);
    }
    semctl(semid, 0, SETVAL, 1);
    return semid;
}

int create_shm(int size) {
    int shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }
    return shmid;
}

char *attach_shm(int shmid) {
    char *shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (char *) -1) {
        perror("shmat");
        exit(1);
    }
    return shmaddr;
}

void detach_shm(char *shmaddr) {
    shmdt(shmaddr);
}

void destroy_sem(int semid) {
    semctl(semid, 0, IPC_RMID);
}

void destroy_shm(int shmid) {
    shmctl(shmid, IPC_RMID, NULL);
}

SharedMemory create_shared_memory(int size) {
    SharedMemory shm;
    shm.shmid = create_shm(size);
    shm.shmaddr = attach_shm(shm.shmid);
    shm.semid = create_sem(SEM_KEY);
    return shm;
}

void destroy_shared_memory(SharedMemory *shm) {
    destroy_sem(shm->semid);
    detach_shm(shm->shmaddr);
    destroy_shm(shm->shmid);
}

void lock_shared_memory(SharedMemory *shm) {
    struct sembuf sops = {0, -1, SEM_UNDO};
    semop(shm->semid, &sops, 1);
}

void unlock_shared_memory(SharedMemory *shm) {
    struct sembuf sops = {0, 1, SEM_UNDO};
    semop(shm->semid, &sops, 1);
}

void write_shared_memory(SharedMemory *shm, int value) {
    lock_shared_memory(shm);
    shm->value = value;
    unlock_shared_memory(shm);
}

int read_shared_memory(SharedMemory *shm) {
    int value;
    lock_shared_memory(shm);
    value = shm->value;
    unlock_shared_memory(shm);
    return value;
}

void add_to_queue(char *message)
{
    sem_wait(&queue_sem); // wait for access to the internal queue

    // add the message to the back of the queue
    strncpy(internal_queue[queue_back], message, sizeof(internal_queue[queue_back]));
    queue_back = (queue_back + 1) % queusize;

    sem_post(&queue_sem); // release access to the internal queue
}

char* getqueue()
{
    char *message = NULL;

    sem_wait(&queue_sem); // wait for access to the internal queue

    // check if the queue is empty
    if (queue_front != queue_back) {
        message = internal_queue[queue_front];
        queue_front = (queue_front + 1) % queusize;
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
        message = getqueue();

        if (message != NULL) { 
            printf("Dispatcher thread received message: %s\n", message);
            int worker_id = strlen(message) % nworkers;
            write(worker_pipes[worker_id][1], message, strlen(message) + 1);
        }

        usleep(100); 
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
void create_proc(void (*function)(void*), void *arg)
{
    if (fork() == 0)
    {
        // tem argumentos?
        if (arg)
            function(arg);
        else
            function(NULL);
    }
}


void worker(void* arg)
{
    int worker_id = *((int*) arg);
    free(arg); 
    writelog("WORKER UP!");

    char message[100];

    while(1) {
        read(worker_pipes[worker_id][0], &message, sizeof(message));

        printf("Worker %d received message: %s\n", worker_id, message);
        struct queuemsg my_msg;
        my_msg.mtype = 1; // Set the message type to 1
        strcpy(my_msg.mtext, "[QUEUE] Hello, world!");
        mq_send(mq, (char *) &my_msg, MAX_MSG_SIZE, 0);

    }


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
    shm = create_shared_memory(sizeof(int));
    write_shared_memory(&shm, 42);
    writelog("SHARED MEMORY INTIALIZED");
    read_conf(argv[1]);
    worker_pipes = malloc(sizeof(int[nworkers][2]));
    internal_queue = malloc(sizeof(char[queusize][100]));
    mq = create_queue(); 

    

	writelog("SYSTEM MANAGER UP!");
	pthread_t ConsoleReaderID, SensorReaderID,DispatcherID;

    bool readConsole = true;
    bool readSensor = false;
    sem_init(&queue_sem, 0, 1);

     for(int i = 0; i < nworkers; i++) {
        if(pipe(worker_pipes[i]) < 0) {
            perror("Error creating pipe");
            exit(1);
        }
    }

     for (int i = 0; i < nworkers; i++)
    {
        int* worker_id = malloc(sizeof(int));
        *worker_id = i;
        create_proc(worker, worker_id);
    }

    create_proc(alert,NULL);

    pthread_create(&ConsoleReaderID, NULL, read_thread, (void *)&readConsole);
    pthread_create(&SensorReaderID, NULL, read_thread, (void *)&readSensor);
    pthread_create(&DispatcherID, NULL, dispatcher_thread, NULL);


   

    pthread_join(ConsoleReaderID, NULL);
    pthread_join(SensorReaderID, NULL);
    pthread_join(DispatcherID, NULL);
    destroy_shared_memory(&shm);



	return 0;
}
