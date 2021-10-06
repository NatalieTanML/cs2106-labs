#include "packer.h"

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

// You can declare global variables here
#define MAX_BALLS 1000

typedef struct BALL
{
    int colour; // colour of the ball
    int id;     // id of this ball
    int index;  // index of the ball in the array
} ball_t;

sem_t mutex;
sem_t waiting[3];         // 1 sem for each colour
ball_t *balls[3];         // 2d array of balls, 3 = no of colours
int count[3] = {0, 0, 0}; // count of no. of balls

void packer_init(void)
{
    // Write initialization code here (called once at the start of the program).
    sem_init(&mutex, 0, 1);
    for (int i = 0; i < 3; ++i)
    {
        balls[i] = (ball_t *)malloc(MAX_BALLS * sizeof(ball_t));
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
        free(balls[i]);
    }
}

int pack_ball(int colour, int id)
{
    // Write your code here.
    int c_i = colour - 1;
    ball_t ball = {.colour = colour, .id = id};
    int cur = count[c_i];
    ball_t *other = &balls[c_i][cur + 1];

    sem_wait(&mutex);
    ball.index = cur;
    balls[c_i][cur] = ball;
    count[c_i] += 1;

    if (count[c_i] == 2)
    {
        other = &balls[c_i][cur - 1];
        sem_post(&waiting[c_i]);
        count[c_i] -= 2;
    }
    sem_post(&mutex);

    sem_wait(&waiting[c_i]);
    sem_post(&waiting[c_i]);

    return other->id;
}