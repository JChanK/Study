#include "common.h"

int compare_index(const void *a, const void *b) {
    index_s *rec_a = (index_s *)a;
    index_s *rec_b = (index_s *)b;

    if (rec_a->time_mark < rec_b->time_mark) return -1;
    if (rec_a->time_mark > rec_b->time_mark) return 1;
    return 0;
}

void merge_blocks(index_s *dest, index_s *src1, size_t len1, index_s *src2, size_t len2) {
    size_t i = 0, j = 0, k = 0;
    
    while (i < len1 && j < len2) {
        if (src1[i].time_mark <= src2[j].time_mark) {
            dest[k++] = src1[i++];
        } else {
            dest[k++] = src2[j++];
        }
    }
    
    while (i < len1) dest[k++] = src1[i++];
    while (j < len2) dest[k++] = src2[j++];
}

void merge_sorted_chunks(int fd, size_t header_size, size_t record_size, uint64_t total_records) {
    size_t file_size = header_size + total_records * record_size;
    void *mapped = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Failed to map file for final merge");
        return;
    }
    
    index_s *records = (index_s *)((char *)mapped + header_size);

    index_s *temp = malloc(total_records * sizeof(index_s));
    if (!temp) {
        perror("Failed to allocate temp space for final merge");
        munmap(mapped, file_size);
        return;
    }
    
    for (size_t width = 1; width < total_records; width *= 2) {
        for (size_t i = 0; i < total_records; i += 2 * width) {
            size_t left = i;
            size_t mid = (i + width) < total_records ? (i + width) : total_records;
            size_t right = (i + 2 * width) < total_records ? (i + 2 * width) : total_records;
            
            merge_blocks(temp + left, 
                        records + left, mid - left,
                        records + mid, right - mid);
            
            memcpy(records + left, temp + left, 
                  (right - left) * sizeof(index_s));
        }
    }
    
    free(temp);
    msync(mapped, file_size, MS_SYNC);
    munmap(mapped, file_size);
}

int is_sorted(index_s *array, size_t size) {
    for (size_t i = 1; i < size; i++) {
        if (array[i-1].time_mark > array[i].time_mark) {
            fprintf(stderr, "Sort violation at %zu: %.5f > %.5f (recnos %lu > %lu)\n",
                   i, array[i-1].time_mark, array[i].time_mark,
                   array[i-1].recno, array[i].recno);
            return 0;
        }
    }
    return 1;
}
