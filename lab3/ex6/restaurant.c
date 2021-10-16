#include "restaurant.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

// You can declare global variables here
#define TABLE_FREE 1
#define TABLE_TAKEN 0
#define TABLE_PARTIAL 2
#define MAX_TABLES 5000

typedef struct
{
    sem_t block[MAX_TABLES]; // to block people from taking tables
    int size;                // no of seats per table (queue size)
    int num_tables;          // no of tables
    int count_taken;         // no of tables fully taken
    int count_waiting;       // no of groups queuing
    bool must_wait;          // true if all tables taken
    int *table_ids;          // store the table ids
    int *table_states;       // either taken or free
    int waiting_head;        // for the waiting queue
    int waiting_tail;        // for the waiting queue
    int *count_seats_left;   // no of avail seats left per table
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
    int i, j, table_id = 0;
    sem_init(&mutex, 0, 1);

    for (i = 0; i < 5; i++)
    {
        int n = num_tables[i];
        queue_t *queue = &queues[i];

        for (j = 0; j < MAX_TABLES; j++)
            sem_init(&queue->block[j], 0, 0);

        queue->size = i + 1;
        queue->num_tables = n;
        queue->count_taken = 0;
        queue->count_waiting = 0;
        queue->must_wait = false;
        queue->table_ids = (int *)malloc(n * sizeof(int));
        queue->table_states = (int *)malloc(n * sizeof(int));
        queue->waiting_head = 0;
        queue->waiting_tail = 0;
        queue->count_seats_left = (int *)malloc(n * sizeof(int));

        for (j = 0; j < n; j++)
        {
            queue->count_seats_left[j] = i + 1;
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
        free(queue->count_seats_left);
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
            // change
            if (!queues[i].must_wait && (queues[i].count_waiting == 0))
            {
                queue = &queues[i];
                q_i = i;
                goto enter;
            }
        }
        // block until table is available
        queue->count_waiting++;
        sem_post(&mutex);
        int k = queue->waiting_tail;
        queue->waiting_tail++;
        sem_wait(&queue->block[k]);
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
            queue->count_seats_left[i] -= num_people;
            if (queue->count_seats_left[i] > 0)
                queue->table_states[i] = TABLE_PARTIAL;
            else
            {
                queue->table_states[i] = TABLE_TAKEN;
                queue->count_taken++;
            }

            printf("table id %d is %s with %d seats left\n", t_id,
                   queue->table_states[i] == TABLE_PARTIAL ? "partial" : "full",
                   queue->count_seats_left[i]);
            goto seated;
        }
    }

    for (i = 0; i < queue->num_tables; i++)
    {
        if (queue->table_states[i] == TABLE_PARTIAL &&
            queue->count_seats_left[i] >= num_people)
        {
            t_id = queue->table_ids[i];
            state->table_id = t_id;
            queue->count_seats_left[i] -= num_people;
            if (queue->count_seats_left[i] > 0)
                queue->table_states[i] = TABLE_PARTIAL;
            else
            {
                queue->table_states[i] = TABLE_TAKEN;
                queue->count_taken++;
            }

            printf("table id %d is %s with %d seats left\n", t_id,
                   queue->table_states[i] == TABLE_PARTIAL ? "partial" : "full",
                   queue->count_seats_left[i]);
            goto seated;
        }
    }

seated:
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
                queue->count_seats_left[i] += state->size;
                if (queue->count_seats_left[i] == queue->size)
                    queue->table_states[i] = TABLE_FREE;
                else
                    queue->table_states[i] = TABLE_PARTIAL;
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
    else if (queue->count_waiting == 0)
    {
        for (i = q_i - 1; i >= 0; i--)
        {
            queue_t *q = &queues[i];
            if (q->count_waiting && q->must_wait)
            {
                swap_queue = true;
                swap_q_i = q_i;
                int k = q->waiting_head;
                q->waiting_head++;
                sem_post(&q->block[k]);
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