#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_records> <output_file>\n", argv[0]);
        return 1;
    }
    
    uint64_t num_records = strtoull(argv[1], NULL, 10);
    const char *filename = argv[2];
    
    if (num_records % 256 != 0) {
        num_records = (num_records / 256 + 1) * 256;
        printf("Adjusting record count to %lu to make it a multiple of 256\n", num_records);
    }
    
    size_t file_size = sizeof(index_hdr_s) + num_records * sizeof(index_s);
    
    void *file_content = malloc(file_size);
    if (!file_content) {
        perror("Failed to allocate memory");
        return 1;
    }
    
    index_hdr_s *header = (index_hdr_s *)file_content;
    header->records = num_records;
    
    srand(time(NULL));
    
    for (uint64_t i = 0; i < num_records; i++) {
        
        time_t now = time(NULL);
        struct tm *tm_now = localtime(&now);
        double current_mjd = 15020.0 + (tm_now->tm_year - 0) * 365.25 + tm_now->tm_yday;
        
        double int_part = 15020.0 + ((double)rand() / RAND_MAX) * (current_mjd - 15020.0 - 1.0);
        double frac_part = (double)rand() / RAND_MAX;
        
        header->idx[i].time_mark = (int)int_part + frac_part;
        
        header->idx[i].recno = i + 1;
    }
    
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open output file");
        free(file_content);
        return 1;
    }
    
    if (fwrite(file_content, file_size, 1, file) != 1) {
        perror("Failed to write to file");
        fclose(file);
        free(file_content);
        return 1;
    }
    
    printf("Generated index file '%s' with %lu records (%.2f MB)\n", 
           filename, num_records, (double)file_size / (1024 * 1024));
    
    fclose(file);
    free(file_content);
    return 0;
}
