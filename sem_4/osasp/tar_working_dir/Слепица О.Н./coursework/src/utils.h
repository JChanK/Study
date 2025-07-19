#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ext2fs/ext2_fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "analyzer.h"

void format_value(uint64_t value, char *buffer, size_t buffer_size, bool is_size);

bool check_bitmap_bit(const unsigned char *bitmap, uint32_t bit_num);

void set_bitmap_bit(unsigned char *bitmap, uint32_t bit_num);

void clear_bitmap_bit(unsigned char *bitmap, uint32_t bit_num);

void superblock_to_string(const struct ext2_super_block *sb, char *buffer, size_t buffer_size);

void group_desc_to_string(const struct ext2_group_desc *gd, char *buffer, size_t buffer_size);

void inode_to_string(const struct ext2_inode *inode, char *buffer, size_t buffer_size);

void get_fs_type_string(const fs_info_t *fs_info, char *buffer, size_t buffer_size);

#endif /* UTILS_H */
