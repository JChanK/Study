#include "header.h"

volatile sig_atomic_t terminate = 0;
pid_t producers[MAX_AMOUNT];
pid_t consumers[MAX_AMOUNT];
int count_producers = 0;
int count_consumers = 0;

static pid_t ppid;
queue *q;
sem_t *mutex;
sem_t *free_space;
sem_t *items;


void display_menu() {
    printf("\n╔════════════════════════════╗");
    printf("\n║        МЕНЮ УПРАВЛЕНИЯ     ║");
    printf("\n╠════════════════════════════╣");
    printf("\n║ 1. Создать производителя   ║");
    printf("\n║ 2. Удалить производителя   ║");
    printf("\n║ 3. Создать потребителя     ║");
    printf("\n║ 4. Удалить потребителя     ║");
    printf("\n║                            ║");
    printf("\n║ m. Показать меню           ║");
    printf("\n║ l. Список процессов        ║");
    printf("\n║ q. Выход                   ║");
    printf("\n╚════════════════════════════╝");
    printf("\nВыберите действие: ");
}

void show_processes() {
    printf("\n┌─────────────────────────────┐");
    printf("\n│      АКТИВНЫЕ ПРОЦЕССЫ      │");
    printf("\n├─────────────────────────────┤");
    printf("\n│ Основной процесс: %-8d  │", ppid);
    printf("\n├─────────────────────────────┤");
    printf("\n│ Производители:              │");
    for (int i = 0; i < count_producers; i++) {
        printf("\n│   %3d. PID: %-8d        │", i+1, producers[i]);
    }
    printf("\n├─────────────────────────────┤");
    printf("\n│ Потребители:                │");
    for (int i = 0; i < count_consumers; i++) {
        printf("\n│   %3d. PID: %-8d        │", i+1, consumers[i]);
    }
    printf("\n└─────────────────────────────┘\n");
}



void create_prod() {
    if (count_producers == MAX_AMOUNT - 1) {
        fprintf(stderr, "YOU REACHED MAX AMOUNT OF PRODUCERS:  %d\n", MAX_AMOUNT);
        return;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        char producer_path[] = "./bin/producer";
        execl(producer_path, producer_path, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }
    
    producers[count_producers++] = pid;
    printf("PRODUCER CREATED. PID: %d\n", pid);
    printf("\n");
}

void del_prod() {
    if (count_producers == 0) {
        fprintf(stderr, "NO PRODUCERS FOUND\n");
        return;
    }
    
    pid_t pid = producers[--count_producers];
    if (kill(pid, SIGUSR1) == -1) {
        perror("kill");
    }
    
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
    }
    
    printf("PRODUCER DELETED. PID: %d\n", pid);
    printf("\n");
}

void create_con() {
    if (count_consumers == MAX_AMOUNT - 1) {
        fprintf(stderr, "YOU REACHED MAX AMOUNT OF CONSUMERS:  %d\n", MAX_AMOUNT);
        return;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        char consumer_path[] = "./bin/consumer";
        execl(consumer_path, consumer_path, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }
    
    consumers[count_consumers++] = pid;
    printf("CONSUMER CREATED. PID: %d\n", pid);
    printf("\n");
}

void del_con() {
    if (count_consumers == 0) {
        fprintf(stderr, "NO CONSUMERS FOUND\n");
        return;
    }
    
    pid_t pid = consumers[--count_consumers];
    if (kill(pid, SIGUSR1) == -1) {
        perror("kill");
    }
    
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
    }
    
    printf("CONSUMER DELETED. PID: %d\n", pid);
    printf("\n");
}

void initialize_queue() {
    ppid = getpid();
    
    int fd = shm_open("message_queue", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(fd, sizeof(queue)) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    void* ptr = mmap(NULL, sizeof(queue), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    close(fd);
    
    q = (queue*)ptr;
    memset(q, 0, sizeof(queue));
    
    mutex = sem_open("message_mutex", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, 1);
    free_space = sem_open("message_free_space", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, BUFFER_SIZE);
    items = sem_open("message_items", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, 0);
    
    if (mutex == SEM_FAILED || free_space == SEM_FAILED || items == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}

void cleanup_resources() {
    for (int i = 0; i < count_producers; i++) {
        if (kill(producers[i], SIGUSR1) == -1) {
            perror("kill producer");
        }
        waitpid(producers[i], NULL, 0);
    }
    
    for (int i = 0; i < count_consumers; i++) {
        if (kill(consumers[i], SIGUSR1) == -1) {
            perror("kill consumer");
        }
        waitpid(consumers[i], NULL, 0);
    }
    
    if (sem_close(mutex) == -1 || sem_close(free_space) == -1 || sem_close(items) == -1) {
        perror("sem_close");
    }
    
    if (sem_unlink("message_mutex") == -1 || 
        sem_unlink("message_free_space") == -1 || 
        sem_unlink("message_items") == -1) {
        perror("sem_unlink");
    }
    
    if (munmap(q, sizeof(queue)) == -1) {
        perror("munmap");
    }
    
    if (shm_unlink("message_queue") == -1) {
        perror("shm_unlink");
    }
    
    printf("All resources cleaned up. Exiting.\n");
}


char get_option() {
    char opt;
    while (1) {
        if (scanf(" %c", &opt) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        
        if ((opt >= '1' && opt <= '4') || opt == 'm' || opt == 'l' || opt == 'q') {
            break;
        }
    }
    return opt;
}

int main() {
    initialize_queue();

    display_menu();
    
    char option;
    while (1) {
        option = get_option();
        printf("\n");
        
        switch (option) {
            case 'm':
                display_menu();
                break;
            case 'l':
                show_processes();
                break;
            case '1':
                create_prod();
                break;
            case '2':
                del_prod();
                break;
            case '3':
                create_con();
                break;
            case '4':
                del_con();
                break;
            case 'q':
                cleanup_resources();
                exit(EXIT_SUCCESS);
        }
    }
    
    return 0;
}
