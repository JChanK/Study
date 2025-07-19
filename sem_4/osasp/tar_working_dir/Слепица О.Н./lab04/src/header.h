#ifndef LAB04_HEADER_H
#define LAB04_HEADER_H

#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#define SIZE 256
#define DATA (((SIZE + 3) / 4) * 4)
#define MAX_AMOUNT 111
#define BUFFER_SIZE 8

typedef struct {
    uint8_t type;
    uint16_t hash;
    uint8_t size;
    uint8_t data[DATA];
} message;

typedef struct {
    int added_count; 
    int extracted_count; 
    int head; 
    int tail;  
    message buffer[BUFFER_SIZE];  
} queue;

extern queue *q;
extern sem_t *mutex;
extern sem_t *free_space;
extern sem_t *items;
extern volatile sig_atomic_t terminate;

uint16_t calculate_hash(const message *msg) {
    uint16_t hash = 0;
    const uint8_t *bytes = (const uint8_t*)msg;
    size_t size = sizeof(message);
    
    for (size_t i = 0; i < size; i++) {
        if (i == 1 || i == 2) continue;
        hash = (hash << 5) + hash + bytes[i];
    }
    return hash;
}

// Function to verify hash of message
void verify_hash(message *msg) {
    uint16_t stored_hash = msg->hash;
    msg->hash = 0;
    uint16_t calculated_hash = calculate_hash(msg);
    
    if (calculated_hash != stored_hash) {
        fprintf(stderr, "HASH VERIFICATION FAILED: CALCULATED %04X != STORED %04X\n", 
                calculated_hash, stored_hash);
    }
    
    msg->hash = stored_hash;
}

void *init_shared_memory() {
    int fd = shm_open("message_queue", O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    void *ptr = mmap(NULL, sizeof(queue), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    close(fd);
    return ptr;
}

void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        terminate = 1;
    }
}

void initialize() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    q = (queue*)init_shared_memory();
    
    mutex = sem_open("message_mutex", O_RDWR);
    free_space = sem_open("message_free_space", O_RDWR);
    items = sem_open("message_items", O_RDWR);
    
    if (mutex == SEM_FAILED || free_space == SEM_FAILED || items == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}

void cleanup() {
    if (sem_close(mutex) == -1 || sem_close(free_space) == -1 || sem_close(items) == -1) {
        perror("sem_close");
    }
    
    if (munmap(q, sizeof(queue)) == -1) {
        perror("munmap");
    }
}

#endif // LAB04_HEADER_H
