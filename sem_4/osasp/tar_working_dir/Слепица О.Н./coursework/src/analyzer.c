#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ext2fs/ext2_fs.h>
#include "analyzer.h"
#include "utils.h"

#define _POSIX_C_SOURCE 200809L

fs_info_t *analyzer_init(const char *device_path) {
    fs_info_t *fs_info = NULL;
    int fd = -1;
    struct ext2_super_block sb;

    fs_info = (fs_info_t *)malloc(sizeof(fs_info_t));
    if (!fs_info) {
        perror("Failed to allocate memory for fs_info");
        return NULL;
    }
    memset(fs_info, 0, sizeof(fs_info_t));

    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        fd = open(device_path, O_RDONLY);
        if (fd < 0) {
            perror("Failed to open device");
            free(fs_info);
            return NULL;
        }
        printf("Warning: Device opened in read-only mode\n");
    }

    fs_info->fd = fd;
    fs_info->device_path = strdup(device_path);
    if (!fs_info->device_path) {
        perror("Failed to allocate memory for device path");
        close(fd);
        free(fs_info);
        return NULL;
    }

    if (pread(fd, &sb, sizeof(struct ext2_super_block), 1024) != sizeof(struct ext2_super_block)) {
        perror("Failed to read superblock");
        close(fd);
        free(fs_info->device_path);
        free(fs_info);
        return NULL;
    }


    if (sb.s_magic != EXT2_SUPER_MAGIC) {
        fprintf(stderr, "Not an ext2/ext3/ext4 filesystem (magic: %x, expected: %x)\n", 
                sb.s_magic, EXT2_SUPER_MAGIC);
        close(fd);
        free(fs_info->device_path);
        free(fs_info);
        return NULL;
    }

    memcpy(&fs_info->sb, &sb, sizeof(struct ext2_super_block));

    fs_info->block_size = 1024 << sb.s_log_block_size;
    fs_info->inodes_per_group = sb.s_inodes_per_group;
    fs_info->blocks_per_group = sb.s_blocks_per_group;
    fs_info->groups_count = (sb.s_blocks_count + sb.s_blocks_per_group - 1) / sb.s_blocks_per_group;

    fs_info->is_ext4 = (sb.s_feature_incompat & EXT4_FEATURE_INCOMPAT_64BIT) != 0;

    size_t gd_size = fs_info->is_ext4 ? sizeof(struct ext2_group_desc) : 
                                     sizeof(struct ext2_group_desc);
    size_t gd_table_size = gd_size * fs_info->groups_count;
    
    fs_info->group_desc = (struct ext2_group_desc *)malloc(gd_table_size);
    if (!fs_info->group_desc) {
        perror("Failed to allocate memory for group descriptors");
        close(fd);
        free(fs_info->device_path);
        free(fs_info);
        return NULL;
    }

    uint32_t gdt_block = (1024 / fs_info->block_size) + 1;
    
    if (pread(fd, fs_info->group_desc, gd_table_size, gdt_block * fs_info->block_size) != (ssize_t)gd_table_size) {
        perror("Failed to read group descriptors");
        free(fs_info->group_desc);
        close(fd);
        free(fs_info->device_path);
        free(fs_info);
        return NULL;
    }

    return fs_info;
}

void analyzer_cleanup(fs_info_t *fs_info) {
    if (!fs_info) {
        return;
    }

    if (fs_info->fd >= 0) {
        close(fs_info->fd);
        fs_info->fd = -1;
    }

    if (fs_info->device_path) {
        free(fs_info->device_path);
        fs_info->device_path = NULL;
    }

    if (fs_info->group_desc) {
        free(fs_info->group_desc);
        fs_info->group_desc = NULL;
    }

    memset(fs_info, 0, sizeof(fs_info_t));
    
    free(fs_info);
}

int read_block(fs_info_t *fs_info, uint32_t block_num, void *buffer) {
    if (!fs_info || !buffer || block_num <= 0 || 
        block_num >= fs_info->sb.s_blocks_count) {
        return -1;
    }

    off_t offset = (off_t)block_num * fs_info->block_size;
    ssize_t bytes_read = pread(fs_info->fd, buffer, fs_info->block_size, offset);
    
    if (bytes_read != fs_info->block_size) {
        perror("Failed to read block");
        return -1;
    }

    return 0;
}

int write_block(fs_info_t *fs_info, uint32_t block_num, void *buffer) {
    if (!fs_info || !buffer || block_num <= 0 || 
        block_num >= fs_info->sb.s_blocks_count) {
        return -1;
    }

    off_t offset = (off_t)block_num * fs_info->block_size;
    ssize_t bytes_written = pwrite(fs_info->fd, buffer, fs_info->block_size, offset);
    
    if (bytes_written != fs_info->block_size) {
        perror("Failed to write block");
        return -1;
    }

    return 0;
}

bool is_block_allocated(fs_info_t *fs_info, uint32_t block_num) {
    if (!fs_info || block_num <= 0 || block_num >= fs_info->sb.s_blocks_count) {
        return false;
    }

    uint32_t group = (block_num - 1) / fs_info->blocks_per_group;
    
    uint32_t bitmap_block = fs_info->group_desc[group].bg_block_bitmap;
    
    uint32_t position = (block_num - 1) % fs_info->blocks_per_group;
    
    unsigned char *bitmap = (unsigned char *)malloc(fs_info->block_size);
    if (!bitmap) {
        perror("Failed to allocate memory for bitmap");
        return false;
    }
    
    if (read_block(fs_info, bitmap_block, bitmap) != 0) {
        free(bitmap);
        return false;
    }
    
    bool allocated = check_bitmap_bit(bitmap, position);
    
    free(bitmap);
    return allocated;
}

bool is_inode_allocated(fs_info_t *fs_info, uint32_t inode_num) {
    if (!fs_info || inode_num <= 0 || inode_num >= fs_info->sb.s_inodes_count) {
        return false;
    }

    uint32_t group = (inode_num - 1) / fs_info->inodes_per_group;
    
    uint32_t bitmap_block = fs_info->group_desc[group].bg_inode_bitmap;
    
    uint32_t position = (inode_num - 1) % fs_info->inodes_per_group;
    
    unsigned char *bitmap = (unsigned char *)malloc(fs_info->block_size);
    if (!bitmap) {
        perror("Failed to allocate memory for bitmap");
        return false;
    }
    
    if (read_block(fs_info, bitmap_block, bitmap) != 0) {
        free(bitmap);
        return false;
    }
    
    bool allocated = check_bitmap_bit(bitmap, position);
    
    free(bitmap);
    return allocated;
}

int read_inode(fs_info_t *fs_info, uint32_t inode_num, struct ext2_inode *inode) {
    if (!fs_info || !inode || inode_num <= 0 || 
        inode_num > fs_info->sb.s_inodes_count) {
        return -1;
    }

    uint32_t group = (inode_num - 1) / fs_info->inodes_per_group;
    
    uint32_t inode_table_block = fs_info->group_desc[group].bg_inode_table;
    
    uint32_t index = (inode_num - 1) % fs_info->inodes_per_group;

    off_t offset = (off_t)inode_table_block * fs_info->block_size + 
                   (off_t)index * fs_info->sb.s_inode_size;
    
    ssize_t bytes_read = pread(fs_info->fd, inode, sizeof(struct ext2_inode), offset);
    
    if (bytes_read != sizeof(struct ext2_inode)) {
        perror("Failed to read inode");
        return -1;
    }

    return 0;
}

int write_inode(fs_info_t *fs_info, uint32_t inode_num, struct ext2_inode *inode) {
    if (!fs_info || !inode || inode_num <= 0 || 
        inode_num > fs_info->sb.s_inodes_count) {
        return -1;
    }

    uint32_t group = (inode_num - 1) / fs_info->inodes_per_group;
    
    uint32_t inode_table_block = fs_info->group_desc[group].bg_inode_table;
    
    uint32_t index = (inode_num - 1) % fs_info->inodes_per_group;
    
    off_t offset = (off_t)inode_table_block * fs_info->block_size + 
                   (off_t)index * fs_info->sb.s_inode_size;
    
    ssize_t bytes_written = pwrite(fs_info->fd, inode, sizeof(struct ext2_inode), offset);
    
    if (bytes_written != sizeof(struct ext2_inode)) {
        perror("Failed to write inode");
        return -1;
    }

    return 0;
}

int get_block_bitmap(fs_info_t *fs_info, uint32_t group_num, unsigned char *bitmap) {
    if (!fs_info || !bitmap || group_num >= fs_info->groups_count) {
        return -1;
    }

    uint32_t bitmap_block = fs_info->group_desc[group_num].bg_block_bitmap;
    
    if (read_block(fs_info, bitmap_block, bitmap) != 0) {
        return -1;
    }

    return 0;
}

int get_inode_bitmap(fs_info_t *fs_info, uint32_t group_num, unsigned char *bitmap) {
    if (!fs_info || !bitmap || group_num >= fs_info->groups_count) {
        return -1;
    }

    uint32_t bitmap_block = fs_info->group_desc[group_num].bg_inode_bitmap;
    
    if (read_block(fs_info, bitmap_block, bitmap) != 0) {
        return -1;
    }

    return 0;
}

void analyze_filesystem(fs_info_t *fs_info) {
    if (!fs_info) {
        return;
    }

    char buffer[256];
    char fs_type[32];
    get_fs_type_string(fs_info, fs_type, sizeof(fs_type));
    
    printf("Filesystem Analysis for %s\n", fs_info->device_path);
    printf("==============================================\n");
    
    printf("Filesystem type: %s\n", fs_type);
    
    format_value(fs_info->block_size, buffer, sizeof(buffer), true);
    printf("Block size: %s\n", buffer);
    
    printf("Total blocks: %u\n", fs_info->sb.s_blocks_count);
    printf("Free blocks: %u\n", fs_info->sb.s_free_blocks_count);
    
    printf("Total inodes: %u\n", fs_info->sb.s_inodes_count);
    printf("Free inodes: %u\n", fs_info->sb.s_free_inodes_count);
    
    uint64_t total_size = (uint64_t)fs_info->sb.s_blocks_count * fs_info->block_size;
    format_value(total_size, buffer, sizeof(buffer), true);
    printf("Total filesystem size: %s\n", buffer);
    
    uint64_t free_size = (uint64_t)fs_info->sb.s_free_blocks_count * fs_info->block_size;
    format_value(free_size, buffer, sizeof(buffer), true);
    printf("Free space: %s\n", buffer);
    
    float used_percent = 100.0f * (1.0f - (float)fs_info->sb.s_free_blocks_count / fs_info->sb.s_blocks_count);
    printf("Used space: %.1f%%\n", used_percent);
    
    printf("\nBlock Groups: %u\n", fs_info->groups_count);
    printf("Blocks per group: %u\n", fs_info->blocks_per_group);
    printf("Inodes per group: %u\n", fs_info->inodes_per_group);
    
    printf("\nSuperblock Features:\n");
    if (fs_info->sb.s_feature_compat & EXT2_FEATURE_COMPAT_DIR_INDEX)
        printf("- Directory indexing supported\n");
    if (fs_info->sb.s_feature_incompat & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)
        printf("- Journal device\n");
    if (fs_info->sb.s_feature_incompat & EXT3_FEATURE_INCOMPAT_RECOVER)
        printf("- Needs recovery\n");
    if (fs_info->sb.s_feature_incompat & EXT4_FEATURE_INCOMPAT_64BIT)
        printf("- 64-bit support\n");
    if (fs_info->sb.s_feature_incompat & EXT4_FEATURE_INCOMPAT_FLEX_BG)
        printf("- Flexible block groups\n");
    
    printf("\nBlock Group Descriptor Table:\n");
    printf("%-5s %-15s %-15s %-15s %-15s\n", 
           "Group", "Block Bitmap", "Inode Bitmap", "Inode Table", "Free Blocks");
    
    for (uint32_t i = 0; i < fs_info->groups_count; i++) {
        printf("%-5u %-15u %-15u %-15u %-15u\n", 
               i,
               fs_info->group_desc[i].bg_block_bitmap,
               fs_info->group_desc[i].bg_inode_bitmap,
               fs_info->group_desc[i].bg_inode_table,
               fs_info->group_desc[i].bg_free_blocks_count);
    }
}
