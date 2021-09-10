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

// Process statuses
#define STATUS_EXITED 0
#define STATUS_RUNNING 1
#define STATUS_TERMINATING 2

typedef struct PROCESS
{
    pid_t pid;       // process pid
    int status;      // process status
    int exit_status; // process exit status
} process;

process *processes;   // process list
int *no_of_processes; // no. of processes spawned

void my_info();
void my_wait(pid_t pid);
void my_terminate(pid_t pid);

int is_background(size_t num_tokens, char **tokens);
void execute_command(size_t num_tokens, char **tokens);

// Builtin functions

// Prints all processes and its PID + status
void my_info()
{
    int status;
    pid_t wpid;
    for (int i = 0; i < *no_of_processes; i++)
    {
        process *p = &processes[i];
        printf("[%d] ", p->pid);
        switch (p->status)
        {
        case STATUS_RUNNING:
            wpid = waitpid(p->pid, &status, WNOHANG);
            if (wpid == p->pid)
            {
                if (WIFEXITED(status))
                {
                    int es = WEXITSTATUS(status);
                    p->status = STATUS_EXITED;
                    p->exit_status = es;
                    printf("Exited %d\n", p->exit_status);
                }
            }
            else
            {
                printf("Running\n");
            }
            break;
        case STATUS_TERMINATING:
            wpid = waitpid(p->pid, &status, WNOHANG);
            if (wpid == p->pid)
            {
                if (WIFEXITED(status) || WIFSIGNALED(status))
                {
                    int es = WEXITSTATUS(status);
                    p->status = STATUS_EXITED;
                    p->exit_status = es;
                    printf("Exited %d\n", p->exit_status);
                }
            }
            else
            {
                printf("Terminating\n");
            }
            break;
        case STATUS_EXITED:
            printf("Exited %d\n", p->exit_status);
            break;
        }
    }
}

// Waits for child process given PID
void my_wait(pid_t pid)
{
    int status;
    for (int i = 0; i < *no_of_processes; i++)
    {
        process *p = &processes[i];
        if (p->pid == pid && (p->status == STATUS_RUNNING || p->status == STATUS_TERMINATING))
        {
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int es = WEXITSTATUS(status);
                p->status = STATUS_EXITED;
                p->exit_status = es;
            }
            break;
        }
    }
}

// Terminates running child process given PID
void my_terminate(pid_t pid)
{
    if (kill(pid, SIGTERM) == -1)
    {
        perror("sigterm");
    };
    for (int i = 0; i < *no_of_processes; i++)
    {
        process *p = &processes[i];
        if (p->pid == pid)
        {
            p->status = STATUS_TERMINATING;
            break;
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

        // execute the program
        execvp(tokens[0], tokens);
        // this part runs only if error occurs when executing
        perror("child");
        return;
    }
    else
    {
        // parent process

        if (!background)
        {
            // parent wait for child (foreground process)
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int es = WEXITSTATUS(status);
                process child_process = {.pid = pid, .status = STATUS_EXITED, .exit_status = es};
                processes[*no_of_processes] = child_process;
                *no_of_processes += 1;
            }
        }
        else
        {
            // parent does not wait (background process)
            printf("Child[%d] in background\n", pid);
            process child_process = {.pid = pid, .status = STATUS_RUNNING, .exit_status = status}; // exit status will get updated correctly when info is executed
            processes[*no_of_processes] = child_process;
            *no_of_processes += 1;
        }
    }
}

void my_init(void)
{
    // Initialize what you need here

    // shared list of processes so parent & child can access
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

    // check if it is a builtin command
    if (strcmp(tokens[0], "info") == 0)
    {
        my_info();
    }
    else if (strcmp(tokens[0], "wait") == 0)
    {
        my_wait(atoi(tokens[1]));
    }
    else if (strcmp(tokens[0], "terminate") == 0)
    {
        my_terminate(atoi(tokens[1]));
    }
    // check if program exists & can be accessed
    else if (access(tokens[0], F_OK) < 0 || access(tokens[0], X_OK) < 0)
    {
        fprintf(stderr, "%s not found\n", tokens[0]);
    }
    // else, execute the system program
    else
    {
        execute_command(num_tokens, tokens);
    }
}

void my_quit(void)
{
    // Clean up function, called after "quit" is entered as a user command
    for (int i = 0; i < *no_of_processes; i++)
    {
        process *p = &processes[i];
        if (waitpid(p->pid, NULL, WNOHANG) == 0)
        {
            kill(p->pid, SIGTERM);
        }
    }

    munmap(processes, MAX_PROCESSES * sizeof(process));
    munmap(no_of_processes, sizeof(int));

    printf("Goodbye!\n");
}
