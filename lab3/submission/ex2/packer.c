#include "packer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>

// You can declare global variables here
typedef struct BOX
{
    sem_t box_sem;         // mutex lock
    sem_t block_outside;   // to block balls from entering the box
    sem_t block_packing_1; // first turnstile for barrier
    sem_t block_packing_2; // second turnstile for barrier
    int count_packed;      // no. of balls packed in the current box (between 0 to N)
    int count_waiting;     // no of balls waiting outside the box
    bool must_wait;        // true if the box has N items (i.e. full)
    int *packed_ids;       // stores the current balls waiting to be packed together
    int packed_ptr;        // stores the current index to add to for ^
} box_t;

box_t boxes[3]; // 1 for each colour
int n = 2;      // no of balls per pack, N >= 2

void packer_init(void)
{
    // Write initialization code here (called once at the start of the program).
    for (int i = 0; i < 3; ++i)
    {
        box_t *box = &boxes[i];

        sem_init(&box->box_sem, 0, 1);
        sem_init(&box->block_outside, 0, 0);
        sem_init(&box->block_packing_1, 0, 0);
        sem_init(&box->block_packing_2, 0, 1);
        box->count_packed = 0;
        box->count_waiting = 0;
        box->must_wait = false;
        box->packed_ids = (int *)malloc(n * sizeof(int));
        box->packed_ptr = 0;
    }
}

void packer_destroy(void)
{
    // Write deinitialization code here (called once at the end of the program).
    for (int i = 0; i < 3; ++i)
    {
        box_t *box = &boxes[i];
        sem_destroy(&box->box_sem);
        sem_destroy(&box->block_outside);
        sem_destroy(&box->block_packing_1);
        sem_destroy(&box->block_packing_2);
        free(box->packed_ids);
    }
}

int pack_ball(int colour, int id)
{
    // Write your code here.
    int i, j, other_id;
    int c_i = colour - 1;
    box_t *box = &boxes[c_i];

    sem_wait(&box->box_sem);
    if (box->must_wait)
    {
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

    // barrier
    sem_wait(&box->box_sem);
    if (box->count_packed == n)
    {
        sem_wait(&box->block_packing_2);
        sem_post(&box->block_packing_1);
    }
    sem_post(&box->box_sem);

    sem_wait(&box->block_packing_1);
    sem_post(&box->block_packing_1);

    sem_wait(&box->box_sem);
    i = 0;
    for (j = 0; j < n; j++)
    {
        // return the other balls' ids excluding ours
        if (box->packed_ids[j] != id)
        {
            other_id = box->packed_ids[j];
            i++;
        }
    }
    sem_post(&box->box_sem);

    sem_wait(&box->box_sem);
    box->count_packed -= 1;

    if (box->count_packed == 0)
    {
        sem_wait(&box->block_packing_1);
        sem_post(&box->block_packing_2);
    }
    sem_post(&box->box_sem);

    sem_wait(&box->block_packing_2);
    sem_post(&box->block_packing_2);
    // end barrier

    sem_wait(&box->box_sem);
    if (box->count_packed == 0)
        box->must_wait = false;

    if (box->count_waiting && (!box->must_wait))
        sem_post(&box->block_outside);
    else
        sem_post(&box->box_sem);

    return other_id;
}