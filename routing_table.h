#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct routing_information
{
    /* data */
    int client_fd;
    struct routing_information* next;
} routing_information;

typedef struct topic_entry
{
    int topic_id;
    routing_information** head;
    pthread_mutex_t mutex;
} topic_entry;

void print_list(routing_information* head);
int topic_entry_init(topic_entry* entry, routing_information** head, int topic_id);
void add_last(routing_information** head, int client_fd);
void delete_node(routing_information** head, int client_fd);
void check_and_add(routing_information** head, int client_fd);
void check_and_delete(routing_information** head, int client_fd);

#endif