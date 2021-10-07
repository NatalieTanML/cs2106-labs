#include "packer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

// You can declare global variables here
#define MAX_BALLS 1000

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
int n = 2;      // no of balls per pack

void packer_init(int balls_per_pack)
{
    // Write initialization code here (called once at the start of the program).
    // It is guaranteed that balls_per_pack >= 2.
    n = balls_per_pack;
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

void pack_ball(int colour, int id, int *other_ids)
{
    // Write your code here.
    // Remember to populate the array `other_ids` with the (balls_per_pack-1) other balls.
    int c_i = colour - 1;
    box_t *box = &boxes[c_i];
    ball_t **others = malloc(n * sizeof(ball_t *));
    for (int i = 0; i < n; i++)
    {
        others[i] = malloc(sizeof(ball_t));
    }

    sem_wait(&box->box_sem);
    // Add ball to its colour's queue
    if (box->head == -1)
        box->head = 0;
    box->tail = box->tail + 1;
    ball_t ball = {.colour = colour, .id = id, .index = box->tail};
    box->balls[box->tail] = ball;
    box->count += 1;

    if (box->count >= n)
    {
        int no_of_groups = box->count / n;
        for (int i = 0; i < no_of_groups; i++)
        {

            for (int j = 0; j < n; j++)
            {
                // int temp = box->head;
                if (box->balls[box->head].id != ball.id)
                    others[j] = &box->balls[box->head + j];
                sem_post(&box->waiting[box->head]);
                box->count -= 1;
                box->head += 1;
                // sem_post(&box->waiting[temp]);
            }
        }
    }
    else
    {
        // skip the current ball
        int i = 0;
        int j;
        for (j = box->head; j < ball.index; j++)
        {
            others[i] = &box->balls[j];
            i++;
        }
        for (j = ball.index + 1; j < n; j++)
        {
            others[i] = &box->balls[j];
            i++;
        }
    }
    sem_post(&box->box_sem);
    sem_wait(&box->waiting[box->tail]);

    sem_wait(&box->box_sem);
    for (int i = 0; i < n; i++)
    {
        other_ids[i] = others[i]->id;
    }
    free(others);
    sem_post(&box->box_sem);
}