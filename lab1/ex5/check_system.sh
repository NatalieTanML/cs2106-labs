#!/bin/bash

####################
# Lab 1 Exercise 5
# Name: Tan Ming Li, Natalie
# Student No: A0220822U
# Lab Group: B14
####################

# Fill the below up
hostname=$(hostname)
machine_hardware="$(uname) $(uname -p)"
max_user_process_count=$(ulimit -u)
user_process_count=$(ps x | wc -l)
user_with_most_processes=$(ps -eo user|sort|uniq -c|sort -nr | awk 'NR==1{print $2}')
mem_free_percentage=$(free | grep Mem | awk '{print $4/$2 * 100.0}')
swap_free_percentage=$(free | grep Swap | awk '{print $4/$2 * 100.0}')

echo "Hostname: $hostname"
echo "Machine Hardware: $machine_hardware"
echo "Max User Processes: $max_user_process_count"
echo "User Processes: $user_process_count"
echo "User With Most Processes: $user_with_most_processes"
echo "Memory Free (%): $mem_free_percentage"
echo "Swap Free (%): $swap_free_percentage"
