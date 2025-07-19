/*
 * Данная программа упорядочивает и отображает переменные окружения,
 * затем позволяет запускать дочерние процессы с разными вариантами окружения
 * в зависимости от выбора пользователя.
 */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <locale.h>

#define MAX_PROCESSES 100
#define PROCESS_NAME_SIZE 20
#define PROGRAM_PATH_SIZE 255

extern char **environ;

int string_comparator(const void *first, const void *second);
char **generate_process_environment(const char *env_config_file);

int main(int argc, char *argv[], char *envp[]) {
    if (argc < 2) {
        printf("Usage: %s <env_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *env_config_file = argv[1];

    setlocale(LC_COLLATE, "C");
    setenv("LC_COLLATE", "C", 1);

    int env_var_count = 0;
    while (environ[env_var_count]) env_var_count++;

    char **ordered_env = malloc((env_var_count + 1) * sizeof(char *));
    if (!ordered_env) {
        perror("Memory allocation error");
        return EXIT_FAILURE;
    }

    for (int idx = 0; idx < env_var_count; idx++) {
        ordered_env[idx] = environ[idx];
    }
    ordered_env[env_var_count] = NULL;

    qsort(ordered_env, env_var_count, sizeof(char *), string_comparator);

    for (int idx = 0; ordered_env[idx]; idx++) {
        puts(ordered_env[idx]);
    }
    free(ordered_env);

    int process_count = 0;
    char user_choice;

    while (1) {
        printf("\nOptions Menu:\n");
        printf("+ - Start process using getenv() path\n");
        printf("* - Start process using envp path\n");
        printf("& - Start process using environ path\n");
        printf("q - Quit program\n");
        printf("Your selection: ");
        scanf(" %c", &user_choice);

        if (user_choice == 'q') {
            break;
        }

        if (user_choice != '+' && user_choice != '*' && user_choice != '&') {
            printf("Invalid option selected\n");
            continue;
        }

        if (process_count >= MAX_PROCESSES) {
            printf("Process limit reached\n");
            continue;
        }

        char *program_location = NULL;
        if (user_choice == '+') {
            program_location = getenv("CHILD_PATH");
        } else if (user_choice == '*') {
            for (int idx = 0; envp[idx]; idx++) {
                if (strncmp(envp[idx], "CHILD_PATH=", 11) == 0) {
                    program_location = envp[idx] + 11;
                    break;
                }
            }
        } else if (user_choice == '&') {
            for (int idx = 0; environ[idx]; idx++) {
                if (strncmp(environ[idx], "CHILD_PATH=", 11) == 0) {
                    program_location = environ[idx] + 11;
                    break;
                }
            }
        }

        if (!program_location) {
            printf("CHILD_PATH environment variable missing\n");
            continue;
        }

        char process_id[PROCESS_NAME_SIZE];
        snprintf(process_id, sizeof(process_id), "child_%02d", process_count);

        char executable_path[PROGRAM_PATH_SIZE];
        snprintf(executable_path, sizeof(executable_path), "%s/child", program_location);

        char *process_args[] = {process_id, "env", NULL};

        pid_t new_pid = fork();
        if (new_pid == -1) {
            perror("Process creation error");
        } else if (new_pid > 0) {
            process_count++;
            waitpid(new_pid, NULL, 0);
        } else {
                if (user_choice == '+') {
                char **custom_env = generate_process_environment(env_config_file);
                if (!custom_env) {
                    exit(EXIT_FAILURE);
                }
                execve(executable_path, process_args, custom_env);
                perror("Execution failed");
                for (int idx = 0; custom_env[idx]; idx++) free(custom_env[idx]);
                free(custom_env);
            }else if(user_choice == '*'){
                execve(executable_path, process_args, envp);
                perror("Execution failed");
            }else {
                execve(executable_path, process_args, environ);
                perror("Execution failed");
            }
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

int string_comparator(const void *first, const void *second) {
    return strcmp(*(const char **)first, *(const char **)second);
}

char **generate_process_environment(const char *env_config_file) {
    char **new_env = malloc(sizeof(char *));
    if (!new_env) {
        perror("Memory allocation error");
        return NULL;
    }
    new_env[0] = NULL; // Initialize as empty environment

    FILE *config_file = fopen(env_config_file, "r");
    if (!config_file) {
        perror("Failed to open environment config file");
        free(new_env);
        return NULL;
    }

    int current_idx = 0;
    char config_line[256];

    while (fgets(config_line, sizeof(config_line), config_file)) {
        // Remove newline character
        config_line[strcspn(config_line, "\n")] = 0;

        // Skip empty lines
        if (strlen(config_line) == 0) continue;

        char *var_value = getenv(config_line);
        if (var_value) {
            // Resize the environment array
            char **temp = realloc(new_env, (current_idx + 2) * sizeof(char *));
            if (!temp) {
                perror("Memory reallocation error");
                fclose(config_file);
                for (int k = 0; k < current_idx; k++) free(new_env[k]);
                free(new_env);
                return NULL;
            }
            new_env = temp;

            // Allocate memory for the new environment variable
            new_env[current_idx] = malloc(strlen(config_line) + strlen(var_value) + 2);
            if (!new_env[current_idx]) {
                perror("Memory allocation error");
                fclose(config_file);
                for (int k = 0; k < current_idx; k++) free(new_env[k]);
                free(new_env);
                return NULL;
            }

            // Format the environment variable
            sprintf(new_env[current_idx], "%s=%s", config_line, var_value);
            current_idx++;
        }
    }

    fclose(config_file);
    new_env[current_idx] = NULL; // NULL-terminate the array

    return new_env;
}
