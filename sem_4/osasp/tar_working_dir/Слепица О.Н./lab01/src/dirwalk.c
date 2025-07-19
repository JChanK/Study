#define _XOPEN_SOURCE 700 // Включение расширенных возможностей POSIX
#include "dirwalk.h"

// Завершение программы с ошибкой
void fail(const char *message) {
    fprintf(stderr, "Ошибка: %s\n", message);
    exit(EXIT_FAILURE);
}

// Анализ аргументов командной строки
void parse_args(int argc, char *argv[], filter_options_t *options, const char **start_dir) {
    int opt;
    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
            case 'l': options->show_links = 1; break; // Показывать символические ссылки
            case 'd': options->show_dirs = 1; break;  // Показывать директории
            case 'f': options->show_files = 1; break; // Показывать файлы
            case 's': options->sort = 1; break;        // Сортировать результаты
            default:
                fprintf(stderr, "Использование: %s [опции] [директория]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    *start_dir = (optind < argc) ? argv[optind] : "."; // Установка начальной директории
}

// Добавление файла в коллекцию
void add_file(file_collection_t *files, const char *path) {
    // Проверка, нужно ли увеличивать емкость коллекции
    if (files->count == files->capacity) {
        // Увеличиваем емкость вдвое или устанавливаем начальную емкость
        size_t new_capacity = (files->capacity == 0) ? 16 : files->capacity * 2;
        // Перераспределение памяти
        char **tmp = realloc(files->items, new_capacity * sizeof(char*));
        if (!tmp) { // Проверка на успешное перераспределение
            fail("Ошибка выделения памяти");
        }
        files->items = tmp; // Обновляем указатель на массив
        files->capacity = new_capacity; // Обновляем емкость
    }
    // Копируем строку пути и добавляем в коллекцию
    files->items[files->count] = strdup(path);
    if (!files->items[files->count]) { // Проверка на успешное дублирование строки
        fail("Ошибка дублирования строки");
    }
    files->count++; // Увеличиваем счетчик элементов
}

// Очистка коллекции файлов
void clear_file_collection(file_collection_t *files) {
    if (files->items) {
        for (size_t i = 0; i < files->count; i++) {
            free(files->items[i]); // Освобождаем каждый элемент
        }
        free(files->items); // Освобождаем массив
        files->items = NULL; // Обнуляем указатель
    }
    files->count = 0;
    files->capacity = 0;
}

// Функция сравнения для сортировки
int compare_file_names(const void *a, const void *b) {
    const char * const *path_a = a;
    const char * const *path_b = b;
    return strcoll(*path_a, *path_b); // Сравнение строк с учетом локали
}

// Проверка, соответствует ли файл фильтру
int matches_filter(const struct stat *file_info, const filter_options_t *options) {
    // Если данные некорректны, считаем, что файл не подходит
    if (!file_info || !options) return 0;

    // Проверяем, задан ли хотя бы один фильтр
    int any_filter = options->show_links || options->show_dirs || options->show_files;

    // Проверка на соответствие типу файла
    if (options->show_links && S_ISLNK(file_info->st_mode)) return 1; // Символическая ссылка
    if (options->show_dirs && S_ISDIR(file_info->st_mode)) return 1; // Директория
    if (options->show_files && S_ISREG(file_info->st_mode)) return 1; // Обычный файл

    // Если не указаны специфичные фильтры, разрешаем все типы
    if (!any_filter) return 1;

    return 0; // Файл не соответствует фильтрам
}

// Сканирование директории
void scan_dir(const char *base_path, const filter_options_t *options, file_collection_t *files) {
    DIR *dir = opendir(base_path); // Открываем директорию
    if (!dir) {
        fprintf(stderr, "Ошибка при открытии каталога '%s': %s\n", base_path, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Пропускаем "." и ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[PATH_MAX]; // Массив для хранения полного пути
        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name); // Формируем полный путь

        struct stat file_info;
        // Используем lstat, чтобы не следовать символическим ссылкам
        if (lstat(path, &file_info) < 0) {
            fprintf(stderr, "Ошибка при lstat '%s': %s\n", path, strerror(errno));
            continue;
        }

        // Если объект удовлетворяет фильтру, добавляем его
        if (matches_filter(&file_info, options)) {
            add_file(files, path);
        }

        // Если это каталог, за исключением символических ссылок, сканируем рекурсивно
        if (S_ISDIR(file_info.st_mode)) {
            scan_dir(path, options, files);
        }
    }
    closedir(dir); // Закрываем директорию
}
