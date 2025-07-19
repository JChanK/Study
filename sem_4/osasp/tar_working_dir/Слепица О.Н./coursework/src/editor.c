#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <ext2fs/ext2_fs.h>
#include <ctype.h>
#include "analyzer.h"
#include "editor.h"
#include "utils.h"

editor_context_t *editor_init(fs_info_t *fs_info) {
    editor_context_t *ctx = (editor_context_t *)calloc(1, sizeof(editor_context_t));
    if (!ctx) {
        return NULL;
    }

    ctx->fs_info = fs_info;
    ctx->current_offset = 0;
    ctx->buffer_size = fs_info->block_size * 2;
    ctx->buffer = (uint8_t *)malloc(ctx->buffer_size);
    if (!ctx->buffer) {
        free(ctx);
        return NULL;
    }

    ctx->bytes_per_row = 16;
    ctx->view_rows = 16;
    ctx->cursor_x = 0;
    ctx->cursor_y = 0;
    ctx->editing_mode = false;

    return ctx;
}

void editor_cleanup(editor_context_t *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->buffer) {
        free(ctx->buffer);
        ctx->buffer = NULL;
    }

    memset(ctx, 0, sizeof(editor_context_t));
    
    free(ctx);
}

int editor_open_structure(editor_context_t *ctx, structure_type_t type, uint32_t id) {
    if (!ctx) {
        return -1;
    }

    ctx->edited_structure = type;
    ctx->edited_id = id;

    ctx->cursor_x = 0;
    ctx->cursor_y = 0;
    
    memset(ctx->buffer, 0, ctx->buffer_size);
    
    switch (type) {
        case STRUCTURE_SUPERBLOCK: {
            ctx->current_offset = 1024;
            memcpy(ctx->buffer, &ctx->fs_info->sb, sizeof(struct ext2_super_block));
            break;
        }
        
        case STRUCTURE_GROUP_DESC: {
            if (id >= ctx->fs_info->groups_count) {
                return -1;
            }
            ctx->current_offset = ctx->fs_info->block_size + id * sizeof(struct ext2_group_desc);
            memcpy(ctx->buffer, &ctx->fs_info->group_desc[id], sizeof(struct ext2_group_desc));
            break;
        }
        
        case STRUCTURE_INODE: {
            struct ext2_inode inode;
            if (read_inode(ctx->fs_info, id, &inode) != 0) {
                return -1;
            }
            memcpy(ctx->buffer, &inode, sizeof(struct ext2_inode));
            break;
        }
        
        case STRUCTURE_BLOCK: {
            if (read_block(ctx->fs_info, id, ctx->buffer) != 0) {
                return -1;
            }
            break;
        }
        
        case STRUCTURE_BLOCK_BITMAP: {
            if (id >= ctx->fs_info->groups_count) {
                return -1;
            }
            unsigned char *bitmap = (unsigned char *)malloc(ctx->fs_info->blocks_per_group / 8);
            if (!bitmap) {
                return -1;
            }
            
            if (get_block_bitmap(ctx->fs_info, id, bitmap) != 0) {
                free(bitmap);
                return -1;
            }
            
            memcpy(ctx->buffer, bitmap, ctx->fs_info->blocks_per_group / 8);
            free(bitmap);
            break;
        }
        
        case STRUCTURE_INODE_BITMAP: {
            if (id >= ctx->fs_info->groups_count) {
                return -1;
            }
            unsigned char *bitmap = (unsigned char *)malloc(ctx->fs_info->inodes_per_group / 8);
            if (!bitmap) {
                return -1;
            }
            
            if (get_inode_bitmap(ctx->fs_info, id, bitmap) != 0) {
                free(bitmap);
                return -1;
            }
            
            memcpy(ctx->buffer, bitmap, ctx->fs_info->inodes_per_group / 8);
            free(bitmap);
            break;
        }
        
        default:
            return -1;
    }
    
    return 0;
}

int editor_save_changes(editor_context_t *ctx) {
    if (!ctx) return -1;

    switch (ctx->edited_structure) {
        case STRUCTURE_SUPERBLOCK:
            if (ctx->buffer_size >= sizeof(struct ext2_super_block)) {
                memcpy(&ctx->fs_info->sb, ctx->buffer, sizeof(struct ext2_super_block));
                lseek(ctx->fs_info->fd, 1024, SEEK_SET);
                write(ctx->fs_info->fd, &ctx->fs_info->sb, sizeof(struct ext2_super_block));
                return 0;
            }
            break;

        case STRUCTURE_INODE: {
            struct ext2_inode *inode = (struct ext2_inode *)ctx->buffer;
            return write_inode(ctx->fs_info, ctx->edited_id, inode);
        }

        case STRUCTURE_BLOCK:
            return write_block(ctx->fs_info, ctx->edited_id, ctx->buffer);

        default:
            break;
    }

    return -1;
}

void editor_move_cursor(editor_context_t *ctx, int dx, int dy) {
    if (!ctx) {
        return;
    }
    
    int new_x = ctx->cursor_x + dx;
    int new_y = ctx->cursor_y + dy;
    
    if (new_x < 0) {
        new_x = 0;
    } else if ((uint32_t)new_x >= ctx->bytes_per_row) {
        new_x = ctx->bytes_per_row - 1;
    }
    
    if (new_y < 0) {
        new_y = 0;
    } else if ((uint32_t)new_y >= ctx->view_rows) {
        new_y = ctx->view_rows - 1;
    }
    
    size_t new_pos = new_y * ctx->bytes_per_row + new_x;
    if (new_pos >= ctx->buffer_size) {
        return;
    }
    
    ctx->cursor_x = new_x;
    ctx->cursor_y = new_y;
}

void editor_set_byte(editor_context_t *ctx, uint8_t value) {
    if (!ctx || !ctx->editing_mode) {
        return;
    }
    
    size_t index = ctx->cursor_y * ctx->bytes_per_row + ctx->cursor_x;
    
    if (index < ctx->buffer_size) {
        ctx->buffer[index] = value;
    }
}

void editor_render(editor_context_t *ctx) {
    if (!ctx) {
        return;
    }

    clear();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int hex_start = 10; 
    int ascii_start = hex_start + (16 * 3); 
    int help_start = ascii_start + 16 + 4; 

    attron(COLOR_PAIR(1));
    char header[128];
    snprintf(header, sizeof(header), "Binary Editor - Offset: 0x%08lx", ctx->current_offset);
    move(0, 0);
    addstr(header);
    attroff(COLOR_PAIR(1));

    for (uint32_t y = 0; y < ctx->view_rows; y++) {
        char addr_str[16];
        snprintf(addr_str, sizeof(addr_str), "%08lx: ", ctx->current_offset + y * ctx->bytes_per_row);
        move(y + 2, 0);
        addstr(addr_str);

        // Hex
        for (uint32_t x = 0; x < ctx->bytes_per_row; x++) {
            size_t index = y * ctx->bytes_per_row + x;
            
            if (index >= ctx->buffer_size) {
                move(y + 2, hex_start + x * 3);
                addstr("   ");
            } else {
                char hex_str[4];
                snprintf(hex_str, sizeof(hex_str), "%02x ", ctx->buffer[index]);
                
                if (x == ctx->cursor_x && y == ctx->cursor_y) {
                    attron(COLOR_PAIR(5));
                    move(y + 2, hex_start + x * 3);
                    addstr(hex_str);
                    attroff(COLOR_PAIR(5));
                } else {
                    move(y + 2, hex_start + x * 3);
                    addstr(hex_str);
                }
            }
        }

        // ASCII
        move(y + 2, ascii_start);
        addstr("| ");
        
        for (uint32_t x = 0; x < ctx->bytes_per_row; x++) {
            size_t index = y * ctx->bytes_per_row + x;
            
            if (index < ctx->buffer_size) {
                char c = ctx->buffer[index];
                char ascii_char = isprint(c) ? c : '.';
                
                if (x == ctx->cursor_x && y == ctx->cursor_y) {
                    attron(COLOR_PAIR(5));
                    addch(ascii_char);
                    attroff(COLOR_PAIR(5));
                } else {
                    addch(ascii_char);
                }
            } else {
                addch(' ');
            }
        }
    }

    if (ctx->editing_mode && help_start + 20 < max_x) {
        for (int y = 1; y < max_y - 2; y++) {
            move(y, help_start - 2);
            addch(ACS_VLINE);
        }

        attron(COLOR_PAIR(3) | A_BOLD);
        move(1, help_start);
        addstr("EDIT MODE HELP");
        attroff(COLOR_PAIR(3) | A_BOLD);

        int help_line = 3;
        
        move(help_line++, help_start);
        addstr("Navigation:");
        move(help_line++, help_start + 2);
        addstr("Arrows - Move cursor");
        move(help_line++, help_start + 2);
        addstr("PgUp/Dn - Scroll");
        help_line++;

        move(help_line++, help_start);
        addstr("Editing:");
        move(help_line++, help_start + 2);
        addstr("0-9,A-F - Hex input");
        move(help_line++, help_start + 2);
        addstr("TAB - Toggle edit");
        move(help_line++, help_start + 2);
        addstr("ESC - Cancel");
        help_line++;

        move(help_line++, help_start);
        addstr("Actions:");
        move(help_line++, help_start + 2);
        addstr("S - Save changes");
        help_line++;

        attron(COLOR_PAIR(4));
        move(help_line++, help_start);
        addstr("Warning:");
        move(help_line++, help_start + 2);
        addstr("Changes written");
        move(help_line++, help_start + 2);
        addstr("directly to disk!");
        attroff(COLOR_PAIR(4));
    }

    attron(COLOR_PAIR(2));
    move(max_y - 2, 0);
    const char *mode_str = ctx->editing_mode ? "EDIT MODE" : "VIEW MODE";
    addstr(mode_str);

    for (int i = strlen(mode_str); i < 80 && i < max_x; i++) {
        addch(' ');
    }
    attroff(COLOR_PAIR(2));

    refresh();
}

bool editor_handle_key(editor_context_t *ctx, int key) {
    if (!ctx) {
        return false;
    }
    
    switch (key) {
        case KEY_UP:
            editor_move_cursor(ctx, 0, -1);
            break;
            
        case KEY_DOWN:
            editor_move_cursor(ctx, 0, 1);
            break;
            
        case KEY_LEFT:
            editor_move_cursor(ctx, -1, 0);
            break;
            
        case KEY_RIGHT:
            editor_move_cursor(ctx, 1, 0);
            break;
            
        case '\t':
            ctx->editing_mode = !ctx->editing_mode;
            break;
            
        case 's':
        case 'S':
            if (editor_save_changes(ctx) == 0) {
                move(ctx->view_rows + 4, 0);
                addstr("Changes saved successfully.");
                clrtoeol();
            } else {
                move(ctx->view_rows + 4, 0);
                addstr("Error saving changes!");
                clrtoeol();
            }
            refresh();
            break;
            
        case 'q':
        case 'Q':
            return false;
            
        default:
            if (ctx->editing_mode) {
                int value = -1;
                
                if (key >= '0' && key <= '9') {
                    value = key - '0';
                } else if (key >= 'a' && key <= 'f') {
                    value = key - 'a' + 10;
                } else if (key >= 'A' && key <= 'F') {
                    value = key - 'A' + 10;
                }
                
                if (value >= 0) {
                    size_t index = ctx->cursor_y * ctx->bytes_per_row + ctx->cursor_x;
                    
                    if (index < ctx->buffer_size) {
                        uint8_t current = ctx->buffer[index];
                        ctx->buffer[index] = (current & 0x0F) | (value << 4);
                        
                        editor_move_cursor(ctx, 1, 0);
                    }
                }
            }
            break;
    }
    
    return true;
}
