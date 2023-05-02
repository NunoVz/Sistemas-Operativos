#include "HeaderFile.h"

#define SEM_KEY 1234

sem_t queue_sem; // Sem da queue interna
mqd_t mq;
int queue_front = 0;
int queue_back = 0; 



int (*worker_pipes)[2];   //worker_pipes
char (*internal_queue)[100]; //internalqueue






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

SharedMemory* create_shared_memory(int nworkers) {
    size_t size = sizeof(SharedMemory) + nworkers * sizeof(int); // Allocate space for FAM
    SharedMemory* shmq = malloc(size);

    if (shmq == NULL) {
        perror("Failed to allocate memory for shared memory");
        exit(EXIT_FAILURE);
    }

    // Initialize the rest of the struct
    shmq->shmid = create_shm(size - sizeof(int) * nworkers); // Subtract FAM size from shared memory size
    shmq->shmaddr = attach_shm(shmq->shmid);
    shmq->semid = create_sem(SEM_KEY);
    memset(shmq->shmaddr, 0, size - sizeof(int) * nworkers);
    shmq->keystatsList = NULL;
    shmq->sensorList = NULL;
    shmq->alertList = NULL;

    // Initialize the FAM
    for (int i = 0; i < nworkers; i++) {
        int semwork = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);

        if (semwork < 0) {
            perror("semget() failed");
            exit(EXIT_FAILURE);
        }

        if (semctl(semwork, 0, SETVAL, 1) < 0) { // set the initial value to 1
            perror("semctl() failed");
            exit(EXIT_FAILURE);
        }

        struct sembuf sb = {0, -1, SEM_UNDO};
        semop(semwork, &sb, 1);

        shmq->semwork[i] = semwork;

        int semval = semctl(semwork, 0, GETVAL); // get the current value of the semaphore
        if (semval < 0) {
            perror("semctl() failed");
            exit(EXIT_FAILURE);
        }

    }

    return shmq;
}

void destroy_shared_memory(SharedMemory *shm) {
    destroy_sem(shm->semid);
    detach_shm(shm->shmaddr);
    destroy_shm(shm->shmid);
}

void lock_shared_memory() {
    struct sembuf sops = {0, -1, SEM_UNDO};
    semop(shm_ptr->semid, &sops, 1);
}

void unlock_shared_memory() {
    struct sembuf sops = {0, 1, SEM_UNDO};
    semop(shm_ptr->semid, &sops, 1);
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

            // Traverse the worker list to find an available worker
            bool flagw=true;
            while(flagw){
                for (int i = 0; i < nworkers; i++) {
                    // Get the current value of the semaphore associated with this worker
                    lock_shared_memory();
                    int semval = semctl(shm_ptr->semwork[i], 0, GETVAL);
                    unlock_shared_memory();
                    if (semval == 0 && worker_pipes[i][0] != -1) {

                        write(worker_pipes[i][1], message, strlen(message) + 1);

                        printf("Message sent to worker %d\n", i);

                        flagw = false;
                        break;
                    }
                }
                    
                
                if(flagw==true){
                    printf("Nenhum worker disponivel vamos percorrer outra vez....\n");
                }
            }
            
            
        
      

            
        }

        usleep(100); // Wait a bit before checking for new messages
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

    if ((config = fopen(filename, "r")) == NULL)
    {
        writelog("[SYSTEM] Failed to open config file");
        exit(1);
    }
    while ((read = getline(&line, &len, config)) != -1)
    {
        if(line_id==5){
            printf("End of file reached, line:%s\n",line);
            break;

        }
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

            fflush(stdout);
            if (number < 0)
                {
                    writelog("[SYSTEM] Number of max alerts is too low");

                    exit(1);
                }
            maxalerts= number;
            line_id++;

           break;
        }
    }

    printf("Final: \n Queue Size: %d \n Workers: %d\n,Keys:%d\n,Sensors:%d\n,Alerts:%d\n", queusize,nworkers,maxkeys,maxsensors,maxalerts);
    fflush(stdout);
    fclose(config);
    if (line)
        free(line);
    if(line_id <5){
        writelog("[SYSTEM] ConfigFile does not have enought information!");
        exit(1);
    }
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
bool getlistsizes(int x){
    //X is 0 checks size of alert size
    //X is 1 checks size of keystatsList
    //X is 2 checks size of sensorList
    int size = 0;

    if(x==0){
        alertStruct *currentNode = shm_ptr->alertList;
        while (currentNode != NULL) {
            size++;
            currentNode = currentNode->next;
        }
        if(size>=maxalerts){
            return false;
        }
        return true;
    }else if(x==1){
        keyStats *currentNode = shm_ptr->keystatsList;
        while (currentNode != NULL) {
            size++;
            currentNode = currentNode->next;
        }
        if(size>=maxkeys){
            return false;
        }
        return true;

    }else if(x==2){
        sensor *currentNode = shm_ptr->sensorList;
        while (currentNode != NULL) {
            size++;
            currentNode = currentNode->next;
        }
        if(size>=maxsensors){
            return false;
        }
        return true;

    }else{
        return false;
    }

}
int addSensor(SharedMemory *shm, const char *sensorId) {
    // Check if sensorId already exists
    lock_shared_memory(shm);
    if(!getlistsizes(2)){
        writelog("Couldnt add new sensor");
    }else{
        sensor *s = shm->sensorList;
        while (s != NULL) {
            if (strcmp(s->sensorId, sensorId) == 0) {
                // Sensor already exists, return failure
                unlock_shared_memory(shm);

                return -1;
            }
            if (s->next == NULL) {
                break;
            }
            s = s->next;
        }

        // Sensor doesn't exist, add new sensor to the list
        sensor *newSensor = (sensor *)malloc(sizeof(sensor));
        strcpy(newSensor->sensorId, sensorId);
        newSensor->next = NULL;
        if (s == NULL) {
            shm->sensorList = newSensor;
        } else {
            s->next = newSensor;
        }
    }
    unlock_shared_memory(shm);

    return 0;
}

// Function to add a new keystats to the shared memory
int addKeystats(SharedMemory *shm, char *key, int value) {
    lock_shared_memory(shm);
    
    // Check if keystats with the same key already exists
    keyStats *k = shm->keystatsList;


    while (k != NULL) {
        fflush(stdout);

        if (k->key != NULL && strcmp(k->key, key) == 0) {
            // Keystats already exists, update values
            if (k->minValue > value) {
                k->minValue = value;
            }
            if (k->maxValue < value) {
                k->maxValue = value;
            }
            k->count++;
            k->avg = ((k->avg * (k->count - 1)) + value) / k->count;
            k->last = value;
            unlock_shared_memory(shm);

            return 1;
        }
        k = k->next;
    }

    if(!getlistsizes(1)){
            writelog("Couldnt add new key");
    }else{
        // Keystats doesn't exist, add new keystats to the list
        keyStats *newKeystats = (keyStats *) malloc(sizeof(keyStats));
        printf("%d",value);
        strcpy(newKeystats->key, key);
        newKeystats->last = value;
        newKeystats->minValue = value;
        newKeystats->maxValue = value;
        newKeystats->count = 1;
        newKeystats->avg = ((newKeystats->avg * (newKeystats->count - 1)) + value) / newKeystats->count;;
        newKeystats->next = NULL;


        if (k == NULL) {
            // The list is empty
            shm->keystatsList = newKeystats;
        } else {
            // Add newKeystats to the end of the list
            k->next = newKeystats;
        }
    }
    unlock_shared_memory(shm);


    return 0;
}

void printKeystatsList(SharedMemory *shm) {

    lock_shared_memory(shm);

    keyStats *k = shm->keystatsList;

    printf("KEY\t| LAST\t| MINVALUE\t| MAXVALUE\t| AVG\t| COUNT\n");

    printf("-----------------------------------------------------------\n");

    while (k != NULL) {

        printf("%s\t| %d\t| %d\t\t| %d\t\t| %d\t| %d\n", k->key, k->last, k->minValue, k->maxValue, k->avg, k->count);

        k = k->next;

    }
    printf("-----------------------------------------------------------\n");


    unlock_shared_memory(shm);

}
void printSensors(SharedMemory *shm) {

    lock_shared_memory(shm);

    sensor *k = shm->sensorList;

    printf("SENSOR ID\t|\n");

    printf("-----------------------------------------------------------\n");

    while (k != NULL) {

        printf("%s\t|\n", k->sensorId);

        k = k->next;

    }
    printf("-----------------------------------------------------------\n");


    unlock_shared_memory(shm);

}


void addAlertToList(char* id, char* key, int minValue, int maxValue, SharedMemory* shm) {
    lock_shared_memory(shm);
    
    alertStruct *currentNode = shm->alertList;
    while (currentNode != NULL) {
        if (strcmp(currentNode->id, id) == 0) {
            printf("Error: Alert with ID %s already exists.\n", id);
            unlock_shared_memory(shm);
            return;
        }
        currentNode = currentNode->next;
    }
    if(!getlistsizes(0)){
            writelog("Couldnt add new Alert");
    }else{
        // Create a new alertStruct node and initialize its fields
        alertStruct *alertNode = malloc(sizeof(alertStruct));
        strcpy(alertNode->id, id);
        strcpy(alertNode->key, key);
        alertNode->minValue = minValue;
        alertNode->maxValue = maxValue;
        alertNode->next = NULL;

        // If the alertList is empty, set the alertNode as the head
        if (shm->alertList == NULL) {
            shm->alertList = alertNode;
            unlock_shared_memory(shm);
            return;
        }

        // Traverse the list to find the end
        currentNode = shm->alertList;
        while (currentNode->next != NULL) {
            currentNode = currentNode->next;
        }

        // Add the alertNode to the end of the list
        currentNode->next = alertNode;
    }
    unlock_shared_memory(shm);

}




void deleteAlertFromList(char *id, SharedMemory *shm) {
    lock_shared_memory(shm);

    if (shm->alertList == NULL) {
        unlock_shared_memory(shm);
        return;
    }

    if (strcmp(shm->alertList->id, id) == 0) {
        alertStruct *temp = shm->alertList;
        shm->alertList = shm->alertList->next;
        free(temp);
        unlock_shared_memory(shm);
        return;
    }

    alertStruct *currentNode = shm->alertList;
    while (currentNode->next != NULL && strcmp(currentNode->next->id, id) != 0) {
        currentNode = currentNode->next;
    }

    if (currentNode->next == NULL) {
        unlock_shared_memory(shm);
        return;
    }

    alertStruct *temp = currentNode->next;
    currentNode->next = currentNode->next->next;
    free(temp);
    unlock_shared_memory(shm);

}




void printAlerts(SharedMemory *shm) {

    lock_shared_memory(shm);

    alertStruct *k = shm->alertList;

    printf("ID\t| KEY\t| MINVALUE\t| MAXVALUE\t|\n");

    printf("-----------------------------------------------------------\n");

    while (k != NULL) {

        printf("%s\t| %s\t| %d\t\t| %d\t\t|\n", k->id, k->key, k->minValue, k->maxValue);

        k = k->next;

    }
    printf("-----------------------------------------------------------\n");


    unlock_shared_memory(shm);

}

void worker(void* arg)
{
    worker_t worker = *((worker_t*) arg);
    int worker_id= worker.i;
    free(arg); 
    writelog("WORKER UP!");
    printf("WORKER UP %d\n",worker_id);




    char message[100];
   

    while(1) {

        read(worker_pipes[worker_id][0], &message, sizeof(message));

        struct sembuf sb = {0, 0, SEM_UNDO}; // Check semaphore value
        lock_shared_memory();
        int semval = semctl(shm_ptr->semwork[worker_id], 0, GETVAL); // Get current semaphore value
        if (semval == 0) {
            sb.sem_op = 1; // Increment semaphore value by 1
            if (semop(shm_ptr->semwork[worker_id], &sb, 1) < 0) { // Lock semaphore
                perror("semop() failed");
                exit(EXIT_FAILURE);
            }
        }
        unlock_shared_memory();

        printf("\nWorker: %d BUSY\n",worker_id);

        if (message[strlen(message) - 1] == '\n') {
            message[strlen(message) - 1] = '\0';
        }
        char msg_copy[100];
        strcpy(msg_copy, message);
        char *word = strtok(message, " ");
        printf("Worker %d received message: %s\n", worker_id, message);

        if (strcmp(word, "stats") == 0) {
            printKeystatsList(shm_ptr);
            printf("Stats command detected\n");
        } else if (strcmp(word, "reset") == 0) {
            lock_shared_memory();
            keyStats *curr = shm_ptr->keystatsList;
            while (curr != NULL) {
                keyStats *temp = curr;
                curr = curr->next;
                free(temp);
            }
            shm_ptr->keystatsList = NULL;
            unlock_shared_memory();
            printf("Reset command detected\n");
        } else if (strcmp(word, "sensors") == 0) {
            printSensors(shm_ptr);
            printf("Sensors command detected\n");
        } else if (strcmp(word, "add_alert") == 0) {
            char command[20], id[20], key[20];
            int min, max;
            //Checka se tem os params corretos
            printf("Mess: %s\n",message);

            if (sscanf(msg_copy, "%s %s %s %d %d", command, id, key, &min, &max) == 5) {
                addAlertToList(id,key,min,max,shm_ptr);
            } else {
                printf("Invalid input format for add_alert\n");
            }
            printf("Add alert command detected\n");
        } else if (strcmp(word, "remove_alert") == 0) {
            char command[20], id[20];
            if (sscanf(msg_copy, "%s %s", command, id) == 2) {
                deleteAlertFromList(id,shm_ptr);
            } else {
                printf("Invalid input format for add_alert\n");
            }
            printf("Remove alert command detected\n");
        } else if (strcmp(word, "list_alerts") == 0) {
            printAlerts(shm_ptr);
            printf("List alerts command detected\n");
        } else {
            char *sensor_id, *key, *value;

            int count = 0;
            for (int i = 0; i < strlen(message); i++) {
                if (message[i] == '#') {
                    count++;
                }
            }
            if (count == 2) {
                sensor_id = strtok(message, "#");
                key = strtok(NULL, "#");
                value = strtok(NULL, "#");

      

                addSensor(shm_ptr,sensor_id);
                
                addKeystats(shm_ptr,key,atoi(value));
            } else {
                printf("Invalid command\n");
            }
        }

        
        struct sembuf sbt = {0, -1, SEM_UNDO}; // Decrement semaphore value by 1
        lock_shared_memory();
        if (semop(shm_ptr->semwork[worker_id], &sbt, 1) < 0) { // Unlock semaphore
            perror("semop() failed");
            exit(EXIT_FAILURE);
        }
        unlock_shared_memory();
        struct queuemsg my_msg;
        my_msg.mtype = 1; 
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
	if (argc != 2) {
        fprintf(stderr, "system {ficheiro de configuração}");
        exit(1);
    }

    initializeSemaphore();
    init_log();

    read_conf(argv[1]);

    shm_ptr = create_shared_memory(sizeof(int));

    writelog("SHARED MEMORY INTIALIZED");
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

    worker_t* current = NULL;
    for (int i = 0; i < nworkers; i++)
    {
        worker_t* new_worker = (worker_t*) malloc(sizeof(worker_t));
        new_worker->i = i;
        new_worker->status = true;
        new_worker->next = NULL;

        create_proc(worker, new_worker);
    }

    create_proc(alert,NULL);



    
    pthread_create(&ConsoleReaderID, NULL, read_thread, (void *)&readConsole);
    pthread_create(&SensorReaderID, NULL, read_thread, (void *)&readSensor);
    pthread_create(&DispatcherID, NULL, dispatcher_thread, NULL);

   

    pthread_join(ConsoleReaderID, NULL);
    pthread_join(SensorReaderID, NULL);
    pthread_join(DispatcherID, NULL);
    destroy_shared_memory(shm_ptr);



	return 0;
}
