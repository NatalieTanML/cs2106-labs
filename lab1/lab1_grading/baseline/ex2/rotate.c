/*
    Used for checking that rotate_list did not deallocate and
    reallocate memory.
*/

// General purpose standard C lib
#include <stdio.h>   // stdio includes printf
#include <stdlib.h>  // stdlib includes malloc() and free()

// User-defined header files
#include "node.h"

// Macros
#define PRINT_LIST 0
#define INSERT_AT 1
#define DELETE_AT 2
#define ROTATE_LIST 3
#define REVERSE_LIST 4
#define RESET_LIST 5

void run_instruction(list *lst, int instr);
void print_list(list *lst);

int main() {
    list *lst = (list *)malloc(sizeof(list));
    lst->head = NULL;

    insert_node_at(lst, 0, 1);
    insert_node_at(lst, 1, 2);
    insert_node_at(lst, 3, 3);

    node *node_list[3] = {
        lst->head,
        lst->head->next,
        lst->head->next->next};

    rotate_list(lst, 1);
    node *curr = lst->head;
    int return_code = 0;

    for (int i = 0; i < 3; i++) {
        if (curr != node_list[(1 + i) % 3]) {
            return_code = 1;
        }
        curr = curr->next;
    }

    reset_list(lst);
    free(lst);

    if (return_code != 0) {
        printf("Addresses of nodes have been modified. Implementation of rotate_list is incorrect.\n");
    }
    return return_code;
}
