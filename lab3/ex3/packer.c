#include "packer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

// You can declare global variables here
#define MAX_BALLS 5000

typedef struct BALL
{
    int colour; // colour of the ball
    int id;     // id of this ball
} ball_t;

typedef struct BOX
{
    // sem_t box_sem;           // mutex lock
    // sem_t barrier_1;         // semaphore for barrier 1
    // sem_t barrier_2;         // semaphore for barrier 2
    // ball_t balls[MAX_BALLS]; // all the balls that have been spawned so far
    // int *waiting_ids;        // stores the current N balls that are grouped together
    // int waiting_head;        // stores the current index to add to ^
    // int count_1;             // no. of balls waiting at barrier 1
    // int count_2;             // no. of balls waiting at barrier 2
    // int head;                // current head of the balls[] queue
    // int tail;                // current tail of the balls[] queue

    sem_t box_sem;       // mutex lock
    sem_t block_outside; // to block balls from entering the box
    sem_t block_packing; // to block balls from being sent out before N balls are in
    int count_packed;    // no. of balls packed in the current box (between 0 to N)
    int count_waiting;   // no of balls waiting outside the box
    bool must_wait;      // true if the box has N items
    // ball_t balls[MAX_BALLS]; // all the balls that have been spawned so far
    // int head;                // for the balls array
    // int tail;                // for the balls array
    int *packed_ids; // stores the current balls waiting to be packed together
    int packed_ptr;  // stores the current index to add to for ^
} box_t;

box_t boxes[3]; // 1 for each colour
int n = 2;      // no of balls per pack, N >= 2

void packer_init(int balls_per_pack)
{
    // Write initialization code here (called once at the start of the program).
    // It is guaranteed that balls_per_pack >= 2.
    n = balls_per_pack;
    for (int i = 0; i < 3; ++i)
    {
        // sem_init(&boxes[i].box_sem, 0, 1);
        // sem_init(&boxes[i].barrier_1, 0, 0);
        // sem_init(&boxes[i].barrier_2, 0, 0);
        // boxes[i].waiting_ids = (int *)malloc(n * sizeof(int));
        // boxes[i].waiting_head = 0;
        // boxes[i].count_1 = 0;
        // boxes[i].count_2 = 0;
        // boxes[i].head = 0;
        // boxes[i].tail = -1;

        box_t *box = &boxes[i];

        sem_init(&box->box_sem, 0, 1);
        sem_init(&box->block_outside, 0, 0);
        sem_init(&box->block_packing, 0, 0);
        box->count_packed = 0;
        box->count_waiting = 0;
        box->must_wait = false;
        // box->head = 0;
        // box->tail = 0;
        box->packed_ids = (int *)malloc(n * sizeof(int));
        box->packed_ptr = 0;
    }
}

void packer_destroy(void)
{
    // Write deinitialization code here (called once at the end of the program).
    for (int i = 0; i < 3; ++i)
    {
        // sem_destroy(&boxes[i].box_sem);
        // sem_destroy(&boxes[i].barrier_1);
        // sem_destroy(&boxes[i].barrier_2);
        // free(boxes[i].waiting_ids);

        box_t *box = &boxes[i];
        sem_destroy(&box->box_sem);
        sem_destroy(&box->block_outside);
        sem_destroy(&box->block_packing);
        free(box->packed_ids);
    }
}

void pack_ball(int colour, int id, int *other_ids)
{
    // Write your code here.
    // Remember to populate the array `other_ids` with the (balls_per_pack-1) other balls.
    // int i, j;
    // int c_i = colour - 1;
    // box_t *box = &boxes[c_i];

    // sem_wait(&box->box_sem);
    // // entered barrier 1, add ourselves to our colour's queue
    // box->tail = box->tail + 1;
    // ball_t ball = {.colour = colour, .id = id};
    // box->balls[box->tail] = ball;
    // box->count_1 += 1;

    // if (box->count_1 == n)
    // {
    //     // N balls have reached barrier 1, let them through to barrier 2
    //     for (j = 0; j < n; j++)
    //     {
    //         box->count_1 -= 1;
    //         box->head += 1;
    //         sem_post(&box->barrier_1);
    //     }
    // }
    // sem_post(&box->box_sem);
    // sem_wait(&box->barrier_1);

    // sem_wait(&box->box_sem);
    // // entered barrier 2, add ourselves to the waiting id array
    // box->count_2 += 1;
    // box->waiting_ids[box->waiting_head] = id;
    // box->waiting_head = (box->waiting_head + 1) % n; // essentially we can override the prev IDs after every N balls
    // if (box->count_2 == n)
    // {
    //     // N balls have reached barrier 2, now we can pack and release them
    //     for (int i = 0; i < n; i++)
    //     {
    //         box->count_2 -= 1;
    //         sem_post(&box->barrier_2);
    //     }
    // }
    // sem_post(&box->box_sem);
    // sem_wait(&box->barrier_2);

    // sem_wait(&box->box_sem);
    // i = 0;
    // for (j = 0; j < n; j++)
    // {
    //     // return the other balls' ids excluding ours
    //     if (box->waiting_ids[j] != id)
    //     {
    //         other_ids[i] = box->waiting_ids[j];
    //         i++;
    //     }
    // }
    // sem_post(&box->box_sem);

    int i, j;
    int c_i = colour - 1;
    box_t *box = &boxes[c_i];

    sem_wait(&box->box_sem);
    // printf("id %d entered\n", id);
    if (box->must_wait)
    {
        // printf("id %d waiting\n", id);
        box->count_waiting += 1;
        sem_post(&box->box_sem);
        sem_wait(&box->block_outside);
        box->count_waiting -= 1;
    }

    box->packed_ids[box->packed_ptr] = id;
    box->packed_ptr = (box->packed_ptr + 1) % n;
    box->count_packed += 1;
    box->must_wait = (box->count_packed == n);

    if (box->count_waiting && (!box->must_wait))
        sem_post(&box->block_outside);
    else
        sem_post(&box->box_sem);

    // printf("id %d packing, packed: %d, waiting: %d\n", id, box->count_packed, box->count_waiting);

    sem_wait(&box->box_sem);
    if (box->count_packed == n)
    {
        for (i = 0; i < n; i++)
        {
            sem_post(&box->block_packing);
            box->count_packed -= 1;
        }
    }

    if (box->count_packed == 0)
        box->must_wait = false;

    if (box->count_waiting && (!box->must_wait))
        sem_post(&box->block_outside);
    else
        sem_post(&box->box_sem);

    sem_wait(&box->block_packing);
    sem_wait(&box->box_sem);
    i = 0;
    for (j = 0; j < n; j++)
    {
        // return the other balls' ids excluding ours
        if (box->packed_ids[j] != id)
        {
            other_ids[i] = box->packed_ids[j];
            i++;
        }
    }
    sem_post(&box->box_sem);
}