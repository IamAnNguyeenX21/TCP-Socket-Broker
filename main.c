#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "routing_table.h"

routing_information* routing_table[100];
topic_entry topic_entry_table[100];

// volatile sig_atomic_t server_running = 1;

// void signal_handler(int signum) {
//     if (signum == SIGINT) {
//         printf("\nServer suspended...\n");
//         server_running = 0;
//     }
// }

#define PORT 8080

void *thread_function(void *arg) {
    printf("Hello from the thread!\n");
    for(int i = 0; i < 100; i++)
    {
        pthread_mutex_lock(&topic_entry_table[i].mutex);
        if(routing_table[i] != NULL)
        {
            printf("Topic %d: ", i);
            print_list(routing_table[i]);
            printf("\n");
        }
        pthread_mutex_unlock(&topic_entry_table[i].mutex);
    }
    int client_fd = *(int *)arg;
    free(arg);
    char buffer[1024];
    ssize_t received_byte;

    char *reply = "OK!\n";
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));

        received_byte = read(client_fd, buffer, 1023);
        
        if(received_byte <= 0)
        {
            perror("recv failed");
            break;
        }
        if(strncmp(buffer, "exit", 4) == 0)
        {
            for(int i = 0; i < 100; i++)
            {
                pthread_mutex_lock(&topic_entry_table[i].mutex);
                if(routing_table[i] != NULL)
                {
                    check_and_delete(&routing_table[i], client_fd);
                }
                pthread_mutex_unlock(&topic_entry_table[i].mutex);
            }
            printf("Client requested to exit. Closing connection.\n");
            break;
        }
        buffer[received_byte] = '\0';
        printf("Received message: %s\n", buffer);
        if(strncmp(buffer, "0x00", 4) == 0)
        {
            int topic_id = atoi(buffer + 4);
            if(topic_id > 0 && topic_id < 100)
            {
                pthread_mutex_lock(&topic_entry_table[topic_id].mutex);
                printf("Client subscribed to topic %d\n", topic_id);
                check_and_add(&routing_table[topic_id], client_fd);
                pthread_mutex_unlock(&topic_entry_table[topic_id].mutex);
            }
            else
            {
                printf("Invalid topic ID: %d\n", topic_id);
            }
        }
        if(strncmp(buffer, "0x01", 4) == 0)
        {
            int topic_id = atoi(buffer + 4);
            if(topic_id > 0 && topic_id < 100)
            {
                pthread_mutex_lock(&topic_entry_table[topic_id].mutex);
                printf("Client unsubscribed from topic %d\n", topic_id);
                check_and_delete(&routing_table[topic_id], client_fd);
                pthread_mutex_unlock(&topic_entry_table[topic_id].mutex);
            }
            else
            {
                printf("Invalid topic ID: %d\n", topic_id);
            }
        }
        int topic_id = atoi(buffer);
        if(topic_id > 0 && topic_id < 100)
        {
            printf("Client published to topic %d\n", topic_id);
            pthread_mutex_lock(&topic_entry_table[topic_id].mutex);
            routing_information* temp = routing_table[topic_id];
            while(temp != NULL)
            {
                if(temp->client_fd != client_fd)
                {
                    char msg[1024];
                    snprintf(msg, sizeof(msg), "Message from topic %d: %s", topic_id, buffer + 1);
                    printf("Forwarding message to client %d: %s\n", temp->client_fd, msg);
                    send(temp->client_fd, msg, strlen(msg), 0);
                }
                temp = temp->next;
            }
            pthread_mutex_unlock(&topic_entry_table[topic_id].mutex);
        }
        send(client_fd, reply, strlen(reply), 0);
    }

    for(int i = 0; i < 100; i++)
    {
        pthread_mutex_lock(&topic_entry_table[i].mutex);
        if(routing_table[i] != NULL)
        {
            check_and_delete(&routing_table[i], client_fd);
        }
        pthread_mutex_unlock(&topic_entry_table[i].mutex);
    }

    close(client_fd);
    return NULL;
}

int main () {

    printf("Hello from the main thread!\n");
    // signal(SIGINT, signal_handler);
    
    for(int i = 0; i < 100; i++)
    {
        routing_table[i] = NULL;
        topic_entry_init(&topic_entry_table[i], &routing_table[i], i);
    }

    int server_fd, new_socket;
    struct sockaddr_in server_address;
    int opt = 1;
    socklen_t addrlen = sizeof(server_address);
    ssize_t received_byte;
    char buffer[1024];
    char *hello = "Hello from the server!\n";
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if((new_socket = accept(server_fd, (struct sockaddr*)&server_address, &addrlen)) < 0)
        {
            // if(!server_running)
            // {
            //     break;
            // }
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pthread_t thread_id;
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        pthread_create(&thread_id, NULL, thread_function, pclient);
        pthread_detach(thread_id);
    }

    // for(int i = 0; i < 100; i++)
    // {
    //     pthread_mutex_destroy(&topic_entry_table[i].mutex);
    // }
    // printf("Server Terminated...\n");
    close(server_fd);
    return 0;
}