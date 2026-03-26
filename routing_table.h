#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H
#include <stdio.h>
#include <stdlib.h>

typedef struct routing_information
{
    /* data */
    int client_fd;
    struct routing_information* next;
} routing_information;

void print_list(routing_information* head);
void add_last(routing_information** head, int client_fd);
void delete_node(routing_information** head, int client_fd);
void check_and_add(routing_information** head, int client_fd);


#endif