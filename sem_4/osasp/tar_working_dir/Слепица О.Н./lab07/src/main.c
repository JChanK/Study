#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#include "data_structures.h"
#include "file_operations.h"
#include "user_interface.h"

int fd = -1;
record_t original_file_record;
record_t current_record;
int current_record_no = -1;
int record_modified = 0;

static void handle_signal(int sig) {
    printf("\nПолучен сигнал %d. Закрываем файл и завершаем работу...\n", sig);
    if (fd != -1) {
        close(fd);
    }
    exit(EXIT_SUCCESS);
}

int main(void) {
    printf("=== Лабораторная работа №7: Блокировки чтения/записи ===\n");
    printf("Программа для работы с записями студентов\n\n");

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    fd = open(DATA_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(EXIT_FAILURE);
    }

    if (init_file() != 0) {
        fprintf(stderr, "Ошибка инициализации файла\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Файл данных успешно открыт: %s\n", DATA_FILE);
    printf("Доступно записей: %d\n\n", get_total_records());

    run_main_loop();

    if (fd != -1) {
        close(fd);
    }

    printf("Программа завершена.\n");
    return EXIT_SUCCESS;
}
