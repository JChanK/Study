#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>
#include <signal.h>

#define MAX_CHILDREN 8
#define CHILD_NAME_LENGTH 16

#define SEPARATE "=============================================\n"

typedef struct process_info_s {
    pid_t pid;             
    bool is_stopped;         
    char name[CHILD_NAME_LENGTH];
} process_info_t;

typedef struct pair_s {
    int first;             
    int second;             
} pair_t;

typedef enum {
    WAITING,                
    PRINT_ALLOWED,            
    PRINT_FORBIDDEN         
} child_state_t;


extern pair_t stats;         
extern size_t c00;           
extern size_t c01;           
extern size_t c10;           
extern size_t c11;           
extern volatile sig_atomic_t received_signal;
extern volatile sig_atomic_t state;         
extern size_t num_child_processes; 
extern size_t max_child_processes;
extern process_info_t *child_processes;
extern char child_name[CHILD_NAME_LENGTH];

#endif
