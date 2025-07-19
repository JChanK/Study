#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#define BUFFER_SIZE 4096
#define MAX_LINE_SIZE 1024

static volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

int send_command(int socket, const char *command) {
    if (send(socket, command, strlen(command), 0) == -1) {
        if (running) perror("send");
        return -1;
    }

    if (send(socket, "\n", 1, 0) == -1) {
        if (running) perror("send");
        return -1;
    }

    return 0;
}

int receive_response(int socket) {
    char buffer[BUFFER_SIZE];
    char accumulated[BUFFER_SIZE * 4] = "";
    size_t total_received = 0;
    int found_prompt = 0;

    while (!found_prompt && running) {
        ssize_t bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            if (!running) return 0;
            if (bytes_received == 0) {
                printf("Server closed connection\n");
            } else {
                perror("recv");
            }
            return -1;
        }

        buffer[bytes_received] = '\0';

        if (total_received + bytes_received < sizeof(accumulated) - 1) {
            strncat(accumulated, buffer, sizeof(accumulated) - total_received - 1);
            total_received += bytes_received;
        }

        char *prompt_pos = NULL;
        char *search_pos = accumulated;
        
        while ((search_pos = strstr(search_pos, "> ")) != NULL && running) {
            char *after_prompt = search_pos + 2;
            if (*after_prompt == '\0' || 
                (strstr(after_prompt, "> ") == NULL && strchr(after_prompt, '\n') == NULL)) {
                prompt_pos = search_pos;
                break;
            }
            search_pos += 2;
        }

        if (prompt_pos != NULL) {
            *prompt_pos = '\0';
            if (strlen(accumulated) > 0) {
                printf("%s", accumulated);
                fflush(stdout);
            }
            found_prompt = 1;
        }

        if (strstr(accumulated, "BYE") != NULL) {
            printf("%s", accumulated);
            fflush(stdout);
            return 1;
        }

        if (total_received >= sizeof(accumulated) - 100) {
            printf("%s", accumulated);
            fflush(stdout);
            accumulated[0] = '\0';
            total_received = 0;
        }
    }

    return running ? 0 : -2;
}

int receive_initial_response(int socket) {
    char buffer[BUFFER_SIZE];
    char accumulated[BUFFER_SIZE * 2] = "";
    size_t total_received = 0;

    while (running) {
        ssize_t bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            if (!running) return 0;
            if (bytes_received == 0) {
                printf("Server closed connection\n");
            } else {
                perror("recv");
            }
            return -1;
        }

        buffer[bytes_received] = '\0';
        
        if (total_received + bytes_received < sizeof(accumulated) - 1) {
            strncat(accumulated, buffer, sizeof(accumulated) - total_received - 1);
            total_received += bytes_received;
        }

        char *prompt_pos = strstr(accumulated, "> ");
        if (prompt_pos != NULL && 
            (prompt_pos[2] == '\0' || strstr(prompt_pos + 2, "> ") == NULL)) {
            *prompt_pos = '\0';
            printf("%s", accumulated);
            fflush(stdout);
            break;
        }
    }

    return running ? 0 : -2;
}

int process_file_commands(int socket, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    char line[MAX_LINE_SIZE];
    while (fgets(line, sizeof(line), file) && running) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';

        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        printf("%s\n", line);
        fflush(stdout);

        if (send_command(socket, line) == -1) {
            fclose(file);
            return -1;
        }

        int result = receive_response(socket);
        if (result != 0) {
            fclose(file);
            return result;
        }
    }

    fclose(file);
    return running ? 0 : -2;
}

int interactive_mode(int socket) {
    char input[MAX_LINE_SIZE];

    while (running) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            if (feof(stdin)) {
                printf("\n");
                break;
            }
            continue;
        }

        char *newline = strchr(input, '\n');
        if (newline) *newline = '\0';

        if (strlen(input) == 0) {
            continue;
        }

        if (input[0] == '@') {
            const char *filename = input + 1;
            int result = process_file_commands(socket, filename);
            if (result != 0) {
                return result;
            }
            continue;
        }

        if (send_command(socket, input) == -1) {
            return -1;
        }

        int result = receive_response(socket);
        if (result != 0) {
            return result;
        }
    }

    if (running) {
        send_command(socket, "QUIT");
        receive_response(socket);
    }

    return 0;
}

struct in_addr resolve_hostname(const char *hostname) {
    struct in_addr addr;

    if (inet_aton(hostname, &addr)) {
        return addr;
    }

    struct hostent *host_entry = gethostbyname(hostname);
    if (host_entry) {
        addr = *((struct in_addr *)host_entry->h_addr_list[0]);
        return addr;
    }

    addr.s_addr = INADDR_NONE;
    return addr;
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_host> <port>\n", argv[0]);
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  %s localhost 8080\n", argv[0]);
        fprintf(stderr, "  %s 192.168.1.100 8080\n", argv[0]);
        return 1;
    }

    const char *server_host = argv[1];
    int port = atoi(argv[2]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        return 1;
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        return 1;
    }

    struct in_addr server_addr_in = resolve_hostname(server_host);
    if (server_addr_in.s_addr == INADDR_NONE) {
        fprintf(stderr, "Cannot resolve hostname '%s'\n", server_host);
        close(client_socket);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = server_addr_in;
    server_addr.sin_port = htons(port);

    printf("Connecting to %s:%d...\n", inet_ntoa(server_addr_in), port);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_socket);
        return 1;
    }

    printf("Connected to server.\n");

    int result = receive_initial_response(client_socket);
    if (result == -1) {
        close(client_socket);
        return 1;
    }

    result = interactive_mode(client_socket);

    close(client_socket);
    printf("Client terminated cleanly\n");

    return result == -2 ? 0 : result;
}
