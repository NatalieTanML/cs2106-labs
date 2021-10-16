#include "restaurant.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

// You can declare global variables here
#define TABLE_FREE 1
#define TABLE_TAKEN 0
#define MAX_TABLES 5000

typedef struct
{
    sem_t block[MAX_TABLES]; // to block people from taking tables
    int num_tables;          // no of tables
    int count_taken;         // no of tables taken
    int count_waiting;       // no of groups queuing
    bool must_wait;          // true if all tables taken
    int *table_ids;          // store the table ids
    int *table_states;       // either taken or free
    int waiting_head;        // for the waiting queue
    int waiting_tail;        // for the waiting queue
} queue_t;

sem_t mutex;
queue_t queues[5]; // 1 queue per table size

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

        for (j = 0; j < MAX_TABLES; j++)
            sem_init(&queue->block[j], 0, 0);

        queue->num_tables = n;
        queue->count_taken = 0;
        queue->count_waiting = 0;
        queue->must_wait = false;
        queue->table_ids = (int *)malloc(n * sizeof(int));
        queue->table_states = (int *)malloc(n * sizeof(int));
        queue->waiting_head = 0;
        queue->waiting_tail = 0;

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
        for (int j = 0; j < MAX_TABLES; j++)
            sem_destroy(&queue->block[j]);
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

    if (queue->must_wait)
    {
        // block until table is available
        queue->count_waiting++;
        int k = queue->waiting_tail;
        queue->waiting_tail++;
        sem_post(&mutex);
        sem_wait(&queue->block[k]);
        queue->count_waiting--;
    }

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

    if (queue->count_waiting && (!queue->must_wait))
    {
        int k = queue->waiting_head;
        queue->waiting_head++;
        sem_post(&queue->block[k]);
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
    int q_i = state->size - 1;
    queue_t *queue = &queues[q_i];

    sem_wait(&mutex);
    int i, t_id;
    t_id = state->table_id;

    queue->count_taken--;

    if (queue->count_taken != queue->num_tables)
    {
        queue->must_wait = false;
        for (i = 0; i < queue->num_tables; i++)
        {
            if (queue->table_ids[i] == t_id)
            {
                state->table_id = 0;
                queue->table_states[i] = TABLE_FREE;
                break;
            }
        }
    }

    if (queue->count_waiting && (!queue->must_wait))
    {
        int k = queue->waiting_head;
        queue->waiting_head++;
        sem_post(&queue->block[k]);
    }
    else
        sem_post(&mutex);
}