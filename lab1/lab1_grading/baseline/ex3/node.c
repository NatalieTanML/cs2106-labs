/*************************************
* Lab 1 Exercise 3
* Name:
* Student No:
* Lab Group:
*************************************/

#include "node.h"

#include <stdio.h>   // stdio includes printf
#include <stdlib.h>  // stdlib includes malloc() and free()

// Copy in your implementation of the functions from ex3.
// There is one extra function called map which you have to fill up too.
// Feel free to add any new functions as you deem fit.

// Gets node at index by following head.
node *get_node(node *head, int index) {
    int curr = 0;
    node *temp = head;
    while (curr != index) {
        curr++;
        temp = temp->next;
    }
    return temp;
}

// Creates a new node with data.
node *new_node(int data) {
    node *nx = (node *)malloc(sizeof(node));
    nx->data = data;
    nx->next = NULL;
    return nx;
}

// Returns true if list is empty.
int is_empty_list(list *lst) {
    return lst->head == NULL;
}

// Gets tail node, i.e. the node with pointer back to head.
node *get_tail(list *lst) {
    if (is_empty_list(lst)) {
        return NULL;
    }
    node *head = lst->head;
    node *curr = head;
    while (curr->next != head) {
        curr = curr->next;
    }
    return curr;
}

// Frees current node and updates pointers.
void remove_node(node *curr, node *prev) {
    if (prev != NULL) {
        prev->next = curr->next;
    }
    free(curr);
}

// Inserts a new node with data value at index (counting from head
// starting at 0).
// Note: index is guaranteed to be valid.
void insert_node_at(list *lst, int index, int data) {
    node *nx = new_node(data);
    if (is_empty_list(lst)) {
        lst->head = nx;
        nx->next = nx;
        return;
    }

    // Node is new head
    if (index == 0) {
        nx->next = lst->head;
        node *tail = get_tail(lst);
        tail->next = nx;
        lst->head = nx;
        return;
    }

    // Find node before index
    int i = 0;
    node *prev = lst->head;
    while (i != index - 1) {
        prev = prev->next;
        i++;
    }

    // Update pointers accordingly
    nx->next = prev->next;
    prev->next = nx;
}

// Deletes node at index (counting from head starting from 0).
// Note: index is guarenteed to be valid.
void delete_node_at(list *lst, int index) {
    // Find node at index
    int i = 0;
    node *prev;
    node *curr = lst->head;

    // Deleted node is current head
    if (index == 0) {
        prev = get_tail(lst);
        if (prev == curr) {
            // lst contains a single node
            lst->head = NULL;
            prev = NULL;
        } else {
            lst->head = curr->next;
        }
    }

    // Find node before index for non-head cases
    while (i != index) {
        prev = curr;
        curr = curr->next;
        i++;
    }

    remove_node(curr, prev);
}

// Rotates list by the given offset.
// Note: offset is guarenteed to be non-negative.
void rotate_list(list *lst, int offset) {
    if (is_empty_list(lst)) {
        return;
    }
    for (int i = 0; i < offset; i++) {
        lst->head = lst->head->next;
    }
}

// Reverses the list, with the original "tail" node
// becoming the new head node.
void reverse_list(list *lst) {
    if (is_empty_list(lst)) {
        return;
    }

    node *prev = NULL;
    node *curr = lst->head;
    node *next = curr->next;

    do {
        curr->next = prev;
        prev = curr;
        curr = next;
        next = curr->next;
    } while (curr != lst->head);

    // Point the original head node back to the original tail node
    curr->next = prev;

    // Set new head node of the lst to the original tail node
    lst->head = prev;
}

// Resets list to an empty state (no nodes) and frees
// any allocated memory in the process
void reset_list(list *lst) {
    if (is_empty_list(lst)) {
        return;
    }

    node *curr = lst->head;
    do {
        node *temp = curr;
        curr = temp->next;
        free(temp);
    } while (curr != lst->head);
    lst->head = NULL;
}

// Traverses list and applies func on data values of
// all elements in the list.
void map(list *lst, int (*func)(int)) {
    if (is_empty_list(lst)) {
        return;
    }
    node *curr = lst->head;
    do {
        curr->data = func(curr->data);
        curr = curr->next;
    } while (curr != lst->head);
}

// Traverses list and returns the sum of the data values
// of every node in the list.
long sum_list(list *lst) {
    long res = 0;
    if (is_empty_list(lst)) {
        return res;
    }
    node *curr = lst->head;
    do {
        res += curr->data;
        curr = curr->next;
    } while (curr != lst->head);
    return res;
}
