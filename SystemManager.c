#include "HeaderFile.h"

#define SEM_KEY 1234

sem_t queue_sem; // Sem da queue interna
mqd_t mq;
int queue_front = 0;
int queue_back = 0; 



int (*worker_pipes)[2];   //worker_pipes
char (*internal_queue)[100]; //internalqueue



//O meu teclado algumas teclas nao funfam vou deixar aqui caso necessite
// _ P = p 0


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

SharedMemory *attach_shm(int shmid) {
    char *shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (SharedMemory *) -1) {
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

SharedMemory* create_shared_memory() {
    size_t size = sizeof(SharedMemory) + (nworkers * sizeof(int)); 

    
    printf("Workers: %d\n",nworkers);

    // Initialize the rest of the struct
    int shmid = create_shm(size - sizeof(int) * nworkers); 
    SharedMemory* shmq = attach_shm(shmid);
    shmq->semid = create_sem(SEM_KEY);
    memset(shmq, 0, size - sizeof(int) * nworkers);
    shmq->contKey=0;
    shmq->contSen=0;
    shmq->contAlert=0;


    printf("\n%d\n",maxkeys);
    shmq->keystatsList = (keyStats *)(((void *)shmq )+  sizeof(SharedMemory)); // Allocate memory for keystatsList
    
    shmq->alertList = (alertStruct * )(((void *) shmq->keystatsList) + maxkeys*sizeof(keyStats));
    
    shmq->sensorList = (sensor * )(((void *) shmq->alertList) + maxalerts*sizeof(alertStruct));
    
    //memset(shmq->keystatsList, 0, maxkeys * sizeof(keyStats));
    //memset(shmq->alertList, 0, maxalerts * sizeof(keyStats));
    
    shmq->semwork = malloc(nworkers * sizeof(int)); // Dynamically allocate semwork array
    if (shmq->semwork == NULL) {
        perror("Failed to allocate memory for semwork");
        exit(EXIT_FAILURE);
    }
    // Initialize the FAM
    for (int i = 0; i < nworkers; i++){
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

        printf("Semaphore %d value: %d\n", semwork, semval);
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


void add_to_queue(char* message) {
    sem_wait(&queue_sem); 

    if ((queue_back + 1) % queusize == queue_front) {
        writelog("Message wasnt added because the internal queue size was reached!");
    } else {
        strncpy(internal_queue[queue_back], message, sizeof(internal_queue[queue_back]));
        queue_back = (queue_back + 1) % queusize;
    }

    sem_post(&queue_sem); 
}

char* getqueue() {
    char* message = NULL;
    int i;

    sem_wait(&queue_sem); 

    //We iterate the queue to find a message that is from the user console
    if (queue_front != queue_back) {
        for (i = queue_front; i != queue_back; i = (i + 1) % queusize) {
            if (internal_queue[i][0] > '1' && internal_queue[i][0] <= '9') {
                message = internal_queue[i];
                queue_front = (i + 1) % queusize;
                break;
            }
        }
    //If there is none we get the first sensor command
        if (message == NULL) {
            message = internal_queue[queue_front];
            queue_front = (queue_front + 1) % queusize;
        }
    }

    sem_post(&queue_sem); 

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
                        printf("Message sent to worker %d is semval %d\n", i,semval);
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
        if(line_id>5){
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

    if (x == 1) {  
        alertStruct *a = shm_ptr->alertList;
        while (a != NULL) {
            size++;
            a = a->next;
        }
    
    } else if (x == 2) {  
        keyStats *k = shm_ptr->keystatsList;
        while (k != NULL) {
            size++;
            k = k->next;
        }
    } else if (x == 3) {  
        sensor *s = shm_ptr->sensorList;
        while (s != NULL) {
            size++;
            s = s->next;
        }
    }

    return size;


}
int addSensor(SharedMemory *shm,  char *sensorId) {
    lock_shared_memory(shm);

    // Check if sensor with the same sensorId already exists
    for (int i = 0; i < shm->contSen; i++) {
        if (strcmp(shm->sensorList[i].sensorId, sensorId) == 0) {
            // Sensor already exists, return failure
            unlock_shared_memory(shm);
            return -1;
        }
    }

    if (getlistsizes(2) == 0) {
        writelog("Couldnt add new sensor");
    } else {
        // Sensor doesn't exist, add new sensor to the end of the list
        int index = shm->contSen;
        // Initialize the new sensor
        strcpy(shm->sensorList[index].sensorId, sensorId);
        shm->sensorList[index].next = NULL;
        if (index != 0) {
            shm->sensorList[index - 1].next = &shm->sensorList[index];
        }
        shm->contSen++;
    }

    unlock_shared_memory(shm);
    return 0;
}


// Function to add a new keystats to the shared memory
int addKeystats(SharedMemory *shm, char *key, int value) {
    lock_shared_memory(shm);

    // Check if keystats with the same key already exists
    
    for(int i =0;i<shm->contKey;i++) {
        if (strcmp(shm->keystatsList[i].key, key) == 0) {
            // Keystats already exists, update values
            if (shm->keystatsList[i].minValue > value) {
                shm->keystatsList[i].minValue = value;
            }
            if (shm->keystatsList[i].maxValue < value) {
                shm->keystatsList[i].maxValue = value;
            }
            shm->keystatsList[i].count++;
            shm->keystatsList[i].avg = ((shm->keystatsList[i].avg * (shm->keystatsList[i].count - 1)) + value) /shm->keystatsList[i].count;
            shm->keystatsList[i].last = value;
            shm->keystatsList[i].verified = false;
            unlock_shared_memory(shm);
            return 1;
        }
    }

    if (getlistsizes(1) == 0) {
        writelog("Couldnt add new key");
    } else {
        // Keystats doesnt exist, add new keystats to the end of the list
        int index = shm->contKey;
        // Initialize the new keystats
        strcpy(shm->keystatsList[index].key, key);
        shm->keystatsList[index].last = value;
        shm->keystatsList[index].minValue = value;
        shm->keystatsList[index].maxValue = value;
        shm->keystatsList[index].count = 1;
        shm->keystatsList[index].avg = value;
        shm->keystatsList[index].verified = false;

        if(index!=1){
            shm->keystatsList[index-1].next = &shm->keystatsList[index];
        }
        shm->contKey++;
        printf("Dei add em :%d\n",index);
      
    }

    unlock_shared_memory(shm);
    return 0;
}





char *generateKeystatsListOutput(SharedMemory *shm) {
    lock_shared_memory(shm);


    // Estimate output size
    ssize_t output_size = 0;


    for(int i =0;i<shm->contKey;i++) {
 
        output_size += snprintf(NULL, 0, "%s\t| %d\t| %d\t\t| %d\t\t| %d\t| %d\n",

                    shm->keystatsList[i].key, shm->keystatsList[i].last, shm->keystatsList[i].minValue, shm->keystatsList[i].maxValue, shm->keystatsList[i].avg, shm->keystatsList[i].count);

    }

    output_size += strlen("KEY\t| LAST\t| MINVALUE\t| MAXVALUE\t| AVG\t| COUNT\n");
    output_size += strlen("-----------------------------------------------------------\n");
    output_size += strlen("-----------------------------------------------------------\n");

    // Allocate memory for output string

    char *output = malloc(output_size + 1);

    if (output == NULL) {

        unlock_shared_memory(shm);

        return NULL;

    }

    // Generate output
    int pos = sprintf(output, "KEY\t| LAST\t| MINVALUE\t| MAXVALUE\t| AVG\t| COUNT\n");
    pos += sprintf(output + pos, "-----------------------------------------------------------\n");

    for(int i =0;i<shm->contKey;i++) {
        pos += sprintf(output + pos, "%s\t| %d\t| %d\t\t| %d\t\t| %d\t| %d\n",
                       shm->keystatsList[i].key, shm->keystatsList[i].last, shm->keystatsList[i].minValue, shm->keystatsList[i].maxValue, shm->keystatsList[i].avg, shm->keystatsList[i].count);
    }
    pos += sprintf(output + pos, "-----------------------------------------------------------\n");
    output[pos] = '\0';

    unlock_shared_memory(shm);
    return output;
}


void printKeystatsList(SharedMemory *shm) {
    lock_shared_memory(shm);


    for(int i=0 ; i < shm->contKey;i++ ){
        printf("Key: %s\n", shm->keystatsList[i].key);
        printf("\tLast Value: %d\n", shm->keystatsList[i].last);
        printf("\tMinimum Value: %d\n", shm->keystatsList[i].minValue);
        printf("\tMaximum Value: %d\n", shm->keystatsList[i].maxValue);
        printf("\tAverage Value: %d\n", shm->keystatsList[i].avg);
        printf("\tValue Count: %d\n", shm->keystatsList[i].count);
    }


    unlock_shared_memory(shm);

}



char* generateSensorsOutput(SharedMemory* shm) {
    lock_shared_memory(shm);

    // Estimate output size
    ssize_t output_size = 0;

    for (int i = 0; i < shm->contSen; i++) {
        output_size += strlen(shm->sensorList[i].sensorId) + 3; // +3 for tab and vertical bar characters
    }

    output_size += strlen("SENSOR ID\t|\n");
    output_size += strlen("-----------------------------------------------------------\n");
    output_size += strlen("-----------------------------------------------------------\n");

    // Allocate memory for output string
    char* output = malloc(output_size + 1);
    if (output == NULL) {
        unlock_shared_memory(shm);
        return NULL;
    }

    // Generate output
    int pos = sprintf(output, "SENSOR ID\t|\n");
    pos += sprintf(output + pos, "-----------------------------------------------------------\n");

    for (int i = 0; i < shm->contSen; i++) {
        pos += sprintf(output + pos, "%s\t|\n", shm->sensorList[i].sensorId);
    }

    pos += sprintf(output + pos, "-----------------------------------------------------------\n");
    output[pos] = '\0';

    unlock_shared_memory(shm);
    return output;
}





void addAlertToList(char* id, char* key, int minValue, int maxValue, SharedMemory* shm, int user) {

    lock_shared_memory(shm);
    
    // Check if alert with the same id already exists
    for (int i = 0; i < shm->contAlert; i++) {
        if (strcmp(shm->alertList[i].id, id) == 0) {
            printf("Error: Alert with ID %s already exists.\n", id);
            unlock_shared_memory(shm);
            return;
        }
    }

   
        // Add new alert to the end of the alertList
        int index = shm->contAlert;
        // Initialize the new alert
        strcpy(shm->alertList[index].id, id);
        strcpy(shm->alertList[index].key, key);
        shm->alertList[index].minValue = minValue;
        shm->alertList[index].maxValue = maxValue;
        shm->alertList[index].myuser = user;
        if(index!=1){
            shm->alertList[index-1].next = &shm->alertList[index];
        }
        shm->contAlert++;
    

    unlock_shared_memory(shm);
}


void deleteAlertFromList(char* id, SharedMemory* shm) {
    lock_shared_memory(shm);

    if (shm->alertList == NULL) {
        unlock_shared_memory(shm);
        return;
    }

    int i;
    for (i = 0; i < shm->contAlert; i++) {
        if (strcmp(shm->alertList[i].id, id) == 0) {
            // found the alert, move the remaining alerts one position left in the array
            for (int j = i; j < shm->contAlert - 1; j++) {
                shm->alertList[j] = shm->alertList[j+1];
            }
            shm->contAlert--;
            // clear the last alert in the array
            memset(&shm->alertList[shm->contAlert], 0, sizeof(alertStruct));
            break;
        }
    }

    unlock_shared_memory(shm);
}


char* generateAlertOutput(SharedMemory *shm) {
    lock_shared_memory(shm);

    // Estimate output size
    ssize_t output_size = 0;
    for(int i = 0; i < shm->contAlert; i++) {
        output_size += snprintf(NULL, 0, "%s\t| %s\t| %d\t\t| %d\t\t|\n",
                                shm->alertList[i].id, shm->alertList[i].key,
                                shm->alertList[i].minValue, shm->alertList[i].maxValue);
    }

    output_size += strlen("ID\t| KEY\t| MINVALUE\t| MAXVALUE\t|\n");
    output_size += strlen("-----------------------------------------------------------\n");
    output_size += strlen("-----------------------------------------------------------\n");

    // Allocate memory for output string
    char *output = malloc(output_size + 1);

    if (output == NULL) {
        printf("Error: Failed to allocate memory\n");
        unlock_shared_memory(shm);
        return NULL;
    }

    // Generate output
    int pos = sprintf(output, "ID\t| KEY\t| MINVALUE\t| MAXVALUE\t|\n");
    pos += sprintf(output + pos, "-----------------------------------------------------------\n");

    for(int i = 0; i < shm->contAlert; i++) {
        pos += sprintf(output + pos, "%s\t| %s\t| %d\t\t| %d\t\t|\n",
                       shm->alertList[i].id, shm->alertList[i].key,
                       shm->alertList[i].minValue, shm->alertList[i].maxValue);
    }

    pos += sprintf(output + pos, "-----------------------------------------------------------\n");
    output[pos] = '\0';

    unlock_shared_memory(shm);

    return output;
}



void worker(void* arg)
{
    int worker_id = *((int*) arg);
    free(arg); 
    writelog("WORKER UP!");
    printf("WORKER UP %d\n",worker_id);
    sem_t* alert_sem = sem_open("/alert_sem", O_CREAT, 0666, 0);




    char message[100];
    //Related to messague queue
    
    char *output = NULL;
    char *pos = NULL;
    struct queuemsg my_msg;
    while(1) {

        read(worker_pipes[worker_id][0], &message, sizeof(message));
        int consoleid;
    

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
        
        printf("Worker %d received message: %s\n", worker_id, message);



        if (message[strlen(message) - 1] == '\n') {
            message[strlen(message) - 1] = '\0';
        }
        char msg_copy[100];
        strcpy(msg_copy, message);
         // extract the first number from the string
        char *word = strtok(message, " ");
        sscanf(word, "%d", &consoleid);

        // extract the first argument from the string
        word = strtok(NULL, " ");




        if (strcmp(word, "stats") == 0) {
            printf("Stats command detected\n");
            output = generateKeystatsListOutput(shm_ptr);
            if (output != NULL) {
                pos = output;
                while (*pos != '\0') {
                    my_msg.mtype = consoleid;

                    size_t len = strnlen(pos, MAX_MSG_SIZE - 1);
                    strncpy(my_msg.mtext, pos, len);
                    my_msg.mtext[len] = '\0';

                    mq_send(mq, (char *) &my_msg, MAX_MSG_SIZE, 0);

                    pos += len;
                }
            free(output);

            
            }
        } else if (strcmp(word, "reset") == 0) {
            printf("Reset command detected\n");
            my_msg.mtype = consoleid; 
            lock_shared_memory();
            keyStats *curr = shm_ptr->keystatsList;
            while (curr != NULL) {
                keyStats *temp = curr;
                curr = curr->next;
                free(temp);
            }
            
            shm_ptr->keystatsList = NULL;
            unlock_shared_memory();
            strcpy(my_msg.mtext, "Reset made!");
            mq_send(mq, (char *) &my_msg, MAX_MSG_SIZE, 0);


        } else if (strcmp(word, "sensors") == 0) {
            printf("Sensors command detected\n");
            fflush(stdout);
            output = generateSensorsOutput(shm_ptr);
            if (output != NULL) {
                pos = output;
                while (*pos != '\0') {
                    
                    my_msg.mtype = consoleid;

                    size_t len = strnlen(pos, MAX_MSG_SIZE - 1);
                    strncpy(my_msg.mtext, pos, len);
                    my_msg.mtext[len] = '\0';

                    mq_send(mq, (char *) &my_msg, MAX_MSG_SIZE, 0);

                    pos += len;
                }
                free(output);

     
            }
        } else if (strcmp(word, "add_alert") == 0) {
            printf("Add alert command detected\n");

            char command[20], id[20], key[20];
            int min, max;
            my_msg.mtype = consoleid; 


            
            //Checka se tem os params corretos

            if (sscanf(msg_copy, "%d %s %s %s %d %d", &consoleid, command, id, key, &min, &max) == 6) {
                addAlertToList(id,key,min,max,shm_ptr,consoleid);
                strncpy(my_msg.mtext, "New alert added\n",MAX_MSG_SIZE - 1);
            } else {
                strncpy(my_msg.mtext, "Invalid input format for add_alert",MAX_MSG_SIZE - 1);
            }

            my_msg.mtext[MAX_MSG_SIZE - 1] = '\0'; // make sure to include null terminator
            mq_send(mq, (char *) &my_msg, MAX_MSG_SIZE, 0);

        } else if (strcmp(word, "remove_alert") == 0) {
            char command[20], id[20];
            my_msg.mtype = consoleid; 

            if (sscanf(msg_copy, "%d %s %s", &consoleid, command, id) == 3) {
                deleteAlertFromList(id,shm_ptr);
                strncpy(my_msg.mtext, "Alert Deleted", MAX_MSG_SIZE - 1);

            } else {
                strncpy(my_msg.mtext, "Invalid input format for remove alert", MAX_MSG_SIZE - 1);

            }
            my_msg.mtext[MAX_MSG_SIZE - 1] = '\0'; // make sure to include null terminator
            mq_send(mq, (char *) &my_msg, MAX_MSG_SIZE, 0);

            printf("Remove alert command detected\n");
        } else if (strcmp(word, "list_alerts") == 0) {
            printf("List alerts command detected\n");
            output = generateAlertOutput(shm_ptr);

            if (output != NULL) {
                pos = output;
                while (*pos != '\0') {
                    my_msg.mtype = consoleid;

                    size_t len = strnlen(pos, MAX_MSG_SIZE - 1);
                    strncpy(my_msg.mtext, pos, len);
                    my_msg.mtext[len] = '\0';

                    mq_send(mq, (char *) &my_msg, MAX_MSG_SIZE, 0);

                    pos += len;
                }
            free(output);

            
            }

        } else {
            char *sensor_id, *key, *value;

            int count = 0;

            for (int i = 0; i < strlen(msg_copy); i++) {
                if (msg_copy[i] == '#') {
                    count++;
                }
            }
            if (count == 2) {

                sensor_id = strtok(msg_copy + 2, "#"); // Skip "S " at the beginning
                key = strtok(NULL, "#");
                value = strtok(NULL, "#");

                addSensor(shm_ptr,sensor_id);
                addKeystats(shm_ptr,key,atoi(value));
                sem_post(alert_sem);  //Warning to Alert Watcher
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
        
        printf("Worker %d FREE!\n", worker_id);



        


    }


}


void checkalert(SharedMemory *shm){
    lock_shared_memory();

    alertStruct* alert = shm->alertList;
    keyStats* a = shm->keystatsList;

    if(alert == NULL){
        printf("NOT MATCHING!\n");
        fflush(stdout);
    }
    if(a== NULL){
        printf("KEYSTATS MORREU!\n");
        fflush(stdout);
    }

    
   
    unlock_shared_memory();

}
alertStruct *findAlertByKey(SharedMemory *shm, char *key) {
    for (int i = 0; i < shm->contAlert; i++) {
        if (strcmp(shm->alertList[i].key, key) == 0) {
            return &shm->alertList[i];
        }
    }
    return NULL;
}

void checkAlerts(SharedMemory *shm) {
    lock_shared_memory(shm);
    struct queuemsg my_msg;

    for (int i = 0; i < shm->contKey; i++) {
        keyStats *keystat = &shm->keystatsList[i];
        if (keystat->verified == false) {
            alertStruct *alert = findAlertByKey(shm, keystat->key);
           
            if (alert != NULL) {
                int lastValue = keystat->last;
                printf("LAST:%d|minV:%d|maxV:%d!\n",lastValue,alert->minValue,alert->maxValue);
                if (lastValue < alert->minValue || lastValue > alert->maxValue) {
                    char message[MAX_MSG_SIZE];
                    snprintf(message, sizeof(message), "ALERT: Key: %s has gone out of range. Last value: %d\n", keystat->key, lastValue);
                    strncpy(my_msg.mtext, message, MAX_MSG_SIZE-1);
                    my_msg.mtext[MAX_MSG_SIZE - 1] = '\0'; // make sure to include null terminator
                    my_msg.mtype= alert->myuser;
                    mq_send(mq, (char *) &my_msg, MAX_MSG_SIZE, 0);
                }
                keystat->verified = true;
            }
        }
    }

    unlock_shared_memory(shm);
}

void alert()
{
    writelog("ALERT WATCHER UP!");
    sem_t* alert_sem = sem_open("/alert_sem", O_CREAT, 0666, 0);
    

    while (1)
    {
        sem_wait(alert_sem);
        printf("Change detected!\n");
        fflush(stdout);
        //lock_shared_memory();
        //printf("%s\n",generateKeystatsListOutput(shm_ptr));
        //unlock_shared_memory();
        // Reset the semaphore to 0
        checkAlerts(shm_ptr);
        sem_trywait(alert_sem);
        
    }
    
    
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
    printf("Number of workers %d\n",nworkers);

    shm_ptr = create_shared_memory();
    //init_shm();
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
    //destroy_shared_memory(shm_ptr);



	return 0;
}