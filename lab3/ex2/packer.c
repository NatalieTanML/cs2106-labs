#include "packer.h"

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

// You can declare global variables here
#define MAX_BALLS 1000
#define N 2

typedef struct BALL
{
    int colour; // colour of the ball
    int id;     // id of this ball
    int index;  // index it was inserted into
} ball_t;

typedef struct BOX
{
    sem_t box_sem;
    sem_t waiting[MAX_BALLS];
    ball_t balls[MAX_BALLS];
    int count;
    int head;
    int tail;
} box_t;

box_t boxes[3]; // 1 for each colour

void packer_init(void)
{
    // Write initialization code here (called once at the start of the program).
    for (int i = 0; i < 3; ++i)
    {
        sem_init(&boxes[i].box_sem, 0, 1);
        for (int j = 0; j < MAX_BALLS; j++)
            sem_init(&boxes[i].waiting[j], 0, 0);
        boxes[i].count = 0;
        boxes[i].head = -1;
        boxes[i].tail = -1;
    }
}

void packer_destroy(void)
{
    // Write deinitialization code here (called once at the end of the program).
    for (int i = 0; i < 3; ++i)
    {
        sem_destroy(&boxes[i].box_sem);
        for (int j = 0; j < MAX_BALLS; j++)
            sem_destroy(&boxes[i].waiting[j]);
    }
}

int pack_ball(int colour, int id)
{
    // Write your code here.
    int c_i = colour - 1;
    ball_t *other;
    box_t *box = &boxes[c_i];

    sem_wait(&box->box_sem);
    // Add ball to its colour's queue
    if (box->head == -1)
        box->head = 0;
    box->tail = box->tail + 1;
    ball_t ball = {.colour = colour, .id = id, .index = box->tail};
    box->balls[box->tail] = ball;

    // Set the other ball its paired with
    other = &box->balls[ball.index + 1];
    box->count += 1;

    if (box->count >= 2)
    {
        int no_of_pairs = box->count / N;
        for (int i = 0; i < no_of_pairs; i++)
        {
            for (int j = 0; j < N; j++)
            {
                other = &box->balls[ball.index - 1];
                // sem_post(&box->waiting);
                sem_post(&box->waiting[box->head]);
                box->count -= 1;
                box->head += 1;
            }
        }
        if (box->head == box->tail)
        {
            box->head = -1;
            box->tail = -1;
            box->count = 0;
        }
    }
    sem_post(&box->box_sem);
    sem_wait(&box->waiting[box->tail]);

    return other->id;
}