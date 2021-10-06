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

int head[3] = {-1, -1, -1};
int tail[3] = {-1, -1, -1};

sem_t mutex;
sem_t waiting[3];           // 1 sem for each colour
ball_t balls[3][MAX_BALLS]; // 2d array of balls, 3 = no of colours
int count[3] = {0, 0, 0};   // count of no. of balls

void packer_init(void)
{
    // Write initialization code here (called once at the start of the program).
    sem_init(&mutex, 0, 1);
    for (int i = 0; i < 3; ++i)
    {
        sem_init(&waiting[i], 0, 0);
    }
}

void packer_destroy(void)
{
    // Write deinitialization code here (called once at the end of the program).
    sem_destroy(&mutex);
    for (int i = 0; i < 3; ++i)
    {
        sem_destroy(&waiting[i]);
    }
}

int pack_ball(int colour, int id)
{
    // Write your code here.
    int c_i = colour - 1;
    ball_t *other;

    sem_wait(&mutex);
    // Add ball to its colour's queue
    if (head[c_i] == -1)
        head[c_i] = 0;
    tail[c_i] = (tail[c_i] + 1) % MAX_BALLS;
    ball_t ball = {.colour = colour, .id = id, .index = tail[c_i]};
    balls[c_i][tail[c_i]] = ball;

    // Set the other ball its paired with
    other = &balls[c_i][ball.index + 1];
    count[c_i] += 1;
    if (count[c_i] >= 2)
    {
        int no_of_pairs = count[c_i] / N;
        for (int i = 0; i < no_of_pairs; i++)
        {
            other = &balls[c_i][ball.index - 1];
            sem_post(&waiting[c_i]);
            sem_post(&waiting[c_i]);
            count[c_i] -= 2;
            head[c_i] += 2;
        }
        if (head[c_i] == tail[c_i])
        {
            head[c_i] = -1;
            tail[c_i] = -1;
        }
    }
    sem_post(&mutex);
    sem_wait(&waiting[c_i]);

    return other->id;
}