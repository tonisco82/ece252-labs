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

// int main(int argc, char *argv[])
// {
//     // Declare first node
//     Node_t *start = NULL;

//     // Allocate Memory
//     char *data = malloc((5 * sizeof(char)));
//     char *data1 = malloc((5 * sizeof(char)));
//     char *data2 = malloc((5 * sizeof(char)));
//     char *data3 = malloc((5 * sizeof(char)));

//     strcpy(data, "test");
//     strcpy(data1, "tes1");
//     strcpy(data2, "tes2");
//     strcpy(data3, "tes3");

//     // Push to the linked list
//     push(&start, data);
//     push(&start, data1);
//     push(&start, data2);
//     push(&start, data3);

//     // Print the linked list
//     scanList(start, printString);

//     // Free memory afterwards
//     freeMemory(start);

//     return 0;
// }