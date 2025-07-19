#include "queue.h"
#include "utils.h"

#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t stop_flag = 0;


#define DEFAULT_queue_SIZE 10
#define MAX_THREADS_COUNT 50

queue_t* queue;
int sync_method = 0; // 0 - semaphores, 1 - condvars


sem_t* producer;
sem_t* consumer;
sem_t* mutex;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

void handle_sigint(int sig);
void graceful_shutdown(
    pthread_t* producer_threads, size_t* producer_threads_count,
    pthread_t* consumer_threads, size_t* consumer_threads_count,
    sem_t* producer, sem_t* consumer, sem_t* mutex);
void initialize_sync_primitives();
void cleanup_sync_primitives();
void display_main_menu();

int main()
{
    signal(SIGINT, handle_sigint);

    printf("Select synchronization method:\n");
    printf("1 - Semaphores and mutex\n");
    printf("2 - Conditional variables\n");
    printf("Your choice: ");
    scanf("%d", &sync_method);
    sync_method--; // 0 or 1

    initialize_sync_primitives();

    pthread_t producer_threads[MAX_THREADS_COUNT];
    pthread_t consumer_threads[MAX_THREADS_COUNT];
    size_t producer_threads_count = 0;
    size_t consumer_threads_count = 0;

    size_t producer_count = 0;
    size_t consumer_count = 0;
    char input[2];

    queue = (queue_t*)calloc(1, sizeof(queue_t));
    if (!queue) {
        perror("Failed to allocate queue");
        exit(EXIT_FAILURE);
    }
    
    // Инициализация кольцевого буфера
    queue->size = DEFAULT_queue_SIZE;
    queue->head = NULL;
    queue->tail = NULL;
    queue->cur = 0;
    queue->added = 0;
    queue->deleted = 0;

    printf("Using %s\n", sync_method == 0 ? "POSIX semaphores" : "conditional variables");
    display_main_menu();
    printf("\nCurrent queue size: %ld\n", queue->size);

    while (!stop_flag)
    {
        scanf("%s", input);

        switch (input[0])
        {
        case '1':
        {
            if (producer_threads_count >= MAX_THREADS_COUNT) {
                printf("Max producer threads reached.\n");
                break;
            }
            void* (*routine)(void*) = sync_method == 0 ? producer_routine : producer_routine_cond;
            pthread_create(&producer_threads[producer_threads_count++], NULL, routine, NULL);
            producer_count++;
            break;
        }  
        case '2':
        {
            if (producer_threads_count > 0)
            {
                pthread_kill(producer_threads[--producer_threads_count], SIGUSR1);
                pthread_join(producer_threads[producer_threads_count], NULL);
                producer_count--;
                printf("Last producer thread removed\n");
            }
            else
            {
                printf("No producer threads to remove\n");
            }
            break;
        }     
        case '3':
        {
            if (consumer_threads_count >= MAX_THREADS_COUNT) {
                printf("Max consumer threads reached.\n");
                break;
            }
            void* (*routine)(void*) = sync_method == 0 ? consumer_routine : consumer_routine_cond;
            pthread_create(&consumer_threads[consumer_threads_count++], NULL, routine, NULL);
            consumer_count++;
            break;
        }
        case '4':
        {
            if (consumer_threads_count > 0)
            {
                pthread_kill(consumer_threads[--consumer_threads_count], SIGUSR1);
                pthread_join(consumer_threads[consumer_threads_count], NULL);
                consumer_count--;
                printf("Last consumer thread removed\n");
            }
            else
            {
                printf("No consumer threads to remove\n");
            }
            break;
        } 
        case 'l':
        {
            if (sync_method == 0) {
                sem_wait(mutex);
            } else {
                pthread_mutex_lock(&queue_mutex);
            }

            printf("ADDED: %ld\nGOT: %ld\nProducers COUNT: %ld\nConsumers COUNT: %ld\nCurrent SIZE: %ld\nMax size: %ld\n", 
                queue->added, 
                queue->deleted, 
                producer_count, 
                consumer_count,
                queue->cur,
                queue->size);

            if (sync_method == 0) {
                sem_post(mutex);
            } else {
                pthread_mutex_unlock(&queue_mutex);
            }
            break;
        }
        case '+':
        {
            if (sync_method == 0) {
                sem_wait(mutex);
                queue->size++;
                sem_post(mutex);
            } else {
                pthread_mutex_lock(&queue_mutex);
                queue->size++;
                pthread_cond_broadcast(&not_full);
                pthread_mutex_unlock(&queue_mutex);
            }
            break;
        }
        
        case '-':
        {
            if (sync_method == 0) {
                sem_wait(mutex);
                if (queue->size > 0) {
                    queue->size--;
                    if (queue->cur > queue->size) {
                        pop(&queue->head, &queue->tail);
                        queue->cur--;
                    }
                } else {
                    printf("\nQueue is empty\n");
                }
                sem_post(mutex);
            } else {
                pthread_mutex_lock(&queue_mutex);
                if (queue->size > 0) {
                    queue->size--;
                    if (queue->cur > queue->size) {
                        pop(&queue->head, &queue->tail);
                        queue->cur--;
                    }
                    pthread_cond_broadcast(&not_full);
                    pthread_cond_broadcast(&not_empty);
                } else {
                    printf("\nQueue is empty\n");
                }
                pthread_mutex_unlock(&queue_mutex);
            }
            break;
        }
        case 'q':
        {
            graceful_shutdown(producer_threads, &producer_threads_count,
                consumer_threads, &consumer_threads_count,
                producer, consumer, mutex);
            return 0;
        }
        case 'm':
            display_main_menu();
            printf("\nQueue size: %ld\n", queue->size);
            break;
        default:
            break;
        }
    }

    if (stop_flag) {
        graceful_shutdown(producer_threads, &producer_threads_count, consumer_threads, &consumer_threads_count, producer, consumer, mutex);
    }
    pthread_cond_broadcast(&not_empty);
    pthread_cond_broadcast(&not_full);

    return 0;
}

void handle_sigint(int sig)
{
    stop_flag = 1;
}

void display_main_menu() {
    printf("\n╔════════════════════════════╗");
    printf("\n║        МЕНЮ УПРАВЛЕНИЯ     ║");
    printf("\n╠════════════════════════════╣");
    printf("\n║ 1 - Добавить производителя ║");
    printf("\n║ 2 - Удалить производителя  ║");
    printf("\n║ 3 - Добавить потребителя   ║");
    printf("\n║ 4 - Удалить потребителя    ║");
    printf("\n║ l - Показать статистику    ║");
    printf("\n║ + - Увеличить очередь      ║");
    printf("\n║ - - Уменьшить очередь      ║");
    printf("\n║ q - Выход                  ║");
    printf("\n║ m - Показать меню          ║");
    printf("\n╚════════════════════════════╝");
    printf("\nВыберите действие: ");
}

void graceful_shutdown(
    pthread_t* producer_threads, size_t* producer_threads_count,
    pthread_t* consumer_threads, size_t* consumer_threads_count,
    sem_t* producer, sem_t* consumer, sem_t* mutex)
{
    
    while (*producer_threads_count > 0) {
        pthread_kill(producer_threads[--(*producer_threads_count)], SIGUSR1);
        pthread_join(producer_threads[*producer_threads_count], NULL);
    }

    while (*consumer_threads_count > 0) {
        pthread_kill(consumer_threads[--(*consumer_threads_count)], SIGUSR1);
        pthread_join(consumer_threads[*consumer_threads_count], NULL);
    }

    cleanup_sync_primitives();

    queue_clear(queue);
    free(queue);

    printf("\nGraceful shutdown complete.\n");
}

void initialize_sync_primitives() {
    if (sync_method == 0) {
        sem_unlink("/producer");
        sem_unlink("/consumer");
        sem_unlink("/mutex");

        producer = sem_open("/producer", O_CREAT, 0644, 1);
        consumer = sem_open("/consumer", O_CREAT, 0644, 1);
        mutex = sem_open("/mutex", O_CREAT, 0644, 1);
    } else {
        pthread_mutex_init(&queue_mutex, NULL);
        pthread_cond_init(&not_empty, NULL);
        pthread_cond_init(&not_full, NULL);
    }
}

void cleanup_sync_primitives() {
    if (sync_method == 0) {
        sem_close(producer);
        sem_close(consumer);
        sem_close(mutex);

        sem_unlink("/producer");
        sem_unlink("/consumer");
        sem_unlink("/mutex");
    } else {
        pthread_mutex_destroy(&queue_mutex);
        pthread_cond_destroy(&not_empty);
        pthread_cond_destroy(&not_full);
    }
}
