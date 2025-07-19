#define _XOPEN_SOURCE 700 // Включение расширенных возможностей POSIX для совместимости

#include <stdio.h>
#include <stdlib.h>
#include "dirwalk.h"

/*
 * Программа для обхода директорий и фильтрации файлов.
 * Позволяет показывать символические ссылки, каталоги и файлы,
 * а также сортировать результаты.
 */

// Главная функция программы
int main(int argc, char *argv[]) {
    // Установка локали для корректной сортировки
    setlocale(LC_COLLATE, "");

    // Проверка наличия аргументов командной строки
    if (argc < 2) {
        fprintf(stderr, "Использование: %s [опции] [директория]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Инициализация коллекции файлов и опций фильтрации
    file_collection_t files = {0};
    filter_options_t options = {0};
    const char *start_dir = NULL;

    // Анализ аргументов командной строки
    parse_args(argc, argv, &options, &start_dir);

    // Получение информации о начальной директории
    struct stat file_info;
    if (lstat(start_dir, &file_info) < 0) {
        fail(strerror(errno)); // Обработка ошибки
    }

    // Если начальная директория соответствует фильтру, добавляем её
    if (matches_filter(&file_info, &options)) {
        add_file(&files, start_dir);
    }

    // Если начальная директория является каталогом, сканируем её
    if (S_ISDIR(file_info.st_mode)) {
        scan_dir(start_dir, &options, &files);
    }

    // Если включена сортировка, сортируем коллекцию файлов
    if (options.sort) {
        qsort(files.items, files.count, sizeof(char *), compare_file_names);
    }

    // Вывод коллекции файлов
    for (size_t i = 0; i < files.count; i++) {
        printf("%s\n", files.items[i]);
    }

    // Очистка коллекции файлов
    clear_file_collection(&files);

    return 0;
}
