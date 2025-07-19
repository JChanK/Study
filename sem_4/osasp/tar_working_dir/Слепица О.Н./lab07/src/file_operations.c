#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "file_operations.h"
#include "data_structures.h"

#ifndef F_OFD_GETLK
#define F_OFD_GETLK     36
#define F_OFD_SETLK     37
#define F_OFD_SETLKW    38
#endif

int init_file(void) {
    struct stat st;
    
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        return -1;
    }
    
    if (st.st_size == 0) {
        printf("Создание тестовых записей...\n");
        
        record_t test_records[] = {
            {"Иван", "Москва", 3},
            {"Петр", "СПб", 2},
            {"Сидор", "Казань", 4},
            {"Николай", "Екатеринбург", 1},
            {"Анна", "Новосибирск", 3},
            {"Дмитрий", "Челябинск", 2},
            {"Елена", "Ростов-на-Дону", 4},
            {"Алексей", "Волгоград", 1},
            {"Мария", "Уфа", 3},
            {"Владимир", "Пермь", 2}
        };
        
        for (int i = 0; i < 10; i++) {
            if (write(fd, &test_records[i], sizeof(record_t)) != sizeof(record_t)) {
                perror("Ошибка записи тестовых данных");
                return -1;
            }
        }
        
        printf("Создано %d тестовых записей.\n", 10);
    }
    
    return 0;
}

int get_record(int rec_no, record_t *rec) {
    if (rec_no < 0 || rec == NULL) {
        return -1;
    }
    
    off_t offset = rec_no * sizeof(record_t);
    
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }
    
    ssize_t bytes_read = read(fd, rec, sizeof(record_t));
    if (bytes_read == 0) {
        return 1;
    } else if (bytes_read != sizeof(record_t)) {
        perror("read");
        return -1;
    }
    
    return 0;
}

int put_record(int rec_no, const record_t *rec) {
    if (rec_no < 0 || rec == NULL) {
        return -1;
    }
    
    off_t offset = rec_no * sizeof(record_t);
    
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }
    
    if (write(fd, rec, sizeof(record_t)) != sizeof(record_t)) {
        perror("write");
        return -1;
    }
    
    if (fsync(fd) == -1) {
        perror("fsync");
        return -1;
    }
    
    return 0;
}

int lock_record(int rec_no, int lock_type) {
    struct flock fl;
    
    fl.l_type = lock_type;
    fl.l_whence = SEEK_SET;
    fl.l_start = rec_no * sizeof(record_t);
    fl.l_len = sizeof(record_t);
    fl.l_pid = 0;
    
    if (fcntl(fd, F_OFD_SETLKW, &fl) == -1) {
        if (errno == EINVAL) {
            if (fcntl(fd, F_SETLKW, &fl) == -1) {
                if (errno == EDEADLK) {
                    printf("Обнаружена взаимная блокировка\n");
                } else {
                    perror("fcntl lock (F_SETLKW)");
                }
                return -1;
            }
        } else {
            if (errno == EDEADLK) {
                printf("Обнаружена взаимная блокировка\n");
            } else {
                perror("fcntl lock (F_OFD_SETLKW)");
            }
            return -1;
        }
    }
    
    return 0;
}

int unlock_record(int rec_no) {
    struct flock fl;
    
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = rec_no * sizeof(record_t);
    fl.l_len = sizeof(record_t);
    fl.l_pid = 0;
    
    if (fcntl(fd, F_OFD_SETLK, &fl) == -1) {
        if (errno == EINVAL) {
            if (fcntl(fd, F_SETLK, &fl) == -1) {
                perror("fcntl unlock (F_SETLK)");
                return -1;
            }
        } else {
            perror("fcntl unlock (F_OFD_SETLK)");
            return -1;
        }
    }
    
    return 0;
}

int get_total_records(void) {
    struct stat st;
    
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        return -1;
    }
    
    return st.st_size / sizeof(record_t);
}

void print_record(int rec_no, const record_t *rec) {
    printf("=== Запись #%d ===\n", rec_no);
    printf("Ф.И.О.:    %s\n", rec->name);
    printf("Адрес:     %s\n", rec->address);
    printf("Семестр:   %d\n", rec->semester);
    printf("==================\n");
}

void list_all_records(void) {
    int total = get_total_records();
    if (total <= 0) {
        printf("Файл пуст или произошла ошибка.\n");
        return;
    }
    
    printf("\n=== Список всех записей ===\n");
    
    for (int i = 0; i < total; i++) {
        record_t rec;
        
        if (lock_record(i, REC_LOCK_READ) != 0) {
            printf("Ошибка блокировки записи #%d\n", i);
            continue;
        }
        
        if (get_record(i, &rec) == 0) {
            print_record(i, &rec);
        } else {
            printf("Ошибка чтения записи #%d\n", i);
        }
        
        unlock_record(i);
    }
    
    printf("Всего записей: %d\n\n", total);
}
