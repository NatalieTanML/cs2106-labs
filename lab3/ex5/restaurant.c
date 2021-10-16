#include "restaurant.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

// You can declare global variables here
#define TABLE_FREE 1
#define TABLE_TAKEN 0
#define MAX_TABLES 5000

typedef struct NODE
{
    sem_t block;
    int size;
    struct NODE *next;
    struct NODE *prev;
} node_t;

typedef struct
{
    int size;          // no of seats per table (queue size)
    int num_tables;    // no of tables
    int count_taken;   // no of tables taken
    int count_waiting; // no of groups queuing for this table size
    bool must_wait;    // true if all tables of this size taken
    int *table_ids;    // store the table ids
    int *table_states; // either taken or free
} queue_t;

sem_t mutex;
queue_t queues[5]; // 1 per table size
int swap_q_i;
bool swap_queue = false;

node_t *head = NULL;
node_t *tail = NULL;
int waiting = 0;

bool is_empty()
{
    return head == NULL;
}

node_t *dequeue_size(int size, bool with_less)
{
    if (is_empty())
        return NULL;

    node_t *cur = head;

    if (with_less)
    {
        while (cur->size > size)
        {
            if (cur->next == NULL)
                return NULL;
            else
                cur = cur->next;
        }
    }
    else
    {
        while (cur->size != size)
        {
            if (cur->next == NULL)
                return NULL;
            else
                cur = cur->next;
        }
    }

    if (cur == head)
        head = head->next;
    else if (cur == tail)
        tail = cur->prev;
    else
        cur->prev->next = cur->next;

    return cur;
}

node_t *enqueue(int size)
{
    node_t *link = (node_t *)malloc(sizeof(node_t));
    link->size = size;
    sem_init(&link->block, 0, 0);

    if (is_empty())
    {
        head = link;
        tail = link;
    }
    else
    {
        link->prev = tail;
        tail->next = link;
        tail = link;
    }
    return link;
}

void restaurant_init(int num_tables[5])
{
    // Write initialization code here (called once at the start of the program).
    // It is guaranteed that num_tables is an array of length 5.
    // TODO
    int i, j, table_id = 0;
    sem_init(&mutex, 0, 1);

    for (i = 0; i < 5; i++)
    {
        int n = num_tables[i];
        queue_t *queue = &queues[i];

        queue->size = i + 1;
        queue->num_tables = n;
        queue->count_taken = 0;
        queue->count_waiting = 0;
        queue->must_wait = false;
        queue->table_ids = (int *)malloc(n * sizeof(int));
        queue->table_states = (int *)malloc(n * sizeof(int));

        for (j = 0; j < n; j++)
        {
            queue->table_ids[j] = table_id;
            queue->table_states[j] = TABLE_FREE;
            table_id++;
        }
    }
}

void restaurant_destroy(void)
{
    // Write deinitialization code here (called once at the end of the program).
    // TODO
    sem_destroy(&mutex);
    for (int i = 0; i < 5; i++)
    {
        queue_t *queue = &queues[i];
        free(queue->table_ids);
        free(queue->table_states);
    }
}

int request_for_table(group_state *state, int num_people)
{
    // Write your code here.
    // Return the id of the table you want this group to sit at.
    // TODO
    sem_wait(&mutex);
    int i, t_id;
    int q_i = num_people - 1;
    queue_t *queue = &queues[q_i];
    on_enqueue();

    state->size = num_people;
    state->q_i = q_i;

    if (queue->must_wait)
    {
        // check if other queues available
        for (i = q_i + 1; i < 5; i++)
        {
            if (!queues[i].must_wait && (queues[i].count_waiting == 0))
            {
                queue = &queues[i];
                q_i = i;
                state->q_i = q_i;
                goto enter;
            }
        }
        // block until table is available
        queue->count_waiting++;
        node_t *node = enqueue(num_people);
        // printf("waiting in queue for table size %d, size %d\n", q_i + 1, num_people);
        sem_post(&mutex);
        sem_wait(&node->block);
        // printf("dequeued %d, size %d\n", q_i, node->size);
        queue->count_waiting--;
        if (swap_queue)
        {
            // printf("swap to %d\n", swap_q_i);
            // joining another queue
            queue = &queues[swap_q_i];
            q_i = swap_q_i;
            state->q_i = q_i;
            swap_queue = false;
        }
    }

enter:
    // taking a seat now
    for (i = 0; i < queue->num_tables; i++)
    {
        if (queue->table_states[i] == TABLE_FREE)
        {
            t_id = queue->table_ids[i];
            state->table_id = t_id;
            queue->table_states[i] = TABLE_TAKEN;
            break;
        }
    }

    queue->count_taken++;
    queue->must_wait = (queue->count_taken == queue->num_tables);
    // printf("waiting: %d, tables taken: %d\n", queue->count_waiting, queue->count_taken);

    if (queue->count_waiting && (!queue->must_wait))
    {
        // printf("hi\n");
        node_t *node = dequeue_size(num_people, true);
        sem_post(&node->block);
    }
    else
        sem_post(&mutex);

    // eat
    return t_id;
}

void leave_table(group_state *state)
{
    // Write your code here.
    // TODO
    sem_wait(&mutex);
    int i;
    int q_i = state->q_i;
    int t_id = state->table_id;
    queue_t *queue = &queues[q_i];

    queue->count_taken--;
    // printf("leaving q_i %d, taken: %d, waiting: %d\n", q_i, queue->count_taken, queue->count_waiting);

    if (queue->count_taken < queue->num_tables)
    {
        queue->must_wait = false;
        for (i = 0; i < queue->num_tables; i++)
        {
            if (queue->table_ids[i] == t_id)
            {
                queue->table_states[i] = TABLE_FREE;
                break;
            }
        }
    }

    // if (queue->count_waiting && (!queue->must_wait))
    // {
    //     // printf("people in queue\n");
    //     node_t *node = dequeue_size(queue->size, false);
    //     sem_post(&node->block);
    // }
    if (!queue->must_wait)
    {
        for (i = q_i; i >= 0; i--)
        {
            // printf("people in other queue q_i %d\n", i);
            queue_t *q = &queues[i];
            if (q->count_waiting)
            {
                swap_queue = true;
                swap_q_i = q_i;
                node_t *node = dequeue_size(q->size, true);
                sem_post(&node->block);
                break;
            }
        }
        sem_post(&mutex);
    }
    else
    {
        sem_post(&mutex);
    }
}

/*
1 1 1 1 1
Enter 101 1 Enter 102 2 Enter 103 3 Enter 104 4 Enter 105 5 .


2 2 2 2 2 Enter 201 1 Enter 202 1 Enter 203 1 Enter 204 1 Enter 205 1 Enter 206 1 Enter 207 1 Enter 208 1 Enter 209 1 Enter 210 1 Enter 211 1 Enter 212 1 .

Enter 213 2
Enter 214 3
.


*/