#include "common.h"

typedef struct {
    int thread_id;              
    size_t block_size;          
    index_s *buffer;             
    int blocks;                 
    int threads;                
    pthread_barrier_t *barrier; 
    pthread_mutex_t *map_mutex;  
    int *block_map;              
    int *merge_map;              
    size_t records_in_mapping;   
} thread_info_t;

void *thread_worker(void *arg) {
    thread_info_t *info = (thread_info_t *)arg;

    pthread_barrier_wait(info->barrier);

    int block_to_sort = info->thread_id;

    if (block_to_sort >= info->blocks) {
        pthread_barrier_wait(info->barrier);
        pthread_barrier_wait(info->barrier);
        return NULL;
    }

    size_t block_offset = block_to_sort * info->block_size;
    size_t current_block_size = info->block_size;
    
    if (block_offset >= info->records_in_mapping) {
        pthread_barrier_wait(info->barrier);
        pthread_barrier_wait(info->barrier);
        return NULL;
    }
    
    if (block_offset + current_block_size > info->records_in_mapping) {
        current_block_size = info->records_in_mapping - block_offset;
    }
    
    if (current_block_size > 0) {
        index_s *block_start = info->buffer + block_offset;
        
        qsort(block_start, current_block_size, sizeof(index_s), compare_index);

        info->block_map[block_to_sort] = 1;
    }

    while (1) {
        int next_block = -1;

        pthread_mutex_lock(info->map_mutex);
        for (int i = 0; i < info->blocks; i++) {
            if (info->block_map[i] == 0) {
                info->block_map[i] = 1;
                next_block = i;
                break;
            }
        }
        pthread_mutex_unlock(info->map_mutex);

        if (next_block == -1) {
            break;
        }

        block_offset = next_block * info->block_size;
        current_block_size = info->block_size;
        
        if (block_offset >= info->records_in_mapping) {
            continue;
        }
        
        if (block_offset + current_block_size > info->records_in_mapping) {
            current_block_size = info->records_in_mapping - block_offset;
            if (current_block_size <= 0) {
                continue;
            }
        }
        
        index_s *block_start = info->buffer + block_offset;

        qsort(block_start, current_block_size, sizeof(index_s), compare_index);
    }

    pthread_barrier_wait(info->barrier);

    int blocks_remaining = info->blocks;
    size_t current_merge_size = info->block_size;

    while (blocks_remaining > 1) {
        int pairs = blocks_remaining / 2;
        int can_merge = 0;
        int block1 = -1, block2 = -1;

        if (info->threads <= pairs) {
            if (info->thread_id < pairs) {
                block1 = info->thread_id * 2;
                block2 = block1 + 1;
                can_merge = 1;
            }
        } else {
            pthread_mutex_lock(info->map_mutex);
            for (int i = 0; i < pairs; i++) {
                if (info->merge_map[i] == 0) {
                    info->merge_map[i] = 1;
                    block1 = i * 2;
                    block2 = block1 + 1;
                    can_merge = 1;
                    break;
                }
            }
            pthread_mutex_unlock(info->map_mutex);
        }

if (can_merge) {
    size_t block1_offset = block1 * current_merge_size;
    size_t block2_offset = block2 * current_merge_size;
    
    if (block1_offset >= info->records_in_mapping) continue;
    
    size_t block1_size = current_merge_size;
    if (block1_offset + block1_size > info->records_in_mapping) {
        block1_size = info->records_in_mapping - block1_offset;
    }
    
    if (block2_offset >= info->records_in_mapping) {
        continue;
    }
    
    size_t block2_size = current_merge_size;
    if (block2_offset + block2_size > info->records_in_mapping) {
        block2_size = info->records_in_mapping - block2_offset;
    }
    
    index_s *temp = malloc((block1_size + block2_size) * sizeof(index_s));
    if (!temp) {
        perror("Failed to allocate temp space for merge");
        pthread_exit(NULL);
    }
    
    merge_blocks(temp, 
                info->buffer + block1_offset, block1_size,
                info->buffer + block2_offset, block2_size);
    
    memcpy(info->buffer + block1_offset, temp, 
          (block1_size + block2_size) * sizeof(index_s));
    free(temp);
}

        pthread_barrier_wait(info->barrier);

        if (info->thread_id == 0) {
            memset(info->merge_map, 0, info->blocks * sizeof(int));
        }

        blocks_remaining = (blocks_remaining + 1) / 2;
        current_merge_size *= 2;

        pthread_barrier_wait(info->barrier);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <memsize> <blocks> <threads> <filename>\n", argv[0]);
        return 1;
    }

    size_t memsize = strtoull(argv[1], NULL, 10);
    int blocks = atoi(argv[2]);
    int threads = atoi(argv[3]);
    const char *filename = argv[4];

    int page_size = sysconf(_SC_PAGESIZE);
    if (memsize % page_size != 0) {
        fprintf(stderr, "Error: memsize must be a multiple of page size (%d)\n", page_size);
        return 1;
    }

    if ((blocks & (blocks - 1)) != 0) {
        fprintf(stderr, "Error: blocks must be a power of 2\n");
        return 1;
    }

    if (blocks < threads * 4) {
        fprintf(stderr, "Error: blocks must be at least 4 times the number of threads\n");
        return 1;
    }

    int fd = open(filename, O_RDWR);
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

    size_t header_size = sizeof(uint64_t); 
    size_t file_size = st.st_size;
    
    uint64_t total_records = 0;
    if (pread(fd, &total_records, sizeof(uint64_t), 0) != sizeof(uint64_t)) {
        perror("Failed to read record count from file");
        close(fd);
        return 1;
    }
    
    size_t record_size = sizeof(index_s);
    size_t data_size = total_records * record_size;

    if (header_size + data_size != file_size) {
        fprintf(stderr, "Warning: File size mismatch. Expected %zu bytes, got %zu bytes.\n",
                header_size + data_size, file_size);
        data_size = file_size - header_size;
        total_records = data_size / record_size;
    }

    if (memsize > data_size) {
        memsize = data_size;
        printf("Adjusting memsize to file data size: %zu bytes\n", memsize);
    }

    size_t records_per_mapping = memsize / record_size;
    size_t records_per_block = records_per_mapping / blocks;

    if (records_per_block == 0) {
        fprintf(stderr, "Error: records_per_block is zero (increase memsize or reduce blocks)\n");
        close(fd);
        return 1;
    }

    printf("File: %s\n", filename);
    printf("File size: %zu bytes (%.2f MB)\n", file_size, (double)file_size / (1024 * 1024));
    printf("Total records from header: %lu\n", total_records);
    printf("Memory mapping size: %zu bytes (%.2f MB)\n", memsize, (double)memsize / (1024 * 1024));
    printf("Blocks: %d\n", blocks);
    printf("Records per block: %zu\n", records_per_block);
    printf("Threads: %d\n", threads);

    pthread_barrier_t barrier;
    pthread_mutex_t map_mutex;
    if (pthread_barrier_init(&barrier, NULL, threads) != 0) {
        perror("Failed to initialize barrier");
        close(fd);
        return 1;
    }
    if (pthread_mutex_init(&map_mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        pthread_barrier_destroy(&barrier);
        close(fd);
        return 1;
    }

    int *block_map = calloc(blocks, sizeof(int));
    int *merge_map = calloc(blocks, sizeof(int));
    if (!block_map || !merge_map) {
        perror("Failed to allocate memory for block maps");
        free(block_map);
        free(merge_map);
        pthread_barrier_destroy(&barrier);
        pthread_mutex_destroy(&map_mutex);
        close(fd);
        return 1;
    }

    thread_info_t *thread_info = calloc(threads, sizeof(thread_info_t));
    pthread_t *thread_ids = calloc(threads, sizeof(pthread_t));
    if (!thread_info || !thread_ids) {
        perror("Failed to allocate memory for thread info");
        free(block_map);
        free(merge_map);
        free(thread_info);
        free(thread_ids);
        pthread_barrier_destroy(&barrier);
        pthread_mutex_destroy(&map_mutex);
        close(fd);
        return 1;
    }

    size_t offset = header_size;
    size_t processed_records = 0;
    
    while (processed_records < total_records) {
        size_t records_this_chunk = (total_records - processed_records < records_per_mapping) ?
                                   (total_records - processed_records) : records_per_mapping;
        
        if (records_this_chunk == 0) {
            break;
        }
        
        size_t bytes_to_map = records_this_chunk * record_size;
        
        size_t aligned_offset = (offset / page_size) * page_size;
        size_t offset_adjustment = offset - aligned_offset;
        size_t aligned_size = bytes_to_map + offset_adjustment;
        
        aligned_size = (aligned_size + page_size - 1) / page_size * page_size;

        if (aligned_offset + aligned_size > file_size) {
            aligned_size = file_size - aligned_offset;
        }

        void *mapped_base = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE, 
                              MAP_SHARED, fd, aligned_offset);
        if (mapped_base == MAP_FAILED) {
            perror("Failed to map file");
            free(block_map);
            free(merge_map);
            free(thread_info);
            free(thread_ids);
            pthread_barrier_destroy(&barrier);
            pthread_mutex_destroy(&map_mutex);
            close(fd);
            return 1;
        }

        index_s *mapped_data = (index_s *)((char *)mapped_base + offset_adjustment);

        memset(block_map, 0, blocks * sizeof(int));
        memset(merge_map, 0, blocks * sizeof(int));

        for (int i = 0; i < threads && i < blocks; i++) {
            block_map[i] = 1;
        }

        for (int i = 0; i < threads; i++) {
            thread_info[i].thread_id = i;
            thread_info[i].block_size = records_per_block;
            thread_info[i].buffer = mapped_data;
            thread_info[i].blocks = blocks;
            thread_info[i].threads = threads;
            thread_info[i].barrier = &barrier;
            thread_info[i].map_mutex = &map_mutex;
            thread_info[i].block_map = block_map;
            thread_info[i].merge_map = merge_map;
            thread_info[i].records_in_mapping = records_this_chunk;
        }

        for (int i = 1; i < threads; i++) {
            if (pthread_create(&thread_ids[i], NULL, thread_worker, &thread_info[i]) != 0) {
                perror("Failed to create thread");
                munmap(mapped_base, aligned_size);
                free(block_map);
                free(merge_map);
                free(thread_info);
                free(thread_ids);
                pthread_barrier_destroy(&barrier);
                pthread_mutex_destroy(&map_mutex);
                close(fd);
                return 1;
            }
        }

        thread_worker(&thread_info[0]);

        for (int i = 1; i < threads; i++) {
            pthread_join(thread_ids[i], NULL);
        }

        if (msync(mapped_base, aligned_size, MS_SYNC) != 0) {
            perror("Failed to sync memory to disk");
        }

        if (munmap(mapped_base, aligned_size) != 0) {
            perror("Failed to unmap memory");
        }

        offset += bytes_to_map;
        processed_records += records_this_chunk;

        printf("Processed chunk of %zu records (%.2f MB). Remaining: %zu records\n",
               records_this_chunk,
               (double)bytes_to_map / (1024 * 1024),
               total_records - processed_records);
    }

    printf("Performing final merge of all chunks...\n");
    merge_sorted_chunks(fd, header_size, record_size, total_records);

    printf("Verifying final sort...\n");

    int sorted = 1;
    double last_time_mark = -1.0;

    lseek(fd, header_size, SEEK_SET);

    size_t verify_chunk_size = 65536;
    if (verify_chunk_size > total_records) {
        verify_chunk_size = total_records;
    }
    
    index_s *verify_buffer = malloc(verify_chunk_size * sizeof(index_s));
    if (!verify_buffer) {
        perror("Failed to allocate verification buffer");
        close(fd);
        return 1;
    }

    processed_records = 0;
    while (processed_records < total_records && sorted) {
        size_t records_to_read = (processed_records + verify_chunk_size > total_records) ?
                               (total_records - processed_records) : verify_chunk_size;

        ssize_t bytes_read = read(fd, verify_buffer, records_to_read * sizeof(index_s));
        if (bytes_read < 0 || (size_t)bytes_read != records_to_read * sizeof(index_s)) {
            fprintf(stderr, "Failed to read records during verification: expected %zu bytes, got %zd\n",
                   records_to_read * sizeof(index_s), bytes_read);
            free(verify_buffer);
            close(fd);
            return 1;
        }

        for (size_t j = 0; j < records_to_read; j++) {
            if (j == 0 && processed_records > 0) {
                if (verify_buffer[j].time_mark < last_time_mark) {
                    sorted = 0;
                    printf("Discontinuity at record %zu: %.5f < %.5f\n", 
                           processed_records + j, verify_buffer[j].time_mark, last_time_mark);
                    break;
                }
            } else if (j > 0) {
                if (verify_buffer[j].time_mark < verify_buffer[j-1].time_mark) {
                    sorted = 0;
                    printf("Disorder at record %zu: %.5f < %.5f\n", 
                           processed_records + j, verify_buffer[j].time_mark, verify_buffer[j-1].time_mark);
                    break;
                }
            }

            last_time_mark = verify_buffer[j].time_mark;
        }

        processed_records += records_to_read;
    }

    free(verify_buffer);
    printf("File is %s\n", sorted ? "SORTED" : "NOT SORTED");

    free(block_map);
    free(merge_map);
    free(thread_info);
    free(thread_ids);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&map_mutex);
    close(fd);

    printf("Sorting completed successfully.\n");
    return 0;
}
