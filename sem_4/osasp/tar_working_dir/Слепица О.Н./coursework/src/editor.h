#ifndef EDITOR_H
#define EDITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h> 
#include "analyzer.h"

#define _POSIX_C_SOURCE 200809L

typedef enum {
    STRUCTURE_SUPERBLOCK,       // Superblock
    STRUCTURE_GROUP_DESC,       // Group descriptor
    STRUCTURE_INODE,            // Inode
    STRUCTURE_BLOCK,            // Data block
    STRUCTURE_BLOCK_BITMAP,     // Block bitmap
    STRUCTURE_INODE_BITMAP      // Inode bitmap
} structure_type_t;

typedef struct {
    fs_info_t *fs_info;         // Filesystem information
    uint64_t current_offset;    // Current offset in the file/device
    uint8_t *buffer;            // Buffer for the current view
    size_t buffer_size;         // Size of the buffer
    uint32_t cursor_x;          // Cursor X position (column)
    uint32_t cursor_y;          // Cursor Y position (row)
    uint32_t bytes_per_row;     // Number of bytes displayed per row
    uint32_t view_rows;         // Number of rows in the view
    bool editing_mode;          // Whether in editing mode
    bool field_highlight;       // Whether to highlight structure fields
    structure_type_t current_structure; // Current structure being edited
    uint32_t current_id;        // ID of current structure (block/inode number)
    bool should_exit;           // Flag to indicate if editor should exit

    structure_type_t edited_structure;
    uint32_t edited_id;
} editor_context_t;

editor_context_t *editor_init(fs_info_t *fs_info);
void editor_cleanup(editor_context_t *ctx);
int editor_open_structure(editor_context_t *ctx, structure_type_t type, uint32_t id);
int editor_save_changes(editor_context_t *ctx);
void editor_move_cursor(editor_context_t *ctx, int dx, int dy);
void editor_set_byte(editor_context_t *ctx, uint8_t value);
void editor_render(editor_context_t *ctx);
bool editor_handle_key(editor_context_t *ctx, int key);

#endif /* EDITOR_H */
