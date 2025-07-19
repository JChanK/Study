#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <index_file>\n", argv[0]);
        return 1;
    }
    
    const char *filename = argv[1];
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open index file");
        return 1;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Failed to get file stat");
        close(fd);
        return 1;
    }
    
    void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Failed to map file");
        close(fd);
        return 1;
    }
    
    index_hdr_s *header = (index_hdr_s *)mapped;
    uint64_t num_records = header->records;
    
    printf("Index file: %s\n", filename);
    printf("Total records: %lu\n", num_records);
    printf("File size: %.2f MB\n", (double)st.st_size / (1024 * 1024));
    printf("\n%-15s %-20s\n", "Time Mark", "Record Number");
    printf("------------------------------------\n");
    
    uint64_t display_count = (num_records > 10) ? 10 : num_records;
    for (uint64_t i = 0; i < display_count; i++) {
        printf("%-15.5f %-20lu\n", header->idx[i].time_mark, header->idx[i].recno);
    }
    
    if (num_records > 10) {
        printf("...\n");
        printf("%-15.5f %-20lu (last record)\n", 
               header->idx[num_records-1].time_mark, 
               header->idx[num_records-1].recno);
    }
    
    int is_sorted = 1;
    uint64_t sort_violations = 0;
    for (uint64_t i = 1; i < num_records; i++) {
       if (header->idx[i-1].time_mark > header->idx[i].time_mark) {
           is_sorted = 0;
           sort_violations++;
       }
    }
    printf("\nThe index is %s (%lu violations)\n", is_sorted ? "SORTED" : "NOT SORTED", sort_violations);
    
    munmap(mapped, st.st_size);
    close(fd);
    
    return 0;
}
