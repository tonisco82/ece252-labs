/**
* @brief: Linked List class for c implementing a linked list of char arrays.
*/

typedef struct Node {
    // Character Array
    char *data;

    struct Node *next;
} Node_t;

// Push a new value to the Linked List.
void push(Node_t** head_node, char *data_new){

    // Malloc new Node
    Node_t *new_node = (Node_t*) malloc(sizeof(Node_t));

    // Assign Data to the same pointer
    new_node->data = data_new;

    // Make the next node the previous head
    new_node->next = (*head_node);

    // Make the new pointer to the head the new node
    (*head_node) = new_node;
}

// Scan list, perform the function on each data value
void scanList(Node_t *head_node, void (*fnc_ptr)(char *)) {
    
    // Iterate over list
    while (head_node != NULL) {
        // Perform desired Function
        (*fnc_ptr)(head_node->data);

        // Move to next node
        node = node->next;
    }
}

// Free Memory for linked list
void freeMemory(Node_t *head_node) {

    Node_t *curr_node = head_node;

    // Iterate over list
    while (head_node != NULL) {
        // Move to next node
        head_node = head_node->next;

        // Free the data
        free(curr_node->data);
        free(curr_node);

        // Assign the current node to the next node
        curr_node = head_node;
    }
}

void printString(char *data){
    printf(data);
}

int main()
{
    Node_t *start = NULL;

    char* data = "TESTING";
    char* data1 = "TESTING1";
    char* data2 = "TESTING2";
    char* data3 = "TESTING3";

    push(&start, data);
    push(&start, data1);
    push(&start, data2);
    push(&start, data3);

    scanList(start, printString);

    freeMemory(start);
}