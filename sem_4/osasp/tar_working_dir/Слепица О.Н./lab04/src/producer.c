#include "header.h"
#include "time.h"

queue *q;
sem_t *mutex;
sem_t *free_space;
sem_t *items;
volatile sig_atomic_t terminate = 0;

void create_message(message *msg) {
    msg->type = rand() % 256;
    
    int rand_size = rand() % 257;
    while (rand_size == 0) {
        rand_size = rand() % 257;
    }
    
    msg->size = (rand_size == 256) ? 0 : rand_size;
    
    for (int i = 0; i < (msg->size == 0 ? 256 : msg->size); i++) {
        msg->data[i] = 32 + rand() % 95; 
    }
    
    msg->hash = 0; 
    msg->hash = calculate_hash(msg);
}

int main() {
    initialize();
    
    srand((unsigned int)time(NULL) ^ getpid());
    
    printf("Producer started (PID: %d)\n", getpid());
    
    while (!terminate) {
        message msg;
        create_message(&msg);
        
        int value;
        if (sem_getvalue(free_space, &value) == 0 && value == 0) {
            fprintf(stderr, "Producer (PID: %d) waiting: queue is full\n", getpid());
        }
        
        sem_wait(free_space);
        
        if (terminate) {
            sem_post(free_space);
            break;
        }
        sem_wait(mutex);
        
        q->buffer[q->tail] = msg;
        q->tail = (q->tail + 1) % BUFFER_SIZE;
        q->added_count++;
        
        int current_count = q->added_count;
        
        sem_post(mutex);
        
        sem_post(items);
        
        printf("PRODUCER %d: TYPE=%d HASH=%04X SIZE=%d ADDED=%d\n", 
        getpid(), msg.type, msg.hash, msg.size == 0 ? 256 : msg.size,
        current_count);
        
        sleep(1 + rand() % 3);
    }
    
    printf("Producer (PID: %d) terminating\n", getpid());
    cleanup();
    
    return 0;
}
