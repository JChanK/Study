#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <stdarg.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define MAX_PATH_SIZE 1024

static int server_socket = -1;
static char root_dir[MAX_PATH_SIZE];
static volatile int running = 1;

typedef struct {
    int socket;
    struct sockaddr_in addr;
    char current_dir[MAX_PATH_SIZE];
} client_info_t;

void log_event(const char *format, ...) {
    struct timeval tv;
    struct tm *tm_info;
    char timestamp[64];

    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    snprintf(timestamp, sizeof(timestamp), "%04d.%02d.%02d-%02d:%02d:%02d.%03d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             (int)(tv.tv_usec / 1000));

    printf("%s ", timestamp);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);
}

int is_subpath(const char *root, const char *path) {
    char real_root[PATH_MAX];
    char real_path[PATH_MAX];

    if (realpath(root, real_root) == NULL) return 0;
    if (realpath(path, real_path) == NULL) return 0;

    size_t root_len = strlen(real_root);
    return strncmp(real_root, real_path, root_len) == 0 &&
           (real_path[root_len] == '\0' || real_path[root_len] == '/');
}

int make_safe_path(const char *base, const char *relative, char *result, size_t result_size) {
    char temp_path[MAX_PATH_SIZE];

    if (relative[0] == '/') {
        snprintf(temp_path, sizeof(temp_path), "%s%s", base, relative);
    } else {
        snprintf(temp_path, sizeof(temp_path), "%s/%s", base, relative);
    }

    char *normalized = realpath(temp_path, NULL);
    if (!normalized) {
        return 0;
    }

    if (!is_subpath(root_dir, normalized)) {
        free(normalized);
        return 0;
    }

    strncpy(result, normalized, result_size - 1);
    result[result_size - 1] = '\0';
    free(normalized);
    return 1;
}

void get_relative_path(const char *full_path, char *relative, size_t size) {
    size_t root_len = strlen(root_dir);
    if (strncmp(full_path, root_dir, root_len) == 0) {
        const char *rel_start = full_path + root_len;
        if (*rel_start == '/') rel_start++;
        if (*rel_start == '\0') {
            strcpy(relative, "");
        } else {
            strncpy(relative, rel_start, size - 1);
            relative[size - 1] = '\0';
        }
    } else {
        strcpy(relative, "");
    }
}

void handle_echo(int client_socket, const char *args) {
    send(client_socket, args, strlen(args), 0);
    send(client_socket, "\n", 1, 0);
}

void handle_info(int client_socket) {
    const char *info = "Вас приветствует учебный сервер 'myserver'\n"
                      "Доступные команды:\n"
                      "  INFO\n"
                      "  ECHO <текст>\n"
                      "  LIST\n"
                      "  CD <путь>\n"
                      "  QUIT\n";
    send(client_socket, info, strlen(info), 0);
}

void handle_list(int client_socket, const char *current_dir) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char full_path[MAX_PATH_SIZE];
    char response[BUFFER_SIZE] = "";

    dir = opendir(current_dir);
    if (!dir) {
        const char *error = "ERROR: Cannot open directory\n";
        send(client_socket, error, strlen(error), 0);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);

        if (lstat(full_path, &statbuf) == 0) {
            char line[2048];

            if (S_ISDIR(statbuf.st_mode)) {
                snprintf(line, sizeof(line), "%s/\n", entry->d_name);
            } else if (S_ISLNK(statbuf.st_mode)) {
                char link_target[MAX_PATH_SIZE];
                ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
                if (len != -1) {
                    link_target[len] = '\0';

                    char target_full_path[MAX_PATH_SIZE * 2];
                    if (strlen(current_dir) + strlen(link_target) + 2 < sizeof(target_full_path)) {
                        snprintf(target_full_path, sizeof(target_full_path), "%s/%s", current_dir, link_target);

                        struct stat target_stat;
                        if (lstat(target_full_path, &target_stat) == 0 && S_ISLNK(target_stat.st_mode)) {
                            if (strlen(entry->d_name) + strlen(link_target) + 10 < sizeof(line)) {
                                snprintf(line, sizeof(line), "%s -->> %s\n", entry->d_name, link_target);
                            } else {
                                snprintf(line, sizeof(line), "%s -->> (path too long)\n", entry->d_name);
                            }
                        } else {
                            if (strlen(entry->d_name) + strlen(link_target) + 9 < sizeof(line)) {
                                snprintf(line, sizeof(line), "%s --> %s\n", entry->d_name, link_target);
                            } else {
                                snprintf(line, sizeof(line), "%s --> (path too long)\n", entry->d_name);
                            }
                        }
                    } else {
                        snprintf(line, sizeof(line), "%s --> (path too long)\n", entry->d_name);
                    }
                } else {
                    snprintf(line, sizeof(line), "%s --> (broken link)\n", entry->d_name);
                }
            } else {
                snprintf(line, sizeof(line), "%s\n", entry->d_name);
            }

            if (strlen(response) + strlen(line) < sizeof(response) - 1) {
                strncat(response, line, sizeof(response) - strlen(response) - 1);
            }
        }
    }

    closedir(dir);
    send(client_socket, response, strlen(response), 0);
}

void handle_cd(client_info_t *client, const char *args) {
    if (!args || strlen(args) == 0) {
        strcpy(client->current_dir, root_dir);
        return;
    }

    char new_path[MAX_PATH_SIZE];
    if (!make_safe_path(client->current_dir, args, new_path, sizeof(new_path))) {
        return;
    }

    struct stat statbuf;
    if (stat(new_path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
        strcpy(client->current_dir, new_path);
    }
}

void get_prompt(const char *current_dir, char *prompt, size_t size) {
    char relative[MAX_PATH_SIZE];
    get_relative_path(current_dir, relative, sizeof(relative));

    if (strlen(relative) == 0) {
        strcpy(prompt, "> ");
    } else {
        snprintf(prompt, size, "%s> ", relative);
    }
}

void *handle_connection(void *arg) {
    client_info_t *client = (client_info_t *)arg;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    log_event("Client connected: %s:%d",
              inet_ntoa(client->addr.sin_addr),
              ntohs(client->addr.sin_port));

    strcpy(client->current_dir, root_dir);

    handle_info(client->socket);

    char prompt[128];
    get_prompt(client->current_dir, prompt, sizeof(prompt));
    send(client->socket, prompt, strlen(prompt), 0);

    while (running) {
        memset(buffer, 0, sizeof(buffer));

        int bytes_received = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            break;
        }

        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        newline = strchr(buffer, '\r');
        if (newline) *newline = '\0';

        char *start = buffer;
        while (*start == ' ' || *start == '\t') start++;
        
        char *end = start + strlen(start) - 1;
        while (end > start && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }

        log_event("Client %s:%d: '%s'",
                  inet_ntoa(client->addr.sin_addr),
                  ntohs(client->addr.sin_port),
                  start);

        char *command = strtok(start, " ");
        char *args = strtok(NULL, "");

        if (!command || strlen(command) == 0) {
            continue;
        }

        if (strcasecmp(command, "QUIT") == 0) {
            const char *bye = "BYE\n";
            send(client->socket, bye, strlen(bye), 0);
            break;
        } else if (strcasecmp(command, "ECHO") == 0) {
            handle_echo(client->socket, args ? args : "");
        } else if (strcasecmp(command, "INFO") == 0) {
            handle_info(client->socket);
        } else if (strcasecmp(command, "LIST") == 0) {
            handle_list(client->socket, client->current_dir);
        } else if (strcasecmp(command, "CD") == 0) {
            handle_cd(client, args);
        } else {
            snprintf(response, sizeof(response), "ERROR: Unknown command '%s'\n", command);
            send(client->socket, response, strlen(response), 0);
        }

        get_prompt(client->current_dir, prompt, sizeof(prompt));
        send(client->socket, prompt, strlen(prompt), 0);
    }

    log_event("Client disconnected: %s:%d",
              inet_ntoa(client->addr.sin_addr),
              ntohs(client->addr.sin_port));

    close(client->socket);
    free(client);
    pthread_exit(NULL);
}

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        log_event("Server shutting down...");
        running = 0;
        if (server_socket != -1) {
            close(server_socket);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <root_dir> <port>\n", argv[0]);
        return 1;
    }

    const char *root_dir_arg = argv[1];
    int port = atoi(argv[2]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        return 1;
    }

    struct stat statbuf;
    if (stat(root_dir_arg, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "Invalid root directory\n");
        return 1;
    }

    if (!realpath(root_dir_arg, root_dir)) {
        perror("realpath");
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_socket);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_socket);
        return 1;
    }

    log_event("Server started on port %d, root directory: %s", port, root_dir);
    printf("Готов.\n");
    fflush(stdout);

    // Главный цикл сервера
    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(server_socket + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        if (activity > 0 && FD_ISSET(server_socket, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket == -1) {
                if (running) {
                    perror("accept");
                }
                continue;
            }

            client_info_t *client = malloc(sizeof(client_info_t));
            if (!client) {
                close(client_socket);
                continue;
            }

            client->socket = client_socket;
            client->addr = client_addr;

            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_connection, client) != 0) {
                perror("pthread_create");
                close(client_socket);
                free(client);
                continue;
            }

            pthread_detach(thread);
        }
    }

    close(server_socket);
    log_event("Server stopped");
    return 0;
}
