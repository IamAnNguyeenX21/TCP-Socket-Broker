#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
// #include <sys/un.h>
#include <netinet/in.h>
#include "routing_table.h"

routing_information* routing_table[100];

#define PORT 8080

void *thread_function(void *arg) {
    printf("Hello from the thread!\n");
    print_list(routing_table[0]);
    int client_fd = *(int *)arg;
    free(arg);
    char buffer[1024];
    ssize_t received_byte;

    char *reply = "OK!\n";
    while(1)
    {
        received_byte = read(client_fd, buffer, 1023);
        
        
        if(received_byte < 0)
        {
            perror("recv failed");
            break;
        }
        if(strncmp(buffer, "exit", 4) == 0)
        {
            printf("Client requested to exit. Closing connection.\n");
            break;
        }
        buffer[received_byte] = '\0';
        printf("Received message: %s\n", buffer);
        if(strncmp(buffer, "0x00", 4) == 0)
        {
            check_and_add(&routing_table[0], client_fd);

        }
        memset(buffer, 0, sizeof(buffer));
        send(client_fd, reply, strlen(reply), 0);
    }
    close(client_fd);
    return NULL;
}

int main () {

    printf("Hello from the main thread!\n");
    printf("Hello from the 1st thread!\n");
    
    for(int i = 0; i < 100; i++)
    {
        routing_table[i] = NULL;
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
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pthread_t thread_id;
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        pthread_create(&thread_id, NULL, thread_function, pclient);
        pthread_detach(thread_id);
    }
    close(server_fd);
    return 0;
}