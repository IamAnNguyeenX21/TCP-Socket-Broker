#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include "routing_table.h"

routing_information* routing_table[100];

volatile bool server_running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n[System] SIGINT received. Shutting down server...\n");
        server_running = 0;
    }
}

#define PORT 8080

int main () 
{
    printf("Server starting...\n");
    
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    sigaction(SIGINT, &sa, NULL);
    
    for(int i = 0; i < 100; i++) 
    {
        routing_table[i] = NULL;
    }

    int server_fd, new_socket;
    struct sockaddr_in server_address;
    int opt = 1;
    socklen_t addrlen = sizeof(server_address);
    char buffer[1024];
    char *reply = "OK!\n";
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 10) < 0) 
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set); 
    int max_fd = server_fd;    

    printf("[System] Server listening on port %d\n", PORT);

    while (server_running)
    {
        read_set = master_set; 

        int activity = select(max_fd + 1, &read_set, NULL, NULL, NULL);

        if (activity < 0) 
        {
            if (errno == EINTR) 
            {
                break; 
            }
            perror("select error");
            break;
        }
        int event_count = 0; 
        for (int i = 0; i <= max_fd; i++) 
        {
            if(event_count >= activity) 
            {
                break;
            }
            if (FD_ISSET(i, &read_set)) 
            {
                event_count++;
                if (i == server_fd) 
                {
                    if ((new_socket = accept(server_fd, (struct sockaddr*)&server_address, &addrlen)) < 0) 
                    {
                        perror("accept");
                        continue;
                    }
                    printf("New connection accepted on FD %d\n", new_socket);

                    FD_SET(new_socket, &master_set); 
                    if (new_socket > max_fd) 
                    {
                        max_fd = new_socket; 
                    }
                } 
                else 
                {
                    memset(buffer, 0, sizeof(buffer));
                    ssize_t received_byte = read(i, buffer, 1023);

                    if (received_byte <= 0 || strncmp(buffer, "exit", 4) == 0) 
                    {
                        if (received_byte == 0 || strncmp(buffer, "exit", 4) == 0) 
                        {
                            printf("Client FD %d disconnected\n", i);
                            if(strncmp(buffer, "exit", 4) == 0) 
                            {
                                send(i, "Goodbye!\n", 9, 0);
                            }
                        } else 
                        {
                            perror("recv failed");
                        }

                        close(i);
                        FD_CLR(i, &master_set); 

                        for(int j = 0; j < 100; j++) 
                        {
                            check_and_delete(&routing_table[j], i);
                        }
                    }
                    else 
                    {
                        buffer[received_byte] = '\0';

                        if (strncmp(buffer, "0x00", 4) == 0) 
                        {
                            int topic_id = atoi(buffer + 4);
                            if (topic_id > 0 && topic_id < 100) 
                            {
                                printf("[SUB] Client FD %d subscribed to Topic %d\n", i, topic_id);
                                check_and_add(&routing_table[topic_id], i);
                            } else 
                            {
                                printf("Invalid topic ID: %d\n", topic_id);
                            }
                        } 
                        else if (strncmp(buffer, "0x01", 4) == 0) 
                        {
                            int topic_id = atoi(buffer + 4);
                            if (topic_id > 0 && topic_id < 100) 
                            {
                                printf("[UNSUB] Client FD %d unsubscribed from Topic %d\n", i, topic_id);
                                check_and_delete(&routing_table[topic_id], i);
                            } else 
                            {
                                printf("Invalid topic ID: %d\n", topic_id);
                            }
                        } 
                        else 
                        {
                            int topic_id = atoi(buffer);
                            if (topic_id > 0 && topic_id < 100) 
                            {
                                printf("[PUB] Client FD %d published to Topic %d\n", i, topic_id);
                                routing_information* temp = routing_table[topic_id];
                                
                                while (temp != NULL) 
                                {
                                    if (temp->client_fd != i) 
                                    {
                                        char msg[1024];
                                        snprintf(msg, sizeof(msg), "Message from topic %d: %s\n", topic_id, buffer + 1); 
                                        send(temp->client_fd, msg, strlen(msg), 0);
                                    }
                                    temp = temp->next;
                                }
                            }
                        }
                        
                        send(i, reply, strlen(reply), 0);
                    }
                }
            }
        }
    } 

    printf("\n[System] Shutting down all connections...\n");
    for (int i = 0; i <= max_fd; i++) 
    {
        if (FD_ISSET(i, &master_set)) 
        {
            close(i);
        }
    }

    for(int i = 0; i < 100; i++) 
    {
        while (routing_table[i] != NULL) 
        {
            check_and_delete(&routing_table[i], routing_table[i]->client_fd);
        }
    }

    printf("[System] Server has shut down safely.\n");
    return 0;
}