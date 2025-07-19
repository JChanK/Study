#ifndef ANALYZER_H
#define ANALYZER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ext2fs/ext2_fs.h>
#define _POSIX_C_SOURCE 200809L

typedef struct {
    int fd;                         // File descriptor for device
    char *device_path;              // Path to the device
    struct ext2_super_block sb;     // Superblock data
    uint32_t block_size;            // Block size in bytes
    uint32_t inodes_per_group;      // Number of inodes per group
    uint32_t blocks_per_group;      // Number of blocks per group
    uint32_t groups_count;          // Number of block groups
    struct ext2_group_desc *group_desc; // Group descriptors
    bool is_ext4;                   // Whether filesystem is ext4
} fs_info_t;

fs_info_t *analyzer_init(const char *device_path);

void analyzer_cleanup(fs_info_t *fs_info);

int read_block(fs_info_t *fs_info, uint32_t block_num, void *buffer);

int write_block(fs_info_t *fs_info, uint32_t block_num, void *buffer);

bool is_block_allocated(fs_info_t *fs_info, uint32_t block_num);

bool is_inode_allocated(fs_info_t *fs_info, uint32_t inode_num);

int read_inode(fs_info_t *fs_info, uint32_t inode_num, struct ext2_inode *inode);

int write_inode(fs_info_t *fs_info, uint32_t inode_num, struct ext2_inode *inode);

int get_block_bitmap(fs_info_t *fs_info, uint32_t group_num, unsigned char *bitmap);

int get_inode_bitmap(fs_info_t *fs_info, uint32_t group_num, unsigned char *bitmap);

void analyze_filesystem(fs_info_t *fs_info);

#endif /* ANALYZER_H */
