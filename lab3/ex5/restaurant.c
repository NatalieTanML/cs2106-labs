#include "restaurant.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

// You can declare global variables here
#define TABLE_FREE 1
#define TABLE_TAKEN 0

typedef struct
{
    sem_t block;       // to block people from taking tables
    int num_tables;    // no of tables
    int count_taken;   // no of tables taken
    int count_waiting; // no of groups queuing
    bool must_wait;    // true if all tables taken
    int *table_ids;    // store the table ids
    int *table_states;
} queue_t;

sem_t mutex;
queue_t queues[5]; // 1 queue per table size
int swap_q_i;
bool swap_queue = false;

void restaurant_init(int num_tables[5])
{
    // Write initialization code here (called once at the start of the program).
    // It is guaranteed that num_tables is an array of length 5.
    // TODO
    int i, table_id = 0;
    sem_init(&mutex, 0, 1);

    for (i = 0; i < 5; i++)
    {
        int n = num_tables[i];
        queue_t *queue = &queues[i];

        sem_init(&queue->block, 0, 0);
        queue->num_tables = n;
        queue->count_taken = 0;
        queue->count_waiting = 0;
        queue->must_wait = false;
        queue->table_ids = (int *)malloc(n * sizeof(int));
        queue->table_states = (int *)malloc(n * sizeof(int));

        for (int j = 0; j < n; j++)
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
        sem_destroy(&queue->block);
        free(queue->table_ids);
        free(queue->table_states);
    }
}

int request_for_table(group_state *state, int num_people)
{
    // Write your code here.
    // Return the id of the table you want this group to sit at.
    // TODO
    int i, t_id;
    int q_i = num_people - 1;
    queue_t *queue = &queues[q_i];

    sem_wait(&mutex);
    on_enqueue();
    state->size = num_people;

    if (queue->must_wait)
    {
        // check if other queues available
        for (i = q_i + 1; i < 5; i++)
        {
            if (!queues[i].must_wait && (queues[i].count_waiting == 0))
            {
                queue = &queues[i];
                q_i = i;
                state->size = i + 1;
                goto enter;
            }
        }
        // block until table is available
        queue->count_waiting++;
        sem_post(&mutex);
        sem_wait(&queue->block);
        queue->count_waiting--;
        if (swap_queue)
        {
            // joining another queue
            queue = &queues[swap_q_i];
            q_i = swap_q_i;
            state->size = swap_q_i + 1;
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

    if (queue->count_waiting && (!queue->must_wait))
        sem_post(&queue->block);
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
                queue->table_states[i] = TABLE_FREE;
                break;
            }
        }
    }
    if (queue->count_waiting && (!queue->must_wait))
    {
        sem_post(&queue->block);
    }
    else if (queue->count_waiting == 0)
    {
        for (i = q_i - 1; i >= 0; i--)
        {
            if (queues[i].count_waiting && queues[i].must_wait)
            {
                swap_queue = true;
                swap_q_i = q_i;
                sem_post(&queues[i].block);
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

*/