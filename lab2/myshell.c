/**
 * CS2106 AY21/22 Semester 1 - Lab 2
 * 
 * Tan Ming Li, Natalie
 * A0220822U
 * 
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
#include <fcntl.h>

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

int is_not_executable(char *token);
int is_background(size_t num_tokens, char **tokens);
int is_multiple(size_t num_tokens, char **tokens);
void process_commands(size_t num_tokens, char **tokens);
int execute_command(size_t num_tokens, char **tokens);

// ---------------- Builtin functions ----------------

/**
 * Prints all processes and its PID + status
 */
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

/**
 * Waits for child process given PID
 */
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

/**
 * Terminates running child process given PID
 */
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

// ---------------- Misc. helper functions ----------------

/**
 * Checks if program can be accessed or exists
 * @return 1 if cannot be executed, 0 if executable
 */
int is_not_executable(char *token)
{
    return (access(token, F_OK) < 0 || access(token, X_OK) < 0);
}

/**
 * Checks if command is to be run in the background or not
 * @return 1 if is background, 0 if foreground
 */
int is_background(size_t num_tokens, char **tokens)
{
    int background = (strcmp(tokens[num_tokens - 2], "&") == 0);
    if (background)
    {
        tokens[num_tokens - 2] = NULL;
    }
    return background;
}

/**
 * Checks if there are multiple commands in the input
 * @return 1 if multiple commands, 0 if single command
 */
int is_multiple(size_t num_tokens, char **tokens)
{
    int ret = 0;
    for (size_t i = 0; i < num_tokens; i++)
    {
        if (tokens[i] == NULL)
            break;
        if (strcmp(tokens[i], "&&") == 0)
        {
            ret = 1;
            break;
        }
    }
    return ret;
}

/**
 * Processes & runs each command sequentially, breaking when an error occurs
 */
void process_commands(size_t num_tokens, char **tokens)
{
    size_t start = 0, end;
    int exe_ret;

    for (end = 0; end < num_tokens; end++)
    {
        // The first condition is for the last command in the chain which has no trailing &&
        if (end == num_tokens - 1 || strcmp(tokens[end], "&&") == 0)
        {
            tokens[end] = NULL;
            size_t size = (end - start + 1) * sizeof(char *);
            char **sub_tokens = malloc(size);
            if (sub_tokens)
                memcpy(sub_tokens, tokens + start, size);

            if (is_not_executable(sub_tokens[0]))
            {
                fprintf(stderr, "%s not found\n", sub_tokens[0]);
                free(sub_tokens);
                return;
            }

            exe_ret = execute_command(size / sizeof(char *), sub_tokens);
            if (exe_ret == EXIT_FAILURE)
            {
                fprintf(stderr, "%s failed\n", sub_tokens[0]);
                free(sub_tokens);
                return;
            }
            // else if (exe_ret)
            start = end + 1;
            free(sub_tokens);
        }
    }
}

/**
 * Executes the system command
 * @return Exit value of the process
 */
int execute_command(size_t num_tokens, char **tokens)
{
    int status, background, ret = 0;
    background = is_background(num_tokens, tokens);
    pid_t pid = fork();

    if (pid < 0)
    {
        // Error forking
        perror("fork");
        ret = EXIT_FAILURE;
    }
    else if (pid == 0)
    {
        // Child process

        // Check if redirection is needed
        size_t i = 1;
        while (tokens[i] != NULL)
        {
            // Input redirection
            if (tokens[i] != NULL && strcmp(tokens[i], "<") == 0)
            {
                int fdin = open(tokens[i + 1], O_RDONLY);
                if (fdin < 0)
                {
                    fprintf(stderr, "%s does not exist\n", tokens[i + 1]);
                    return EXIT_FAILURE;
                }
                if (dup2(fdin, STDIN_FILENO) < 0)
                {
                    perror("fdin");
                    return EXIT_FAILURE;
                }
                close(fdin);
                for (size_t j = i; tokens[j - 1] != NULL; j++)
                {
                    tokens[j] = tokens[j + 2];
                }
            }

            // Output redirection
            if (tokens[i] != NULL && strcmp(tokens[i], ">") == 0)
            {
                int fdout = open(tokens[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
                if (fdout < 0)
                {
                    perror("fdout");
                    return EXIT_FAILURE;
                }
                if (dup2(fdout, STDOUT_FILENO) < 0)
                {
                    perror("fdout");
                    return EXIT_FAILURE;
                }
                close(fdout);
                for (size_t j = i; tokens[j - 1] != NULL; j++)
                {
                    tokens[j] = tokens[j + 2];
                }
            }

            // Error redirection
            if (tokens[i] != NULL && strcmp(tokens[i], "2>") == 0)
            {
                int fderr = open(tokens[i + 1], O_WRONLY | O_CREAT, 0644);
                if (fderr < 0)
                {
                    perror("fderr");
                    return EXIT_FAILURE;
                }
                if (dup2(fderr, STDERR_FILENO) < 0)
                {
                    perror("fderr");
                    return EXIT_FAILURE;
                }
                dup2(fderr, 2);
                close(fderr);
                for (size_t j = i; tokens[j - 1] != NULL; j++)
                {
                    tokens[j] = tokens[j + 2];
                }
            }

            // Normal token
            i++;
        }

        // Execute the program
        execvp(tokens[0], tokens);
        // This part runs only if error occurs when executing
        perror("child");
        return EXIT_FAILURE;
    }
    else
    {
        // Parent process

        if (!background)
        {
            // Parent wait for child (foreground process)
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int es = WEXITSTATUS(status);
                process child_process = {.pid = pid, .status = STATUS_EXITED, .exit_status = es};
                processes[*no_of_processes] = child_process;
                *no_of_processes += 1;
                return es;
            }
            else
            {
                fprintf(stderr, "child pid %d exited with: %d\n", pid, status);
            }
        }
        else
        {
            // Parent does not wait (background process)
            printf("Child[%d] in background\n", pid);
            waitpid(pid, &status, WNOHANG);
            int es = WEXITSTATUS(status);
            process child_process = {.pid = pid, .status = STATUS_RUNNING, .exit_status = status}; // exit status will get updated correctly when info is executed
            processes[*no_of_processes] = child_process;
            *no_of_processes += 1;
            return es;
        }
    }
    return ret;
}

// ---------------- Functions given ----------------

void my_init(void)
{
    // Initialize what you need here

    // Shared list of processes so parent & child can access
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

    // Check if it is a builtin command
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
    // Check if there are multiple commands, and process one by one
    else if (is_multiple(num_tokens, tokens))
    {
        process_commands(num_tokens, tokens);
    }
    // If the program cannot be accessed or does not exist
    else if (is_not_executable(tokens[0]))
    {
        fprintf(stderr, "%s not found\n", tokens[0]);
    }
    // Else, execute the system command
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
