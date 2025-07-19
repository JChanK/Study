#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 199309L
#include "parent_ops.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#ifndef SA_RESTART
#define SA_RESTART 0x10000000
#endif

size_t num_child_processes = 0;
size_t max_child_processes = MAX_CHILDREN;
process_info_t* child_processes = NULL;

void handle_alarm_signal(int sig) {
    if (sig == SIGALRM) {
        unblock_all_child_output();
        printf("Timeout expired. Enabled output for all children.\n");
    }
}

void handle_parent_signals(int sig, siginfo_t* info, void* context) {
    (void)context;

    for (size_t i = 0; i < num_child_processes; i++) {
        if (child_processes[i].pid == info->si_pid) {
            if (sig == SIGUSR1) {
                if (child_processes[i].is_stopped) {
                    if (kill(info->si_pid, SIGUSR2) == -1) {
                        perror("Failed to send SIGUSR2");
                    }
                } else {
                    if (kill(info->si_pid, SIGUSR1) == -1) {
                        perror("Failed to send SIGUSR1");
                    }
                }
            }
            else if (sig == SIGUSR2) {
                printf("C_%d finished output\n", info->si_pid);
            }
            return;
        }
    }
    printf("Received signal from unknown child PID: %d\n", info->si_pid);
}

void init_parent() {
    child_processes = (process_info_t*)malloc(max_child_processes * sizeof(process_info_t));
    if (!child_processes) {
        perror("Failed to allocate memory for child processes");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < max_child_processes; i++) {
        child_processes[i].pid = 0;
        child_processes[i].is_stopped = false;
        memset(child_processes[i].name, 0, CHILD_NAME_LENGTH);
    }

    struct sigaction sa;
    sa.sa_sigaction = handle_parent_signals;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1 ||
        sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Failed to set signal handlers");
        exit(EXIT_FAILURE);
    }

    signal(SIGALRM, handle_alarm_signal);
}

void cleanup_parent() {
    alarm(0);
    terminate_all_children();
    free(child_processes);
    child_processes = NULL;
}

void spawn_child_process() {
    if (num_child_processes >= max_child_processes) {
        printf("Maximum number of children (%zu) reached\n", max_child_processes);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        return;
    }

    if (pid == 0) {
        execl("./child", "./child", NULL);
        perror("Failed to exec child");
        _exit(EXIT_FAILURE);
    }
    else {
        child_processes[num_child_processes].pid = pid;
        child_processes[num_child_processes].is_stopped = false;
        if (snprintf(child_processes[num_child_processes].name, CHILD_NAME_LENGTH,
                   "C_%zu", num_child_processes + 1) < 0) {
            perror("Failed to create child name");
            kill(pid, SIGTERM);
            waitpid(pid, NULL, 0);
            return;
        }

        num_child_processes++;
        printf("Created C_%zu (PID: %d)\n", num_child_processes, pid);
    }
}

void terminate_last_child() {
    if (num_child_processes == 0) {
        printf("No children to remove\n");
        return;
    }

    size_t index = num_child_processes - 1;
    pid_t pid = child_processes[index].pid;

    if (kill(pid, SIGTERM) == -1) {
        perror("Failed to send SIGTERM");
        return;
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("Failed to wait for child");
    }

    num_child_processes--;
    printf("Removed C_%zu (PID: %d). %zu children remaining\n",
           index+1, pid, num_child_processes);
}

void terminate_all_children() {
    if (num_child_processes == 0) {
        printf("No children to remove\n");
        return;
    }

    printf("Removing all %zu C_XX...\n", num_child_processes);
    for (size_t i = 0; i < num_child_processes; i++) {
        if (child_processes[i].pid > 0) {
            if (kill(child_processes[i].pid, SIGTERM) == -1) {
                perror("Failed to send SIGTERM");
            }
            if (waitpid(child_processes[i].pid, NULL, 0) == -1) {
                perror("Failed to wait for child");
            }
        }
    }

    num_child_processes = 0;
    printf("All children removed\n");
}

void display_process_list() {
    printf(SEPARATE);
    printf("Parent PID: %d\n", getpid());
    printf("Child processes (%zu):\n", num_child_processes);

    for (size_t i = 0; i < num_child_processes; i++) {
        printf("%zu. %s (PID: %d, %s)\n",
               i + 1,
               child_processes[i].name,
               child_processes[i].pid,
               child_processes[i].is_stopped ? "stopped" : "running");
    }
    printf(SEPARATE);
}

void block_all_child_output() {
    for (size_t i = 0; i < num_child_processes; i++) {
        child_processes[i].is_stopped = true;
    }
    printf("Disabled output for all children\n");
}

void unblock_all_child_output() {
    for (size_t i = 0; i < num_child_processes; i++) {
        child_processes[i].is_stopped = false;
    }
    printf("Enabled output for all children\n");
}

void block_child_output(int child_num) {
    if (child_num > 0 && child_num <= (int)num_child_processes) {
        child_processes[child_num-1].is_stopped = true;
        printf("Disabled output for child %d (PID: %d)\n",
               child_num, child_processes[child_num-1].pid);
    } else {
        printf("Invalid child number: %d\n", child_num);
    }
}

void unblock_child_output(int child_num) {
    if (child_num > 0 && child_num <= (int)num_child_processes) {
        child_processes[child_num-1].is_stopped = false;
        printf("Enabled output for child %d (PID: %d)\n",
               child_num, child_processes[child_num-1].pid);
    } else {
        printf("Invalid child number: %d\n", child_num);
    }
}

void request_child_stats(int child_num) {
    if (child_num > 0 && child_num <= (int)num_child_processes) {
        block_all_child_output();
        unblock_child_output(child_num);

        alarm(5);
        printf("Requested output from child %d. You have 5 seconds...\n", child_num);
    } else {
        printf("Invalid child number: %d\n", child_num);
    }
}

void handle_user_command(const char* input) {
    if (strlen(input) == 0) return;

    switch (input[0]) {
        case '+':
            spawn_child_process();
            break;
        case '-':
            terminate_last_child();
            break;
        case 'l':
            display_process_list();
            break;
        case 'k':
            terminate_all_children();
            break;
        case 's':
            if (strlen(input) > 1 && isdigit(input[1])) {
                block_child_output(atoi(&input[1]));
            } else {
                block_all_child_output();
            }
            break;
        case 'g':
            if (strlen(input) > 1 && isdigit(input[1])) {
                unblock_child_output(atoi(&input[1]));
            } else {
                unblock_all_child_output();
            }
            break;
        case 'p':
            if (strlen(input) > 1 && isdigit(input[1])) {
                request_child_stats(atoi(&input[1]));
            } else {
                printf("Usage: p<num> (e.g. p1)\n");
            }
            break;
        case 'q':
            cleanup_parent();
            exit(EXIT_SUCCESS);
            break;
        case '?':
            printf("  + : Create child\n  - : Remove last child\n");
            printf("  l : List processes\n  k : Kill all children\n");
            printf("  s : Disable all output\n  g : Enable all output\n");
            printf("  s<num> : Disable output for child <num>\n");
            printf("  g<num> : Enable output for child <num>\n");
            printf("  p<num> : Request output from child <num>\n");
            printf("  q : Quit\n");
            break;
        default:
            printf("Unknown command: %s\n", input);
            break;
    }
}

void parent_main_loop() {
    char input[32];

    printf("Parent process started. PID: %d\n", getpid());
    printf("Type 'help' for available commands\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin)) {
            input[strcspn(input, "\n")] = '\0';

            if (strcmp(input, "help") == 0) {
                handle_user_command("?");
            } else {
                handle_user_command(input);
            }
        } else {
            clearerr(stdin);
            printf("\n");
        }
    }
}

void print_status(const char* message) {
    printf("[PARENT %d] %s\n", getpid(), message);
}
