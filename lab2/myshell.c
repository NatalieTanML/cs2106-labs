/**
 * CS2106 AY21/22 Semester 1 - Lab 2
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */

#include <sys/wait.h>
#include <sys/mman.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "myshell.h"

#define NO_OF_FUNCTIONS 1;

typedef struct PROCESS
{
    pid_t pid;       // process pid
    bool running;    // check if process is running
    int exit_status; // process exit status
} process;

process *processes;   // process list
int *no_of_processes; // no. of processes spawned

void my_info();
// void my_wait(pid_t pid);
// void my_terminate(pid_t pid);
int is_background(size_t num_tokens, char **tokens);
void execute_command(size_t num_tokens, char **tokens);

// Builtin functions

// Prints all processes and its PID + status
void my_info()
{
    for (int i = 0; i < *no_of_processes; i++)
    {
        process *p = &processes[i];
        printf("[%d] ", p->pid);
        if (p->running)
        {
            int status;
            pid_t w = waitpid(p->pid, &status, WNOHANG);
            if (w == p->pid)
            {
                p->running = false;
                p->exit_status = status;
                printf("Exited %d\n", p->exit_status);
            }
            else
            {
                printf("Running\n");
            }
        }
        else
        {
            printf("Exited %d\n", p->exit_status);
        }
    }
}

// Misc. helper functions

// Checks if command is to be run in the background or not
int is_background(size_t num_tokens, char **tokens)
{
    int background = (strcmp(tokens[num_tokens - 2], "&") == 0);
    if (background)
    {
        tokens[num_tokens - 2] = NULL;
    }
    return background;
}

// Execute system program
void execute_command(size_t num_tokens, char **tokens)
{
    int status, background;
    pid_t pid;

    background = is_background(num_tokens, tokens);
    pid = fork();

    if (pid < 0)
    {
        // error forking
        perror("fork");
    }
    else if (pid == 0)
    {
        // child process
        execvp(tokens[0], tokens);

        // this part runs only if program is not found or error occurs
        fprintf(stderr, "%s not found\n", tokens[0]);
        exit(EXIT_FAILURE);
    }
    else
    {
        // parent process
        if (!background)
        {
            // parent wait for child (foreground process)
            waitpid(pid, &status, 0);
            process child_process = {.pid = pid, .running = false, .exit_status = status};
            processes[*no_of_processes] = child_process;
            *no_of_processes += 1;
        }
        else
        {
            // parent does not wait (background process)
            printf("Child[%d] in background\n", pid);
            process child_process = {.pid = pid, .running = true, .exit_status = status};
            processes[*no_of_processes] = child_process;
            *no_of_processes += 1;
        }
        if (WIFEXITED(status))
        {
            int es = WEXITSTATUS(status);
            for (int i = 0; i < *no_of_processes; i++)
            {
                if (processes[i].pid == pid)
                {
                    processes[i].exit_status = es;
                }
            }
        }
    }
}

void my_init(void)
{
    // Initialize what you need here
    processes = mmap(NULL, MAX_PROCESSES * sizeof(process), PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!processes)
    {
        perror("mmap processes");
        exit(EXIT_FAILURE);
    }

    no_of_processes = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!no_of_processes)
    {
        perror("mmap no_of_processes");
        exit(EXIT_FAILURE);
    }
}

void my_process_command(size_t num_tokens, char **tokens)
{
    // Your code here, refer to the lab document for a description of the arguments

    // empty command
    if (tokens[0] == NULL)
        return;

    // check if it is a builtin command
    if (strcmp(tokens[0], "info") == 0)
    {
        my_info();
        return;
    }

    // else, execute the system process
    execute_command(num_tokens, tokens);
}

void my_quit(void)
{
    // Clean up function, called after "quit" is entered as a user command
    munmap(processes, MAX_PROCESSES * sizeof(process));
    munmap(no_of_processes, sizeof(int));

    printf("Goodbye!\n");
    exit(0);
}
