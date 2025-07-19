#include "header.h"

queue *q;
sem_t *mutex;
sem_t *free_space;
sem_t *items;
volatile sig_atomic_t terminate = 0;

void print_message(const message *msg, int extracted_count) {
    printf("CONSUMER %d: TYPE=%d HASH=%04X SIZE=%d EXTRACTED=%d\n", 
           getpid(), msg->type, msg->hash, msg->size == 0 ? 256 : msg->size, 
           extracted_count);
}

void extract_message(message *msg) {
    *msg = q->buffer[q->head];
    
    q->head = (q->head + 1) % BUFFER_SIZE;
    
    q->extracted_count++;
}

int main() {
    initialize();
    
    printf("Consumer started (PID: %d)\n", getpid());
    
    while (!terminate) {
        int value;
        if (sem_getvalue(items, &value) == 0 && value == 0) {
            fprintf(stderr, "Consumer (PID: %d) waiting: queue is empty\n", getpid());
        }
        
        sem_wait(items);
        if (terminate) {
            sem_post(items);
            break;
        }
        
        sem_wait(mutex);
        
        message msg;
        extract_message(&msg);
        int current_count = q->extracted_count;
        
        sem_post(mutex);
        
        sem_post(free_space);
        
        verify_hash(&msg);
        
        print_message(&msg, current_count);
        
        sleep(2 + rand() % 3);
    }
    
    printf("Consumer (PID: %d) terminating\n", getpid());
    cleanup();
    
    return 0;
}
