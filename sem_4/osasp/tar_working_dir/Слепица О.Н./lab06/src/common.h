#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200112L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

typedef struct {
    double time_mark;   
    uint64_t recno;     
} index_s;

typedef struct {
    uint64_t records;   
    index_s idx[];       
} index_hdr_s;

int compare_index(const void *a, const void *b);

void merge_blocks(index_s *dest, index_s *src1, size_t len1, index_s *src2, size_t len2);
void merge_sorted_chunks(int fd, size_t header_size, size_t record_size, uint64_t total_records);
int is_sorted(index_s *array, size_t size);

#endif // COMMON_H
