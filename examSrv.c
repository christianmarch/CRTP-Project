#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_THREADS 10

// Structure to represent a task
struct Task
{
    char name[50];
    int period;    // in seconds
    int execution; // in microseconds
    int deadline;  // in microseconds
};

// Function prototypes
void *task_routine(void *arg);
void handle_request(char *task_name);

// Global variables
struct Task tasks[] = {
    {"Task1", 5, 100000, 300000},
    {"Task2", 10, 150000, 400000},
    // Add more tasks as needed
};
int num_tasks = sizeof(tasks) / sizeof(struct Task);

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create a socket
    server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    /* set socket options REUSE ADDRESS */
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
#ifdef SO_REUSEPORT
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEPORT) failed");
#endif

    // Bind the socket to an address and port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1)
    {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_THREADS; ++i)
    {
        // Accept a connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1)
        {
            perror("Error accepting connection");
            continue;
        }

        printf("Connection received from %s\n", inet_ntoa(client_addr.sin_addr));

        // Handle the request in a new thread
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, (void *)&handle_request, (void *)&client_socket) != 0)
        {
            perror("Error creating thread");
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}

void *task_routine(void *arg)
{
    struct Task *task = (struct Task *)arg;

    while (1)
    {
        // Simulate task execution
        printf("Executing %s\n", task->name);
        usleep(task->execution);

        // Sleep until the next period
        printf("Sleeping for %d seconds\n", task->period);
        sleep(task->period);
    }

    return NULL;
}

void handle_request(char *task_name)
{
    // Find the requested task
    struct Task *task = NULL;
    for (int i = 0; i < num_tasks; ++i)
    {
        if (strcmp(tasks[i].name, task_name) == 0)
        {
            task = &tasks[i];
            break;
        }
    }

    if (task == NULL)
    {
        printf("Task not found: %s\n", task_name);
        return;
    }

    // Perform response time analysis (not implemented in this example)

    // Create a new thread for the task
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, &task_routine, (void *)task) != 0)
    {
        perror("Error creating thread");
    }
    else
    {
        printf("Task %s activated\n", task->name);
    }
}