//user_interface.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "user_interface.h"
#include "file_operations.h"
#include "data_structures.h"

void show_menu(void) {
    printf("\n=== МЕНЮ ===\n");
    printf("1. LST  - Показать все записи\n");
    printf("2. GET  - Получить запись по номеру\n");
    printf("3. MOD  - Модифицировать текущую запись\n");
    printf("4. PUT  - Сохранить изменения\n");
    printf("5. EXIT - Выход\n");
    printf("============\n");

    if (current_record_no >= 0) {
        printf("Текущая запись: #%d %s\n",
               current_record_no,
               record_modified ? "(изменена)" : "");
    }

    printf("Выберите команду: ");
}

void clear_input(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int get_user_choice(void) {
    int choice;
    if (scanf("%d", &choice) != 1) {
        clear_input();
        return -1;
    }
    clear_input();
    return choice;
}

void handle_list_command(void) {
    printf("\nВыполняется команда LST...\n");
    list_all_records();
}

void handle_get_command(void) {
    int rec_no;
    printf("Введите номер записи: ");

    if (scanf("%d", &rec_no) != 1) {
        printf("Неверный формат номера записи.\n");
        clear_input();
        return;
    }
    clear_input();

    if (rec_no < 0) {
        printf("Номер записи должен быть >= 0.\n");
        return;
    }

    if (record_modified) {
        printf("ВНИМАНИЕ: Текущая запись была изменена но не сохранена!\n");
        printf("Продолжить? (y/n): ");
        char answer;
        if (scanf("%c", &answer) == 1 && (answer == 'y' || answer == 'Y')) {
            clear_input();
        } else {
            clear_input();
            return;
        }
    }

    printf("Получение записи #%d...\n", rec_no);

    if (lock_record(rec_no, REC_LOCK_READ) != 0) {
        printf("Ошибка блокировки записи #%d\n", rec_no);
        return;
    }

    record_t rec;
    int result = get_record(rec_no, &rec);

    if (result == 0) {
        current_record = rec;
        original_file_record = rec;
        current_record_no = rec_no;
        record_modified = 0;

        print_record(rec_no, &rec);
        printf("Запись #%d загружена в буфер.\n", rec_no);
    } else if (result == 1) {
        printf("Запись #%d не найдена (конец файла).\n", rec_no);
    } else {
        printf("Ошибка чтения записи #%d.\n", rec_no);
    }

    // Разблокируем запись
    unlock_record(rec_no);
}

void handle_modify_command(void) {
    if (current_record_no < 0) {
        printf("Сначала загрузите запись командой GET.\n");
        return;
    }

    printf("\n=== Модификация записи #%d ===\n", current_record_no);
    printf("Текущие данные:\n");
    print_record(current_record_no, &current_record);

    record_t working_record = current_record;

    printf("\nВведите новые данные (Enter - оставить без изменений):\n");

    printf("Ф.И.О. [%s]: ", working_record.name);
    char input[256];
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Удаляем символ новой строки
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            strncpy(working_record.name, input, MAX_NAME_LEN - 1);
            working_record.name[MAX_NAME_LEN - 1] = '\0';
        }
    }

    printf("Адрес [%s]: ", working_record.address);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            strncpy(working_record.address, input, MAX_ADDRESS_LEN - 1);
            working_record.address[MAX_ADDRESS_LEN - 1] = '\0';
        }
    }

    printf("Семестр [%d]: ", working_record.semester);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            int semester = atoi(input);
            if (semester > 0 && semester <= 10) {
                working_record.semester = (uint8_t)semester;
            } else {
                printf("Семестр должен быть от 1 до 10. Оставляем старое значение.\n");
            }
        }
    }

    if (memcmp(&current_record, &working_record, sizeof(record_t)) != 0) {
        current_record = working_record;
        record_modified = 1;
        printf("\nЗапись изменена в буфере. Используйте PUT для сохранения.\n");
        printf("Новые данные:\n");
        print_record(current_record_no, &current_record);
    } else {
        printf("\nИзменений не обнаружено.\n");
    }
}

void handle_put_command(void) {
    if (current_record_no < 0) {
        printf("Нет записи для сохранения. Используйте GET для загрузки записи.\n");
        return;
    }

    if (!record_modified) {
        printf("Запись не была изменена.\n");
        return;
    }

    printf("Сохранение записи #%d...\n", current_record_no);

Again:
    if (lock_record(current_record_no, REC_LOCK_WRITE) != 0) {
        printf("Ошибка блокировки записи для записи.\n");
        return;
    }

    record_t rec_current_from_file;
    if (get_record(current_record_no, &rec_current_from_file) != 0) {
        printf("Ошибка чтения записи из файла.\n");
        unlock_record(current_record_no);
        return;
    }

    if (memcmp(&original_file_record, &rec_current_from_file, sizeof(record_t)) != 0) {
        unlock_record(current_record_no);

        printf("\nВНИМАНИЕ: Запись была изменена другим процессом!\n");
        printf("Версия при загрузке:\n");
        print_record(current_record_no, &original_file_record);
        printf("Текущие данные в файле:\n");
        print_record(current_record_no, &rec_current_from_file);
        printf("Ваши изменения:\n");
        print_record(current_record_no, &current_record);

        printf("\nВыберите действие:\n");
        printf("1. Перезаписать изменения другого процесса\n");
        printf("2. Отменить свои изменения\n");
        printf("3. Попробовать снова (повторить чтение)\n");
        printf("Ваш выбор: ");

        int choice = get_user_choice();

        switch (choice) {
            case 1:
                original_file_record = rec_current_from_file;
                goto Again;

            case 2:
                current_record = rec_current_from_file;
                original_file_record = rec_current_from_file;
                record_modified = 0;
                printf("Изменения отменены. Загружена актуальная версия.\n");
                return;

            case 3:
                original_file_record = rec_current_from_file;
                printf("Повторяем операцию с актуальной версией записи.\n");
                goto Again;

            default:
                printf("Операция отменена.\n");
                return;
        }
    }

    if (put_record(current_record_no, &current_record) == 0) {
        printf("Запись #%d успешно сохранена.\n", current_record_no);
        record_modified = 0;
        original_file_record = current_record;
    } else {
        printf("Ошибка сохранения записи.\n");
    }

    unlock_record(current_record_no);
}

void run_main_loop(void) {
    int choice;

    while (1) {
        show_menu();
        choice = get_user_choice();

        switch (choice) {
            case 1:
                handle_list_command();
                break;

            case 2:
                handle_get_command();
                break;

            case 3:
                handle_modify_command();
                break;

            case 4:
                handle_put_command();
                break;

            case 5:
                printf("Завершение работы...\n");
                if (record_modified) {
                    printf("ВНИМАНИЕ: У вас есть несохраненные изменения!\n");
                    printf("Сохранить? (y/n): ");
                    char answer;
                    if (scanf("%c", &answer) == 1 && (answer == 'y' || answer == 'Y')) {
                        handle_put_command();
                    }
                    clear_input();
                }
                return;

            default:
                printf("Неверный выбор. Попробуйте снова.\n");
                break;
        }
    }
}
