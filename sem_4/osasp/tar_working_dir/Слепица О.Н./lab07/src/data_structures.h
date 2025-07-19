#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <fcntl.h> 
#include <sys/file.h>   
#include <stdint.h>
#include <unistd.h>

#define MAX_NAME_LEN 80
#define MAX_ADDRESS_LEN 80
#define MAX_RECORDS 100
#define DATA_FILE "students.dat"

#if defined(F_OFD_SETLK)
    #define USE_OFD_LOCKS 1
    #define LOCK_CMD_SETLK F_OFD_SETLK
    #define LOCK_CMD_SETLKW F_OFD_SETLKW
#else
    #define USE_OFD_LOCKS 0
    #define LOCK_CMD_SETLK F_SETLK
    #define LOCK_CMD_SETLKW F_SETLKW
#endif

typedef struct {
    char name[MAX_NAME_LEN];        
    char address[MAX_ADDRESS_LEN];  
    uint8_t semester;               
} record_t;

extern int fd;
extern record_t original_file_record;
extern record_t current_record;
extern int current_record_no;
extern int record_modified;

#endif
