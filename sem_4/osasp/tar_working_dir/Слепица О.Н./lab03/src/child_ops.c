#define _POSIX_C_SOURCE 199309L
#include "child_ops.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define STATS_UPDATE_INTERVAL_NS 100000000
#define STATS_OUTPUT_FREQUENCY 101

pair_t stats = {0, 0};
size_t c00 = 0, c01 = 0, c10 = 0, c11 = 0;
size_t iteration_count = 0;
char child_name[CHILD_NAME_LENGTH] = {0};
volatile sig_atomic_t alarm_received = false;
volatile sig_atomic_t output_permission_state = WAITING;

void handle_child_signals(int sig)
{
    switch (sig) {
        case SIGALRM:
            alarm_received = true;
            break;
        case SIGUSR1:
            output_permission_state = PRINT_ALLOWED;
            break;
        case SIGUSR2:
            output_permission_state = PRINT_FORBIDDEN;
            break;
    }
}

void update_pair_stats(void)
{
    if (stats.first == 0 && stats.second == 0) c00++;
    else if (stats.first == 0 && stats.second == 1) c01++;
    else if (stats.first == 1 && stats.second == 0) c10++;
    else if (stats.first == 1 && stats.second == 1) c11++;
}

void cycle_pair_values(void)
{
    static int cycle_pos = 0;

    switch (cycle_pos) {
        case 0: stats.first = 0; stats.second = 0; break;
        case 1: stats.first = 0; stats.second = 1; break;
        case 2: stats.first = 1; stats.second = 0; break;
        case 3: stats.first = 1; stats.second = 1; break;
    }

    cycle_pos = (cycle_pos + 1) % 4;
}

void print_safe(const char* str) {
    for (; *str; str++) {
        if (putchar(*str) == EOF) {
            if (errno == EPIPE) {
                _exit(EXIT_FAILURE); 
            }
            perror("putchar failed");
            break;
        }
    }
    if (fflush(stdout) == EOF) {
        perror("fflush failed");
    }
}

void output_stats_report(void)
{
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
                       "[%s pid: %d ppid: %d] stats: 00=%zu 01=%zu 10=%zu 11=%zu\n",
                       child_name, getpid(), getppid(), c00, c01, c10, c11);

    if (len > 0 && len < (int)sizeof(buffer)) {
        print_safe(buffer);
    }

    c00 = c01 = c10 = c11 = 0;

    if (kill(getppid(), SIGUSR2) == -1) {
        perror("failed to send SIGUSR2 to parent");
    }
}

void ask_parent_for_output(void)
{
    if (kill(getppid(), SIGUSR1) == -1) {
        perror("failed to request output permission");
        return;
    }
    output_permission_state = WAITING;
}

void init_child(void)
{
    if (snprintf(child_name, CHILD_NAME_LENGTH, "child_%d", getpid()) < 0) {
        perror("failed to create child name");
        _exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = handle_child_signals;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1 ||
        sigaction(SIGUSR1, &sa, NULL) == -1 ||
        sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("failed to set signal handlers");
        _exit(EXIT_FAILURE);
    }
}

void run_child_process(void) {
    const struct timespec interval = {0, STATS_UPDATE_INTERVAL_NS};

    while (1) {
        if (getppid() == 1) {
            print_safe("Parent process died, exiting...\n");
            _exit(EXIT_FAILURE);
        }

        if (nanosleep(&interval, NULL) == -1) {
            if (errno != EINTR) {
                perror("nanosleep failed");
                break;
            }
            continue;
        }

        cycle_pair_values();
        update_pair_stats();
        iteration_count++;

        if (iteration_count % STATS_OUTPUT_FREQUENCY == 0) {
            ask_parent_for_output();

            while (output_permission_state == WAITING) {
                pause();
            }

            if (output_permission_state == PRINT_ALLOWED) {
                output_stats_report();
            }

            output_permission_state = WAITING;
        }
    }
}
