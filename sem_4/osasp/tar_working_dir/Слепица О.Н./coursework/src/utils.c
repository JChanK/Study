#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "utils.h"
#define S_ISVTX 01000 

void format_value(uint64_t value, char *buffer, size_t buffer_size, bool is_size) {
    if (!buffer || buffer_size <= 0) {
        return;
    }
    
    if (!is_size) {
        snprintf(buffer, buffer_size, "%lu", (unsigned long)value);
        return;
    }
    
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = (double)value;
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    if (unit_index == 0) {
        snprintf(buffer, buffer_size, "%lu %s", (unsigned long)value, units[unit_index]);
    } else {
        snprintf(buffer, buffer_size, "%.2f %s", size, units[unit_index]);
    }
}

bool check_bitmap_bit(const unsigned char *bitmap, uint32_t bit_num) {
    uint32_t byte_index = bit_num / 8;
    uint32_t bit_offset = bit_num % 8;
    
    return (bitmap[byte_index] & (1 << bit_offset)) != 0;
}

void set_bitmap_bit(unsigned char *bitmap, uint32_t bit_num) {
    uint32_t byte_index = bit_num / 8;
    uint32_t bit_offset = bit_num % 8;
    
    bitmap[byte_index] |= (1 << bit_offset);
}

void clear_bitmap_bit(unsigned char *bitmap, uint32_t bit_num) {
    uint32_t byte_index = bit_num / 8;
    uint32_t bit_offset = bit_num % 8;
    
    bitmap[byte_index] &= ~(1 << bit_offset);
}

void superblock_to_string(const struct ext2_super_block *sb, char *buffer, size_t buffer_size) {
    if (!sb || !buffer || buffer_size <= 0) {
        return;
    }
    
    char temp[1024];
    snprintf(temp, sizeof(temp),
             "Superblock:\n"
             "  Inodes count: %u\n"
             "  Blocks count: %u\n"
             "  Reserved blocks count: %u\n"
             "  Free blocks count: %u\n"
             "  Free inodes count: %u\n"
             "  First data block: %u\n"
             "  Block size: %u\n"
             "  Blocks per group: %u\n"
             "  Inodes per group: %u\n"
             "  Mount count: %u\n"
             "  Maximum mount count: %u\n"
             "  Magic signature: 0x%x\n"
             "  Filesystem state: %u\n"
             "  Error behavior: %u\n"
             "  Minor revision level: %u\n"
             "  Last check time: %u\n"
             "  Check interval: %u\n"
             "  Creator OS: %u\n"
             "  Revision level: %u\n"
             "  Reserved blocks UID: %u\n"
             "  Reserved blocks GID: %u\n",
             sb->s_inodes_count,
             sb->s_blocks_count,
             sb->s_r_blocks_count,
             sb->s_free_blocks_count,
             sb->s_free_inodes_count,
             sb->s_first_data_block,
             1024 << sb->s_log_block_size,
             sb->s_blocks_per_group,
             sb->s_inodes_per_group,
             sb->s_mnt_count,
             sb->s_max_mnt_count,
             sb->s_magic,
             sb->s_state,
             sb->s_errors,
             sb->s_minor_rev_level,
             sb->s_lastcheck,
             sb->s_checkinterval,
             sb->s_creator_os,
             sb->s_rev_level,
             sb->s_def_resuid,
             sb->s_def_resgid);
    
    strncpy(buffer, temp, buffer_size);
    buffer[buffer_size - 1] = '\0';
}

void group_desc_to_string(const struct ext2_group_desc *gd, char *buffer, size_t buffer_size) {
    if (!gd || !buffer || buffer_size <= 0) {
        return;
    }
    
    char temp[512];
    snprintf(temp, sizeof(temp),
             "Group Descriptor:\n"
             "  Block bitmap: %u\n"
             "  Inode bitmap: %u\n"
             "  Inode table: %u\n"
             "  Free blocks count: %u\n"
             "  Free inodes count: %u\n"
             "  Used directories count: %u\n",
             gd->bg_block_bitmap,
             gd->bg_inode_bitmap,
             gd->bg_inode_table,
             gd->bg_free_blocks_count,
             gd->bg_free_inodes_count,
             gd->bg_used_dirs_count);
    
    strncpy(buffer, temp, buffer_size);
    buffer[buffer_size - 1] = '\0';
}

void inode_to_string(const struct ext2_inode *inode, char *buffer, size_t buffer_size) {
    if (!inode || !buffer || buffer_size <= 0) {
        return;
    }
    
    char temp[1024];
    char mode_str[11] = "----------";
    
    if (S_ISREG(inode->i_mode)) mode_str[0] = '-';
    else if (S_ISDIR(inode->i_mode)) mode_str[0] = 'd';
    else if (S_ISLNK(inode->i_mode)) mode_str[0] = 'l';
    else if (S_ISCHR(inode->i_mode)) mode_str[0] = 'c';
    else if (S_ISBLK(inode->i_mode)) mode_str[0] = 'b';
    else if (S_ISFIFO(inode->i_mode)) mode_str[0] = 'p';
    else if (S_ISSOCK(inode->i_mode)) mode_str[0] = 's';
    
    if (inode->i_mode & S_IRUSR) mode_str[1] = 'r';
    if (inode->i_mode & S_IWUSR) mode_str[2] = 'w';
    if (inode->i_mode & S_IXUSR) mode_str[3] = 'x';
    if (inode->i_mode & S_IRGRP) mode_str[4] = 'r';
    if (inode->i_mode & S_IWGRP) mode_str[5] = 'w';
    if (inode->i_mode & S_IXGRP) mode_str[6] = 'x';
    if (inode->i_mode & S_IROTH) mode_str[7] = 'r';
    if (inode->i_mode & S_IWOTH) mode_str[8] = 'w';
    if (inode->i_mode & S_IXOTH) mode_str[9] = 'x';
    
    if (inode->i_mode & S_ISUID) {
        if (mode_str[3] == 'x') mode_str[3] = 's';
        else mode_str[3] = 'S';
    }
    if (inode->i_mode & S_ISGID) {
        if (mode_str[6] == 'x') mode_str[6] = 's';
        else mode_str[6] = 'S';
    }
    if (inode->i_mode & S_ISVTX) {
        if (mode_str[9] == 'x') mode_str[9] = 't';
        else mode_str[9] = 'T';
    }
    
    snprintf(temp, sizeof(temp),
             "Inode:\n"
             "  Mode: %s (0%o)\n"
             "  Owner: %u\n"
             "  Size: %u\n"
             "  Access time: %u\n"
             "  Creation time: %u\n"
             "  Modification time: %u\n"
             "  Deletion time: %u\n"
             "  Links count: %u\n"
             "  Blocks count: %u\n"
             "  Flags: 0x%x\n"
             "  Direct blocks:\n"
             "    [0]: %u\n"
             "    [1]: %u\n"
             "    [2]: %u\n"
             "    [3]: %u\n"
             "    [4]: %u\n"
             "    [5]: %u\n"
             "    [6]: %u\n"
             "    [7]: %u\n"
             "    [8]: %u\n"
             "    [9]: %u\n"
             "    [10]: %u\n"
             "    [11]: %u\n"
             "  Singly-indirect block: %u\n"
             "  Doubly-indirect block: %u\n"
             "  Triply-indirect block: %u\n",
             mode_str, inode->i_mode & 0xFFF,
             inode->i_uid,
             inode->i_size,
             inode->i_atime,
             inode->i_ctime,
             inode->i_mtime,
             inode->i_dtime,
             inode->i_links_count,
             inode->i_blocks,
             inode->i_flags,
             inode->i_block[0],
             inode->i_block[1],
             inode->i_block[2],
             inode->i_block[3],
             inode->i_block[4],
             inode->i_block[5],
             inode->i_block[6],
             inode->i_block[7],
             inode->i_block[8],
             inode->i_block[9],
             inode->i_block[10],
             inode->i_block[11],
             inode->i_block[12],
             inode->i_block[13],
             inode->i_block[14]);
    
    strncpy(buffer, temp, buffer_size);
    buffer[buffer_size - 1] = '\0';
}

void get_fs_type_string(const fs_info_t *fs_info, char *buffer, size_t buffer_size) {
    if (!fs_info || !buffer || buffer_size <= 0) {
        return;
    }
    
    if (fs_info->is_ext4) {
        strncpy(buffer, "ext4", buffer_size);
    } else if (fs_info->sb.s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL) {
        strncpy(buffer, "ext3", buffer_size);
    } else {
        strncpy(buffer, "ext2", buffer_size);
    }
    
    buffer[buffer_size - 1] = '\0';
}
