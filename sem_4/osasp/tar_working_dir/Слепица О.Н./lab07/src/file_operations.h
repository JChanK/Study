#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "data_structures.h"

int init_file(void);
int get_record(int rec_no, record_t *rec);
int put_record(int rec_no, const record_t *rec);
int lock_record(int rec_no, int lock_type);
int unlock_record(int rec_no);
int get_total_records(void);
void print_record(int rec_no, const record_t *rec);
void list_all_records(void);

#define REC_LOCK_READ F_RDLCK
#define REC_LOCK_WRITE F_WRLCK
#define REC_LOCK_UNLOCK F_UNLCK

#endif
