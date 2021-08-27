/*************************************
* Lab 1 Exercise 2
* Name: Tan Ming Li, Natalie
* Student No: A0220822U
* Lab Group: B14
*************************************/

#include "node.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Add in your implementation below to the respective functions
// Feel free to add any headers you deem fit (although you do not need to)

// check if list is empty
int is_empty(list *lst)
{
  return lst->head == NULL;
}

// gets node at index
node *get_nth_node(list *lst, int index)
{
  int i = 0;
  node *temp = lst->head;
  while (i != index)
  {
    temp = temp->next;
    i++;
  }
  return temp;
}

// gets node at end of list
node *get_last_node(list *lst)
{
  node *temp = lst->head;
  do
  {
    temp = temp->next;
  } while (temp->next != lst->head);
  return temp;
}

// Inserts a new node with data value at index (counting from head
// starting at 0).
// Note: index is guaranteed to be valid.
void insert_node_at(list *lst, int index, int data)
{
  node *new_node = (node *)malloc(sizeof(node));
  new_node->data = data;

  // if list is empty
  if (is_empty(lst))
  {
    lst->head = new_node;
    new_node->next = lst->head;
    return;
  }

  // if insert at front of list
  if (index == 0)
  {
    node *last = get_last_node(lst);
    last->next = new_node;
    new_node->next = lst->head;
    lst->head = new_node;

    return;
  }

  // if inserting elsewhere
  node *curr = get_nth_node(lst, index - 1);
  new_node->next = curr->next;
  curr->next = new_node;
}

// Deletes node at index (counting from head starting from 0).
// Note: index is guarenteed to be valid.
void delete_node_at(list *lst, int index)
{
  // list is empty
  if (is_empty(lst))
  {
    return;
  }

  node *curr = lst->head;

  // if deleting first node
  if (index == 0)
  {
    node *temp = curr->next;
    if (temp != curr)
    {
      node *last = get_last_node(lst);
      lst->head = temp;
      last->next = lst->head;
    }
    else
    {
      lst->head = NULL;
    }
    free(curr);
    return;
  }

  // if deleting other nodes
  curr = get_nth_node(lst, index - 1);
  node *temp = curr->next;
  curr->next = temp->next;
  free(temp);
}

// Rotates list by the given offset.
// Note: offset is guarenteed to be non-negative.
void rotate_list(list *lst, int offset)
{
  if (is_empty(lst))
  {
    return;
  }
  node *curr = get_nth_node(lst, offset);
  lst->head = curr;
}

// Reverses the list, with the original "tail" node
// becoming the new head node.
void reverse_list(list *lst)
{
  if (is_empty(lst))
  {
    return;
  }

  node *prev, *curr, *next;
  prev = NULL;
  curr = lst->head;
  do
  {
    next = curr->next;
    curr->next = prev;
    prev = curr;
    curr = next;
  } while (curr != lst->head);
  // point the last node to the new head
  lst->head->next = prev;
  lst->head = prev;
}

// Resets list to an empty state (no nodes) and frees
// any allocated memory in the process
void reset_list(list *lst)
{
  if (is_empty(lst))
  {
    return;
  }
  node *curr = lst->head;
  node *next;
  do
  {
    next = curr->next;
    free(curr);
    curr = next;
  } while (curr != lst->head);
  lst->head = NULL;
}
