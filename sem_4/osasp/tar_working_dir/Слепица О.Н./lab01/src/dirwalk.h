#ifndef DIRWALK_H
#define DIRWALK_H

#include <stdio.h>    // Для fprintf и stderr
#include <stdlib.h>   // Для exit, EXIT_FAILURE и strdup
#include <string.h>   // Для strcoll и strdup
#include <dirent.h>   // Для DIR и struct dirent
#include <sys/stat.h> // Для struct stat и lstat
#include <locale.h>   // Для setlocale
#include <errno.h>    // Для errno
#include <unistd.h>   // Для lstat и getopt

// Максимальная длина пути
#define PATH_MAX 4096

// Структура для хранения коллекции файлов
typedef struct {
    char **items;    // Массив строк (путей к файлам/директориям)
    size_t count;    // Текущее количество элементов
    size_t capacity; // Текущая емкость массива
} file_collection_t;  // Переименовано

// Структура для хранения опций фильтрации и сортировки
typedef struct {
    int show_links;  // Показывать символические ссылки
    int show_dirs;   // Показывать директории
    int show_files;  // Показывать файлы
    int sort;        // Сортировать результаты
} filter_options_t;  // Переименовано

// Прототипы функций
void fail(const char *message);
void parse_args(int argc, char *argv[], filter_options_t *options, const char **start_dir);
void add_file(file_collection_t *files, const char *path);
void clear_file_collection(file_collection_t *files);
int compare_file_names(const void *a, const void *b);
int matches_filter(const struct stat *file_info, const filter_options_t *options);
void scan_dir(const char *base_path, const filter_options_t *options, file_collection_t *files);

#endif // DIRWALK_H
