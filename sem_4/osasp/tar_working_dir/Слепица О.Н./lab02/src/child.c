/*
 * Дочерний процесс отображает информацию о себе и переменные окружения.
 * При получении параметра "env" читает переменные из файла конфигурации,
 * иначе выводит все полученные переменные окружения.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define ENV_CONFIG_FILE "env"

void display_env_from_config(const char *config_filename);
void display_environment_vars(char *env_vars[]);

int main(int argc, char *argv[], char *envp[]) {
    if (argc < 2) {
        printf("Usage: %s [env]\n", argv[0]);
        printf("  env - display environment variables from %s\n", ENV_CONFIG_FILE);
        return EXIT_FAILURE;
    }

    printf("Process Name: %s\n", argv[0]);
    printf("Process ID: %d\n", getpid());
    printf("Parent Process ID: %d\n", getppid());

    if (strcmp(argv[1], "env") == 0) {
        display_env_from_config(ENV_CONFIG_FILE);
    } else {
        display_environment_vars(envp);
    }

    return EXIT_SUCCESS;
}

void display_env_from_config(const char *config_filename) {
    FILE *file = fopen(config_filename, "r");
    if (!file) {
        perror("File opening error");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        char *value = getenv(line);
        if (value) {
            printf("%s=%s\n", line, value);
        }
    }
    fclose(file);
}

void display_environment_vars(char *env_vars[]) {
    for (int i = 0; env_vars[i]; i++) {
        printf("%s\n", env_vars[i]);
    }
}
