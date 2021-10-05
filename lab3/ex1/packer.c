#include "packer.h"

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

// You can declare global variables here
int count = 0;
sem_t *mutex;
sem_t *waiting;

void packer_init(void)
{
    // Write initialization code here (called once at the start of the program).
    mutex = (sem_t *)malloc(sizeof(sem_t));
    waiting = (sem_t *)malloc(sizeof(sem_t));

    sem_init(mutex, 0, 1);
    sem_init(waiting, 0, 0);
}

void packer_destroy(void)
{
    // Write deinitialization code here (called once at the end of the program).
    sem_destroy(mutex);
    sem_destroy(waiting);
    free(mutex);
    free(waiting);
}

int pack_ball(int colour, int id)
{
    // Write your code here.
    sem_wait(mutex);
    printf("Ball colour %d with id %d arrived\n", colour, id);
    count++;
    sem_post(mutex);
    if (count == 2)
    {
        sem_post(waiting);
        printf("All balls arrived\n");
    }

    sem_wait(waiting);
    sem_post(waiting);

    return id; // need to fix this
}