#!/bin/bash

####################
# Lab 1 Exercise 6
# Name: Tan Ming Li, Natalie
# Student No: A0220822U
# Lab Group: B14
####################

echo "Printing system call report"

# Compile file
gcc -std=c99 pid_checker.c -o ex6

# Use strace to get report
strace -c ./ex6
