#include "routing_table.h"

routing_information* node_init(int client_fd)
{
    routing_information* new_node = (routing_information*)malloc(sizeof(routing_information));
    if (new_node != NULL)
    {
        new_node->client_fd = client_fd; 
        new_node->next = NULL;
        printf("[DB] Node created with client fd: %d\n", new_node->client_fd);
        return new_node;
    }
    return NULL;
}

void add_last(routing_information** head, int client_fd)
{
    routing_information* new_node = node_init(client_fd);
    if (new_node == NULL) return;

    if (*head == NULL)
    {
        *head = new_node;
        return;
    }

    routing_information* temp = *head;
    while (temp->next != NULL)
    {
        temp = temp->next;
    }
    temp->next = new_node;
}

void print_list(routing_information* head)
{
    routing_information* temp = head;
    printf("Routing Table: ");
    while (temp != NULL)
    {
        printf("%d -> ", temp->client_fd);
        temp = temp->next;
    }
    printf("NULL\n");
}

void delete_node(routing_information** head, int client_fd)
{
    if (head == NULL || *head == NULL)
    {
        printf("[DB] Topic is empty, no client to delete!\n");
        return;
    }
    
    routing_information* temp = *head;
    routing_information* prev = NULL;

    if (temp != NULL && temp->client_fd == client_fd)
    {
        *head = temp->next; 
        free(temp);         
        printf("[DB] Deleted client %d (Head node)\n", client_fd);
        return;
    }

    while (temp != NULL && temp->client_fd != client_fd)
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        printf("[DB] Client %d not found in the routing table.\n", client_fd);
        return;
    }

    prev->next = temp->next;
    free(temp);
    printf("[DB] Deleted client %d\n", client_fd);
}