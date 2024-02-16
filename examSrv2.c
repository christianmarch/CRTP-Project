#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define MAX_THREADS 10
int exit_status = 0;
pthread_mutex_t mutex;

// Structure to represent a task
struct Task
{
    char name[40];
    int period;             // period time (in seconds)
    int execution;          // worst-case execution time (in seconds)
    int deadline;           // deadline (in seconds)
    int priority;           // fixed-priority of the task (from 1 to 99)
    int active;             // 1 if active, 0 otherwise 
    unsigned int count;     // variable decremented during execution
};

// Global variables
struct Task tasks[] = {
    {"1", 8, 3, 8, 95, 0, 2100000000},
    {"2", 14, 4, 14,50, 0, 2800000000},
    {"3", 22, 5, 22, 25, 0, 3700000000},
    {"4", 6, 4, 6, 99, 0, 2800000000},
    //{"5", 5, 1,, 0, 300000, 0},
    //{"6", 10, 4,, 0, 400000, 0}
};
int num_tasks = sizeof(tasks) / sizeof(struct Task);

    

void *task_routine(void *arg)
{
    struct timespec start_time, end_time, execution_time, remain_time;
    struct Task *task = (struct Task *)arg;

    while (task->active)
    {
        // Simulate task execution
        pthread_mutex_lock(&mutex);
        clock_gettime(CLOCK_REALTIME, &start_time);
        printf("%ld.%ld   Task %s: Executing for %d seconds\n", start_time.tv_sec, start_time.tv_nsec, task->name, task->execution);
        unsigned int i = task->count;
        while(i>0){i--;}
        clock_gettime(CLOCK_REALTIME, &end_time);
        // Calculation of real execution times
        execution_time.tv_sec = end_time.tv_sec - start_time.tv_sec;
        execution_time.tv_nsec = end_time.tv_nsec - start_time.tv_nsec;
        // Adjust representation if tv_nsec is negative
        if (execution_time.tv_nsec < 0) {
            execution_time.tv_sec -= 1;             // Remove a second
            execution_time.tv_nsec += 1000000000;   // Add a second in nanoseconds
        }
        // Calculation of the sleep time (period time - execution time)
        remain_time.tv_sec = task->period - execution_time.tv_sec - 1;  // Remove a second
        remain_time.tv_nsec = 1000000000 - execution_time.tv_nsec;      // Add a second in nanoseconds
        // Sleep until the next period
        printf("%ld.%ld   Task %s: Sleeping for %d s and %d ns (PERIOD: %ds)\n", end_time.tv_sec, end_time.tv_nsec, task->name, 
            remain_time.tv_sec, remain_time.tv_nsec, task->period);
        pthread_mutex_unlock(&mutex);
        nanosleep(&remain_time, NULL);
    }
    return NULL;
}

static int time_analysis(struct Task *task)
{
    int w_curr = 0;    // K-th estimate of Ri 
    int w_prec = 0;    // (K-1)-th estimate of Ri
    int k = 0;

    for (int i = 0; i < num_tasks; ++i)
    {
        if (tasks[i].active || strcmp(tasks[i].name,task->name) == 0)
        {
            // Evaluation of the task i
            printf("\nEvaluation of task %s\n", tasks[i].name);
            k = 0;
            w_curr = tasks[i].execution;
            printf("W_curr(0) = %d\n", w_curr);
            while(w_curr <= tasks[i].deadline)
            {
                // Calculation of the w_curr
                k++;
                w_prec = w_curr;
                w_curr = tasks[i].execution;
                printf("W_curr^(%d) = %d ", k, w_curr);
                for(int j = 0; j < num_tasks; ++j)
                {
                    if((tasks[j].active || strcmp(tasks[j].name,task->name) == 0) && (tasks[j].priority > tasks[i].priority))
                    {
                        printf("+ %d ", ((int)ceil(((double)w_prec)/((double)tasks[j].period)))*tasks[j].execution);
                        w_curr += ((int)ceil(((double)w_prec)/((double)tasks[j].period)))*tasks[j].execution;
                    }
                }
                printf("= %d\n", w_curr);
                if(w_curr == w_prec)
                    break;
            }
            printf("FINAL w_curr=Ri for task %s is %d with deadline %d\n", tasks[i].name, w_curr, tasks[i].deadline);
            if(w_curr > tasks[i].deadline)
                return 0;
        }
    }
    return 1;
}

/* Receive routine: use recv to receive from socket and manage
   the fact that recv may return after having read less bytes than
   the passed buffer size
   In most cases recv will read ALL requested bytes, and the loop body
   will be executed once. This is not however guaranteed and must
   be handled by the user program. The routine returns 0 upon
   successful completion, -1 otherwise */
static int receive(int sd, char *retBuf, int size)
{
    int totSize, currSize;
    totSize = 0;
    while (totSize < size)
    {
        currSize = recv(sd, &retBuf[totSize], size - totSize, 0);
        if (currSize <= 0)
            /* An error occurred */
            return -1;
        totSize += currSize;
    }
    return 0;
}

/* Handle an established  connection
   routine receive is listed in the previous example */
static void handleConnection(int currSd)
{
    unsigned int netLen;
    int len;
    char *command, *answer;
    char response[50];
    for (;;)
    {
        /* Get the command string length
           If receive fails, the client most likely exited */
        if (receive(currSd, (char *)&netLen, sizeof(netLen)))
            break;
        /* Convert from network byte order */
        len = ntohl(netLen);
        command = malloc(len + 1);
        /* Get the command and write terminator */
        receive(currSd, command, len);
        command[len] = 0;
        /* Execute the command and get the answer character string */
        if (strcmp(command, "help") == 0)
            answer = strdup(
                "server is active.\n\n"
                "    commands:\n"
                "       help: print this help\n"
                "       quit: stop client connection\n"
                "       stop: force stop server connection\n");
        else if (strcmp(command, "stop") == 0)
        {
            answer = strdup("closing server connection");
            exit_status = 1;
        }
        else if (strncmp(command, "task", 4) == 0 && len < 7)
        {
            //activate_task(&command[4]);
            // Find the requested task
            struct Task *task = NULL;
            for (int i = 0; i < num_tasks; ++i)
            {
                if (strcmp(tasks[i].name, &command[4]) == 0)
                {
                    task = &tasks[i];
                    break;
                }
            }

            if (task == NULL)
            {
                printf("Task %s not found \n", &command[4]);
                return;
            }

        
            if (task->active == 0)
            {
                if(time_analysis(task)){
                    task->active = 1;
                    // Create a new thread for the task
                    pthread_t thread_id;
                    
                    pthread_attr_t attr;
                    struct sched_param param;
                    pthread_attr_init(&attr);
                    /*
                    if (task->period<10) param.sched_priority = 99;
                    else if (task->period<20) param.sched_priority = 50;
                    else param.sched_priority = 10;
                    */
                    param.sched_priority = task->priority;
                    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
                    pthread_attr_setschedparam(&attr, &param);
                    printf("Priorità della task %s impostata a: %d\n", task->name, param.sched_priority);
                    
                    if (pthread_create(&thread_id, &attr, &task_routine, (void *)task) != 0)
                        perror("Error creating thread");
                    else
                    {
                        //pthread_attr_destroy(&attr);
                        printf("\nTask %s activated\n\n", task->name);
                        sprintf(response, "Task n. %s activated", task->name);
                    }
                    pthread_attr_destroy(&attr);
                }
                else
                {
                    printf("Task %s not schedulable\n", task->name);
                    sprintf(response, "Task n. %s not schedulable", task->name);
                }
            }
            else
            {
                task->active = 0;
                printf("Task %s deactivated\n", task->name);
                sprintf(response, "Task n. %s deactivated", task->name);
            }
            answer = strdup(response);
        }
        else
            answer = strdup("invalid command (try help).");
        /* Send the answer back */
        len = strlen(answer);
        /* Convert to network byte order */
        netLen = htonl(len);
        /* Send answer character length */
        if (send(currSd, &netLen, sizeof(netLen), 0) == -1)
            break;
        /* Send answer characters */
        if (send(currSd, answer, len, 0) == -1)
            break;
        free(command);
        free(answer);
        if (exit_status)
        {
            printf("Force stoping server...\n");
            exit(0);
        }
    }
    /* The loop is most likely exited when the connection is terminated */
    printf("Connection terminated\n");
    close(currSd);
}

/* Thread routine. It calls routine handleConnection()
   defined in the previous program. */
static void *connectionHandler(void *arg)
{
    int currSock = *(int *)arg;
    handleConnection(currSock);
    free(arg);
    pthread_exit(0);
    return NULL;
}

/* Main Program */
int main(int argc, char *argv[])
{
    int SrvSocket, ClnAddrLen, port, len, *ClnSocket;
    unsigned int netLen;
    char *command, *answer;
    struct sockaddr_in SrvAddr, ClnAddr;
    pthread_t threads[MAX_THREADS];
    pthread_mutex_init(&mutex, NULL);

    /* The port number is passed as command argument */
    if (argc < 2)
    {
        printf("Usage: server <port>\n");
        exit(0);
    }
    sscanf(argv[1], "%d", &port);
    /* Create a new socket */
    if ((SrvSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    /* set socket options REUSE ADDRESS */
    int reuse = 1;
    if (setsockopt(SrvSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
#ifdef SO_REUSEPORT
    if (setsockopt(SrvSocket, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEPORT) failed");
#endif
    /* Initialize the address (struct sokaddr_in) fields */
    memset(&SrvAddr, 0, sizeof(SrvAddr));
    SrvAddr.sin_family = AF_INET;
    SrvAddr.sin_addr.s_addr = INADDR_ANY;
    SrvAddr.sin_port = htons(port);

    /* Bind the socket to the specified port number */
    if (bind(SrvSocket, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    /* Set the maximum queue length for clients requesting connection to 5 */
    if (listen(SrvSocket, 5) == -1)
    {
        perror("listen");
        exit(1);
    }
    ClnAddrLen = sizeof(ClnAddr);
    /* Accept and serve all incoming connections in a loop */
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        /* Allocate the current socket.
           It will be freed just before thread termination. */
        ClnSocket = (int *)malloc(sizeof(int));
        if ((*ClnSocket = accept(SrvSocket, (struct sockaddr *)&ClnAddr, &ClnAddrLen)) == -1)
        {
            perror("accept");
            exit(1);
        }
        printf("Connection received from %s\n", inet_ntoa(ClnAddr.sin_addr));
        /* Connection received, start a new thread serving the connection */
        pthread_create(&threads[i], NULL, connectionHandler, ClnSocket);
    }

    return 0; // never reached
}