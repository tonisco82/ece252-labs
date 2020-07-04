/**
* @brief: Linked List class for c implementing a linked list of char arrays.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node
{
    // Character Array
    char *data;

    struct Node *next;
} Node_t;

// Push a new value to the Linked List.
void push(Node_t **head_node, char *data_new)
{

    // Malloc new Node
    Node_t *new_node = (Node_t *)malloc(sizeof(Node_t));

    // Assign Data to the same pointer
    new_node->data = data_new;

    // Make the next node the previous head
    new_node->next = (*head_node);

    // Make the new pointer to the head the new node
    (*head_node) = new_node;
}

// Remove the First Node from the list
char *pop(Node_t **head_node)
{
    // Base Case
    if((*head_node) == NULL) return NULL;

    // Temporary Store Head Node
    Node_t *new_node = (*head_node);

    // Move Head node to next
    (*head_node) = (*head_node)->next;

    // Temp Return Value
    char * result = new_node->data;

    // Free Node
    free(new_node);


    // Return Value
    return result;
}

// Scan list, perform the function on each data value
void scanList(Node_t *head_node, void (*fnc_ptr)(char *))
{
    // Iterate over list
    while (head_node != NULL)
    {
        // Perform desired Function
        (*fnc_ptr)(head_node->data);

        // Move to next node
        head_node = head_node->next;
    }
}

// Free Memory for linked list
void freeMemory(Node_t *head_node)
{

    Node_t *curr_node = head_node;

    // Iterate over list
    while (head_node != NULL)
    {
        // Move to next node
        head_node = head_node->next;

        // Free the data
        free(curr_node->data);
        free(curr_node);

        // Assign the current node to the next node
        curr_node = head_node;
    }
}

void printString(char *data)
{
    printf("%s\n", data);
}

// Print a linked list to a file.
// Each entry is a line
int linkedlist_to_file(Node_t *head_node, char *file_path){
    if(file_path == NULL) return -1;

    FILE *fp = fopen (file_path, "wb+");

    if(!fp) return -1;

    char new_line = '\n';

    // Iterate over list
    while (head_node != NULL)
    {
        fwrite(head_node->data, strlen(head_node->data), 1, fp); //Write Data
        fwrite(&new_line, sizeof(char), 1, fp); //Write newline character

        // Move to next node
        head_node = head_node->next;
    }

    fclose(fp);

    return 0;
}