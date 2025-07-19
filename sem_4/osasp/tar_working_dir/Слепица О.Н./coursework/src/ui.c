#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <ncurses.h>
#include "ui.h"
#include "utils.h"
#include "editor.h"

static bool ui_handle_binary_editor_input(ui_context_t *ui_ctx, int key);
static void ui_display_menu(ui_context_t *ui_ctx);
static bool ui_handle_menu_input(ui_context_t *ui_ctx, int key);
static bool ui_handle_analyzer_input(ui_context_t *ui_ctx, int key);
static bool ui_handle_block_browser_input(ui_context_t *ui_ctx, int key);
static bool ui_handle_inode_browser_input(ui_context_t *ui_ctx, int key);

static void safe_delwin(WINDOW **win) {
    if (win && *win) {
        delwin(*win);
        *win = NULL;
    }
}

ui_context_t *ui_init(fs_info_t *fs_info) {
    ui_context_t *ui_ctx = (ui_context_t *)malloc(sizeof(ui_context_t));
    if (!ui_ctx) {
        return NULL;
    }

    ui_ctx->fs_info = fs_info;
    ui_ctx->current_mode = UI_MODE_MENU;
    ui_ctx->current_block = 0;
    ui_ctx->current_inode = 1;
    ui_ctx->current_group = 0;

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    ui_ctx->main_win = newwin(max_y - 2, max_x, 0, 0);
    ui_ctx->status_win = newwin(1, max_x, max_y - 2, 0);
    ui_ctx->help_win = newwin(1, max_x, max_y - 1, 0);

    wbkgd(ui_ctx->status_win, COLOR_PAIR(2));
    wbkgd(ui_ctx->help_win, COLOR_PAIR(1));

    ui_ctx->editor_ctx = editor_init(fs_info);
    if (!ui_ctx->editor_ctx) {
        ui_cleanup(ui_ctx);
        return NULL;
    }

    return ui_ctx;
}

void ui_cleanup(ui_context_t *ui_ctx) {
    if (!ui_ctx) {
        return;
    }

    if (ui_ctx->status_win) {
        werase(ui_ctx->status_win);
        wrefresh(ui_ctx->status_win);
    }

    if (ui_ctx->editor_ctx) {
        editor_cleanup(ui_ctx->editor_ctx);
        ui_ctx->editor_ctx = NULL;
    }

    if (ui_ctx->main_win) {
        werase(ui_ctx->main_win);
        wrefresh(ui_ctx->main_win);
    }
    if (ui_ctx->status_win) {
        werase(ui_ctx->status_win);
        wrefresh(ui_ctx->status_win);
    }
    if (ui_ctx->help_win) {
        werase(ui_ctx->help_win);
        wrefresh(ui_ctx->help_win);
    }

    refresh();

    safe_delwin(&ui_ctx->help_win);
    safe_delwin(&ui_ctx->status_win);
    safe_delwin(&ui_ctx->main_win);

    memset(ui_ctx, 0, sizeof(ui_context_t));

    free(ui_ctx);
}

static void ui_display_status(ui_context_t *ui_ctx, const char *message, ...) {
    if (!ui_ctx || !ui_ctx->status_win || !message) {
        return;
    }

    werase(ui_ctx->status_win);
    wbkgd(ui_ctx->status_win, COLOR_PAIR(2));

    char buffer[256];
    va_list args;
    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);

    // Use wmove + waddstr instead of mvwprintw/vw_printw
    wmove(ui_ctx->status_win, 0, 0);
    waddstr(ui_ctx->status_win, buffer);

    wrefresh(ui_ctx->status_win);
}

static void ui_display_detailed_help(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);

    int y = 0;
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "EXT Filesystem Editor Help");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "=========================");
    y++;

    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "General Navigation:");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  - Arrow keys: Move cursor");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  - ESC: Return to previous menu");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  - Q: Quit the program");
    y++;

    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "Editable Structures:");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  1. Superblock (Block 1)");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  2. Group Descriptors (Block 2)");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  3. Block Bitmaps (Per group)");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  4. Inode Bitmaps (Per group)");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  5. Inode Table (Per group)");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  6. Data Blocks");
    y++;

    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "Editing Instructions:");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  - TAB: Toggle edit mode");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  - In edit mode, type hex digits (0-9, A-F)");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  - Each key press modifies a nibble (4 bits)");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  - Cursor moves automatically after edit");
    y++;

    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "Saving Changes:");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  1. Make your changes in the editor");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  2. Press S to save changes");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  3. Changes are written immediately to disk");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "  - WARNING: Changes can corrupt filesystem!");
    y++;

    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "Press any key to return...");

    wrefresh(ui_ctx->main_win);
    getch();
}

void ui_display_help(ui_context_t *ui_ctx) {
    werase(ui_ctx->help_win);
    wbkgd(ui_ctx->help_win, COLOR_PAIR(1));

    wmove(ui_ctx->help_win, 0, 0);
    
    switch (ui_ctx->current_mode) {
        case UI_MODE_MENU:
            waddstr(ui_ctx->help_win, "F1:Help | 1:Analyzer | 2:Block Browser | 3:Inode Browser | Q:Quit");
            break;
        case UI_MODE_ANALYZER:
            waddstr(ui_ctx->help_win, "F1:Help | ESC:Back | G:Group | Q:Quit");
            break;
        case UI_MODE_BLOCK_BROWSER:
            waddstr(ui_ctx->help_win, "F1:Help | ESC:Back | ARROWS:Navigate | E:Edit Block | G:Go to Block | Q:Quit");
            break;
        case UI_MODE_INODE_BROWSER:
            waddstr(ui_ctx->help_win, "F1:Help | ESC:Back | ARROWS:Navigate | E:Edit Inode | G:Go to Inode | Q:Quit");
            break;
        case UI_MODE_BINARY_EDITOR:
            waddstr(ui_ctx->help_win, "F1:Help | ESC:Back | ARROWS:Move | TAB:Edit Mode | S:Save | Q:Quit");
            break;
    }

    wrefresh(ui_ctx->help_win);
}

void ui_show_error(ui_context_t *ui_ctx, const char *message) {
    ui_display_status(ui_ctx, "%s", message);
    wgetch(ui_ctx->status_win);
}

bool ui_prompt(ui_context_t *ui_ctx, const char *prompt, char *buffer, size_t max_len) {
    if (!ui_ctx || !ui_ctx->status_win || !prompt || !buffer || max_len == 0) {
        return false;
    }

    memset(buffer, 0, max_len);

    ui_display_status(ui_ctx, "%s", prompt);

    echo();
    curs_set(1);

    wmove(ui_ctx->status_win, 0, strlen(prompt));
    wrefresh(ui_ctx->status_win);

    int result = wgetnstr(ui_ctx->status_win, buffer, (int)(max_len - 1));

    noecho();
    curs_set(0);

    werase(ui_ctx->status_win);
    wbkgd(ui_ctx->status_win, COLOR_PAIR(2));
    wrefresh(ui_ctx->status_win);

    if (result == ERR || buffer[0] == '\0') {
        return false;
    }

    buffer[max_len - 1] = '\0';

    return true;
}

void ui_set_mode(ui_context_t *ui_ctx, ui_mode_t mode) {
    ui_ctx->current_mode = mode;
    werase(ui_ctx->main_win);

    switch (mode) {
        case UI_MODE_MENU:
            ui_display_status(ui_ctx, "EXT Filesystem Analyzer - %s", ui_ctx->fs_info->device_path);
            break;
        case UI_MODE_ANALYZER:
            ui_display_status(ui_ctx, "Filesystem Analyzer - %s", ui_ctx->fs_info->device_path);
            break;
        case UI_MODE_BLOCK_BROWSER:
            ui_display_status(ui_ctx, "Block Browser - Block %d", ui_ctx->current_block);
            break;
        case UI_MODE_INODE_BROWSER:
            ui_display_status(ui_ctx, "Inode Browser - Inode %d", ui_ctx->current_inode);
            break;
        case UI_MODE_BINARY_EDITOR:
            ui_display_status(ui_ctx, "Binary Editor");
            break;
    }

    ui_display_help(ui_ctx);
}

static void ui_display_menu(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);

    int max_y, max_x;
    getmaxyx(ui_ctx->main_win, max_y, max_x);
    (void)max_y;

    char fs_type[32];
    get_fs_type_string(ui_ctx->fs_info, fs_type, sizeof(fs_type));

    int y = 2;
    const char *title = "EXT Filesystem Analyzer and Editor";
    wmove(ui_ctx->main_win, y++, (max_x - strlen(title)) / 2);
    waddstr(ui_ctx->main_win, title);

    const char *divider = "=================================";
    wmove(ui_ctx->main_win, y++, (max_x - strlen(divider)) / 2);
    waddstr(ui_ctx->main_win, divider);

    y++;
    char device_info[256];
    snprintf(device_info, sizeof(device_info), "Device: %s", ui_ctx->fs_info->device_path);
    wmove(ui_ctx->main_win, y++, (max_x - strlen(device_info)) / 2);
    waddstr(ui_ctx->main_win, device_info);

    char type_info[256];
    snprintf(type_info, sizeof(type_info), "Type: %s", fs_type);
    wmove(ui_ctx->main_win, y++, (max_x - strlen(type_info)) / 2);
    waddstr(ui_ctx->main_win, type_info);

    y++;
    const char *menu_items[] = {
        "1. Filesystem Analyzer",
        "2. Block Browser",
        "3. Inode Browser",
        "4. Edit Superblock",
        "",
        "Q. Quit"
    };

    size_t max_len = 0;
    for (size_t i = 0; i < sizeof(menu_items)/sizeof(menu_items[0]); i++) {
        size_t len = strlen(menu_items[i]);
        if (len > max_len) max_len = len;
    }

    for (size_t i = 0; i < sizeof(menu_items)/sizeof(menu_items[0]); i++) {
        if (menu_items[i][0] == '\0') {
            y++;
            continue;
        }
        wmove(ui_ctx->main_win, y++, (max_x - max_len) / 2);
        waddstr(ui_ctx->main_win, menu_items[i]);
    }

    wrefresh(ui_ctx->main_win);
}

void ui_display_fs_info(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);

    const struct ext2_super_block *sb = &ui_ctx->fs_info->sb;

    int y = 0;

    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "Filesystem Information:");
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "======================");

    char fs_type[32];
    get_fs_type_string(ui_ctx->fs_info, fs_type, sizeof(fs_type));

    y++;
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Filesystem Type: ");
    waddstr(ui_ctx->main_win, fs_type);

    char size_str[64];
    format_value(sb->s_blocks_count * ui_ctx->fs_info->block_size, size_str, sizeof(size_str), true);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Filesystem Size: ");
    waddstr(ui_ctx->main_win, size_str);

    char block_size_str[32];
    snprintf(block_size_str, sizeof(block_size_str), "%u bytes", ui_ctx->fs_info->block_size);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Block Size: ");
    waddstr(ui_ctx->main_win, block_size_str);

    char inode_size_str[32];
    snprintf(inode_size_str, sizeof(inode_size_str), "%u bytes", sb->s_inode_size);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Inode Size: ");
    waddstr(ui_ctx->main_win, inode_size_str);

    char blocks_count_str[32];
    snprintf(blocks_count_str, sizeof(blocks_count_str), "%u", sb->s_blocks_count);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Blocks Count: ");
    waddstr(ui_ctx->main_win, blocks_count_str);

    char free_blocks_str[32];
    snprintf(free_blocks_str, sizeof(free_blocks_str), "%u", sb->s_free_blocks_count);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Free Blocks: ");
    waddstr(ui_ctx->main_win, free_blocks_str);

    char inodes_count_str[32];
    snprintf(inodes_count_str, sizeof(inodes_count_str), "%u", sb->s_inodes_count);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Inodes Count: ");
    waddstr(ui_ctx->main_win, inodes_count_str);

    char free_inodes_str[32];
    snprintf(free_inodes_str, sizeof(free_inodes_str), "%u", sb->s_free_inodes_count);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Free Inodes: ");
    waddstr(ui_ctx->main_win, free_inodes_str);

    char groups_count_str[32];
    snprintf(groups_count_str, sizeof(groups_count_str), "%u", ui_ctx->fs_info->groups_count);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Block Groups: ");
    waddstr(ui_ctx->main_win, groups_count_str);

    char blocks_per_group_str[32];
    snprintf(blocks_per_group_str, sizeof(blocks_per_group_str), "%u", ui_ctx->fs_info->blocks_per_group);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Blocks Per Group: ");
    waddstr(ui_ctx->main_win, blocks_per_group_str);

    char inodes_per_group_str[32];
    snprintf(inodes_per_group_str, sizeof(inodes_per_group_str), "%u", ui_ctx->fs_info->inodes_per_group);
    wmove(ui_ctx->main_win, y++, 2);
    waddstr(ui_ctx->main_win, "Inodes Per Group: ");
    waddstr(ui_ctx->main_win, inodes_per_group_str);

    y++;
    char group_title[64];
    snprintf(group_title, sizeof(group_title), "Block Group #%d Information:", ui_ctx->current_group);
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, group_title);
    
    wmove(ui_ctx->main_win, y++, 0);
    waddstr(ui_ctx->main_win, "=============================");

    if ((uint32_t)ui_ctx->current_group < ui_ctx->fs_info->groups_count) {
        struct ext2_group_desc *gd = &ui_ctx->fs_info->group_desc[ui_ctx->current_group];

        char block_bitmap_str[32];
        snprintf(block_bitmap_str, sizeof(block_bitmap_str), "%u", gd->bg_block_bitmap);
        wmove(ui_ctx->main_win, y++, 2);
        waddstr(ui_ctx->main_win, "Block Bitmap: ");
        waddstr(ui_ctx->main_win, block_bitmap_str);

        char inode_bitmap_str[32];
        snprintf(inode_bitmap_str, sizeof(inode_bitmap_str), "%u", gd->bg_inode_bitmap);
        wmove(ui_ctx->main_win, y++, 2);
        waddstr(ui_ctx->main_win, "Inode Bitmap: ");
        waddstr(ui_ctx->main_win, inode_bitmap_str);

        char inode_table_str[32];
        snprintf(inode_table_str, sizeof(inode_table_str), "%u", gd->bg_inode_table);
        wmove(ui_ctx->main_win, y++, 2);
        waddstr(ui_ctx->main_win, "Inode Table: ");
        waddstr(ui_ctx->main_win, inode_table_str);

        char free_blocks_count_str[32];
        snprintf(free_blocks_count_str, sizeof(free_blocks_count_str), "%u", gd->bg_free_blocks_count);
        wmove(ui_ctx->main_win, y++, 2);
        waddstr(ui_ctx->main_win, "Free Blocks Count: ");
        waddstr(ui_ctx->main_win, free_blocks_count_str);

        char free_inodes_count_str[32];
        snprintf(free_inodes_count_str, sizeof(free_inodes_count_str), "%u", gd->bg_free_inodes_count);
        wmove(ui_ctx->main_win, y++, 2);
        waddstr(ui_ctx->main_win, "Free Inodes Count: ");
        waddstr(ui_ctx->main_win, free_inodes_count_str);

        char used_dirs_count_str[32];
        snprintf(used_dirs_count_str, sizeof(used_dirs_count_str), "%u", gd->bg_used_dirs_count);
        wmove(ui_ctx->main_win, y++, 2);
        waddstr(ui_ctx->main_win, "Used Directories Count: ");
        waddstr(ui_ctx->main_win, used_dirs_count_str);
    } else {
        wmove(ui_ctx->main_win, y++, 2);
        waddstr(ui_ctx->main_win, "Invalid block group number");
    }

    wrefresh(ui_ctx->main_win);
}

void ui_display_block_browser(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);

    char title[128];
    snprintf(title, sizeof(title), "Block Browser - Block %d of %d",
              ui_ctx->current_block, ui_ctx->fs_info->sb.s_blocks_count - 1);
    wmove(ui_ctx->main_win, 0, 0);
    waddstr(ui_ctx->main_win, title);
    
    wmove(ui_ctx->main_win, 1, 0);
    waddstr(ui_ctx->main_win, "===============================");

    bool is_allocated = is_block_allocated(ui_ctx->fs_info, ui_ctx->current_block);

    char block_type[64] = "Regular Data Block";

    if (ui_ctx->current_block == 0) {
        strcpy(block_type, "Reserved (Block 0)");
    } else if (ui_ctx->current_block == 1) {
        strcpy(block_type, "Superblock");
    } else if ((uint32_t)ui_ctx->current_block >= 1 && (uint32_t)ui_ctx->current_block < 1 + ui_ctx->fs_info->groups_count) {
        strcpy(block_type, "Group Descriptor");
    } else {
        for (uint32_t i = 0; i < ui_ctx->fs_info->groups_count; i++) {
            struct ext2_group_desc *gd = &ui_ctx->fs_info->group_desc[i];

            if ((uint32_t)ui_ctx->current_block == gd->bg_block_bitmap) {
                snprintf(block_type, sizeof(block_type), "Block Bitmap (Group %u)", i);
                break;
            } else if ((uint32_t)ui_ctx->current_block == gd->bg_inode_bitmap) {
                snprintf(block_type, sizeof(block_type), "Inode Bitmap (Group %u)", i);
                break;
            } else if ((uint32_t)ui_ctx->current_block >= gd->bg_inode_table &&
                       (uint32_t)ui_ctx->current_block < gd->bg_inode_table +
                       (ui_ctx->fs_info->inodes_per_group * ui_ctx->fs_info->sb.s_inode_size + ui_ctx->fs_info->block_size - 1) /
                        ui_ctx->fs_info->block_size) {
                snprintf(block_type, sizeof(block_type), "Inode Table (Group %u)", i);
                break;
            }
        }
    }

    char status_info[64];
    snprintf(status_info, sizeof(status_info), "Block Status: %s", is_allocated ? "Allocated" : "Free");
    wmove(ui_ctx->main_win, 3, 0);
    waddstr(ui_ctx->main_win, status_info);

    char type_info[128];
    snprintf(type_info, sizeof(type_info), "Block Type: %s", block_type);
    wmove(ui_ctx->main_win, 4, 0);
    waddstr(ui_ctx->main_win, type_info);

    char size_info[64];
    snprintf(size_info, sizeof(size_info), "Block Size: %u bytes", ui_ctx->fs_info->block_size);
    wmove(ui_ctx->main_win, 5, 0);
    waddstr(ui_ctx->main_win, size_info);

    uint32_t block_group = ui_ctx->current_block / ui_ctx->fs_info->blocks_per_group;
    uint32_t block_in_group = ui_ctx->current_block % ui_ctx->fs_info->blocks_per_group;

    char group_info[64];
    snprintf(group_info, sizeof(group_info), "Block Group: %u", block_group);
    wmove(ui_ctx->main_win, 6, 0);
    waddstr(ui_ctx->main_win, group_info);

    char block_in_group_info[64];
    snprintf(block_in_group_info, sizeof(block_in_group_info), "Block in Group: %u", block_in_group);
    wmove(ui_ctx->main_win, 7, 0);
    waddstr(ui_ctx->main_win, block_in_group_info);

    uint8_t *block_data = malloc(ui_ctx->fs_info->block_size);
    if (block_data) {
        if (read_block(ui_ctx->fs_info, ui_ctx->current_block, block_data) == 0) {
            wmove(ui_ctx->main_win, 9, 0);
            waddstr(ui_ctx->main_win, "Block Data (first 256 bytes):");

            int max_rows = 10;
            int bytes_per_row = 16;
            uint32_t rows = (ui_ctx->fs_info->block_size < (uint32_t)(max_rows * bytes_per_row)) ?
                       ui_ctx->fs_info->block_size / bytes_per_row : (uint32_t)max_rows;

            for (uint32_t i = 0; i < rows; i++) {
                char hex_line[256];
                char ascii_line[32];
                
                snprintf(hex_line, sizeof(hex_line), "%04X: ", i * bytes_per_row);
                
                // Формируем hex часть
                for (int j = 0; j < bytes_per_row; j++) {
                    uint32_t offset = i * bytes_per_row + j;
                    if (offset < ui_ctx->fs_info->block_size) {
                        char hex_byte[8];
                        snprintf(hex_byte, sizeof(hex_byte), "%02X ", block_data[offset]);
                        strcat(hex_line, hex_byte);
                    } else {
                        strcat(hex_line, "   ");
                    }
                }
                
                strcat(hex_line, " | ");
                
                // Формируем ASCII часть
                for (int j = 0; j < bytes_per_row; j++) {
                    uint32_t offset = i * bytes_per_row + j;
                    if (offset < ui_ctx->fs_info->block_size) {
                        char c = (char)block_data[offset];
                        ascii_line[j] = isprint((unsigned char)c) ? c : '.';
                    } else {
                        ascii_line[j] = ' ';
                    }
                }
                ascii_line[bytes_per_row] = '\0';
                
                strcat(hex_line, ascii_line);
                
                wmove(ui_ctx->main_win, 10 + (int)i, 0);
                waddstr(ui_ctx->main_win, hex_line);
            }
        } else {
            wmove(ui_ctx->main_win, 9, 0);
            waddstr(ui_ctx->main_win, "Error reading block data");
        }

        free(block_data);
        block_data = NULL;
    } else {
        wmove(ui_ctx->main_win, 9, 0);
        waddstr(ui_ctx->main_win, "Memory allocation error");
    }

    wrefresh(ui_ctx->main_win);
}

void ui_display_inode_browser(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);
    
    char title[128];
    snprintf(title, sizeof(title), "Inode Browser - Inode %d of %d", 
              ui_ctx->current_inode, ui_ctx->fs_info->sb.s_inodes_count - 1);
    wmove(ui_ctx->main_win, 0, 0);
    waddstr(ui_ctx->main_win, title);
    
    wmove(ui_ctx->main_win, 1, 0);
    waddstr(ui_ctx->main_win, "===============================");
    
    bool is_allocated = is_inode_allocated(ui_ctx->fs_info, ui_ctx->current_inode);
    
    char status_info[64];
    snprintf(status_info, sizeof(status_info), "Inode Status: %s", is_allocated ? "Allocated" : "Free");
    wmove(ui_ctx->main_win, 3, 0);
    waddstr(ui_ctx->main_win, status_info);
    
    struct ext2_inode inode;
    if (read_inode(ui_ctx->fs_info, ui_ctx->current_inode, &inode) == 0) {
        char mode_info[32];
        snprintf(mode_info, sizeof(mode_info), "Mode: 0%o", inode.i_mode);
        wmove(ui_ctx->main_win, 4, 0);
        waddstr(ui_ctx->main_win, mode_info);
        
        char file_type[32] = "Unknown";
        if (S_ISREG(inode.i_mode)) strcpy(file_type, "Regular File");
        else if (S_ISDIR(inode.i_mode)) strcpy(file_type, "Directory");
        else if (S_ISLNK(inode.i_mode)) strcpy(file_type, "Symbolic Link");
        else if (S_ISCHR(inode.i_mode)) strcpy(file_type, "Character Device");
        else if (S_ISBLK(inode.i_mode)) strcpy(file_type, "Block Device");
        else if (S_ISFIFO(inode.i_mode)) strcpy(file_type, "FIFO");
        else if (S_ISSOCK(inode.i_mode)) strcpy(file_type, "Socket");
        
        char type_info[64];
        snprintf(type_info, sizeof(type_info), "File Type: %s", file_type);
        wmove(ui_ctx->main_win, 5, 0);
        waddstr(ui_ctx->main_win, type_info);
        
        char size_str[32];
        format_value(inode.i_size, size_str, sizeof(size_str), true);
        wmove(ui_ctx->main_win, 6, 0);
        waddstr(ui_ctx->main_win, "Size: ");
        waddstr(ui_ctx->main_win, size_str);
        
        char links_info[32];
        snprintf(links_info, sizeof(links_info), "Links: %u", inode.i_links_count);
        wmove(ui_ctx->main_win, 7, 0);
        waddstr(ui_ctx->main_win, links_info);
        
        char uid_info[32];
        snprintf(uid_info, sizeof(uid_info), "UID: %u", inode.i_uid);
        wmove(ui_ctx->main_win, 8, 0);
        waddstr(ui_ctx->main_win, uid_info);
        
        char gid_info[32];
        snprintf(gid_info, sizeof(gid_info), "GID: %u", inode.i_gid);
        wmove(ui_ctx->main_win, 9, 0);
        waddstr(ui_ctx->main_win, gid_info);
        
        char atime_buf[32], mtime_buf[32], ctime_buf[32];
        time_t atime = inode.i_atime;
        time_t mtime = inode.i_mtime;
        time_t ctime = inode.i_ctime;
        
        struct tm *atime_tm = localtime(&atime);
        struct tm *mtime_tm = localtime(&mtime);
        struct tm *ctime_tm = localtime(&ctime);
        
        strftime(atime_buf, sizeof(atime_buf), "%Y-%m-%d %H:%M:%S", atime_tm);
        strftime(mtime_buf, sizeof(mtime_buf), "%Y-%m-%d %H:%M:%S", mtime_tm);
        strftime(ctime_buf, sizeof(ctime_buf), "%Y-%m-%d %H:%M:%S", ctime_tm);
        
        wmove(ui_ctx->main_win, 10, 0);
        waddstr(ui_ctx->main_win, "Access Time: ");
        waddstr(ui_ctx->main_win, atime_buf);
        
        wmove(ui_ctx->main_win, 11, 0);
        waddstr(ui_ctx->main_win, "Modify Time: ");
        waddstr(ui_ctx->main_win, mtime_buf);
        
        wmove(ui_ctx->main_win, 12, 0);
        waddstr(ui_ctx->main_win, "Change Time: ");
        waddstr(ui_ctx->main_win, ctime_buf);
        
        wmove(ui_ctx->main_win, 14, 0);
        waddstr(ui_ctx->main_win, "Direct Blocks:");
        
        for (int i = 0; i < 12 && i < 8; i++) {
            char block_info[32];
            snprintf(block_info, sizeof(block_info), "[%d]: %u", i, inode.i_block[i]);
            wmove(ui_ctx->main_win, 15 + i, 2);
            waddstr(ui_ctx->main_win, block_info);
        }
        
        wmove(ui_ctx->main_win, 23, 0);
        waddstr(ui_ctx->main_win, "Indirect Blocks:");
        
        char single_info[32];
        snprintf(single_info, sizeof(single_info), "Single: %u", inode.i_block[12]);
        wmove(ui_ctx->main_win, 24, 2);
        waddstr(ui_ctx->main_win, single_info);
        
        char double_info[32];
        snprintf(double_info, sizeof(double_info), "Double: %u", inode.i_block[13]);
        wmove(ui_ctx->main_win, 25, 2);
        waddstr(ui_ctx->main_win, double_info);
        
        char triple_info[32];
        snprintf(triple_info, sizeof(triple_info), "Triple: %u", inode.i_block[14]);
        wmove(ui_ctx->main_win, 26, 2);
        waddstr(ui_ctx->main_win, triple_info);
    } else {
        wmove(ui_ctx->main_win, 4, 0);
        waddstr(ui_ctx->main_win, "Error reading inode");
    }
    
    wrefresh(ui_ctx->main_win);
}

void ui_display_binary_editor(ui_context_t *ui_ctx) {
    editor_render(ui_ctx->editor_ctx);
}

static bool ui_handle_binary_editor_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case 27: // ESC
            ui_set_mode(ui_ctx, UI_MODE_MENU);
            return true;
        case 'q':
        case 'Q':
            return false;
        default:
            return editor_handle_key(ui_ctx->editor_ctx, key);
    }
}

void ui_main_loop(ui_context_t *ui_ctx) {
    bool running = true;
    
    ui_set_mode(ui_ctx, UI_MODE_MENU);
    
    while (running) {
        switch (ui_ctx->current_mode) {
            case UI_MODE_MENU:
                ui_display_menu(ui_ctx);
                break;
            case UI_MODE_ANALYZER:
                ui_display_fs_info(ui_ctx);
                break;
            case UI_MODE_BLOCK_BROWSER:
                ui_display_block_browser(ui_ctx);
                break;
            case UI_MODE_INODE_BROWSER:
                ui_display_inode_browser(ui_ctx);
                break;
            case UI_MODE_BINARY_EDITOR:
                ui_display_binary_editor(ui_ctx);
                break;
        }
        
        ui_display_help(ui_ctx);
        
        int key = getch();
        
        if (key == KEY_F(1)) {
            ui_display_detailed_help(ui_ctx);
            continue; 
        }
        
        switch (ui_ctx->current_mode) {
            case UI_MODE_MENU:
                running = ui_handle_menu_input(ui_ctx, key);
                break;
            case UI_MODE_ANALYZER:
                running = ui_handle_analyzer_input(ui_ctx, key);
                break;
            case UI_MODE_BLOCK_BROWSER:
                running = ui_handle_block_browser_input(ui_ctx, key);
                break;
            case UI_MODE_INODE_BROWSER:
                running = ui_handle_inode_browser_input(ui_ctx, key);
                break;
            case UI_MODE_BINARY_EDITOR:
                running = ui_handle_binary_editor_input(ui_ctx, key);
                break;
        }
    }
}

static bool ui_handle_menu_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case '1':
            ui_set_mode(ui_ctx, UI_MODE_ANALYZER);
            return true;
        case '2':
            ui_set_mode(ui_ctx, UI_MODE_BLOCK_BROWSER);
            return true;
        case '3':
            ui_set_mode(ui_ctx, UI_MODE_INODE_BROWSER);
            return true;
        case '4':
            ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
            editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_SUPERBLOCK, 0);
            return true;
        case 'q':
        case 'Q':
            return false;
        default:
            return true;
    }
}

static bool ui_handle_analyzer_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case 27: // ESC
            ui_set_mode(ui_ctx, UI_MODE_MENU);
            return true;
        case 'g': case 'G': {
            char buffer[32];
            if (ui_prompt(ui_ctx, "Enter group number: ", buffer, sizeof(buffer))) {
                uint32_t group;
                if (sscanf(buffer, "%u", &group) == 1 && group < ui_ctx->fs_info->groups_count) {
                    ui_ctx->current_group = (int)group;
                }
            }
            return true;
        }
        case 'q':
        case 'Q':
            return false;
        default:
            return true;
    }
}

static bool ui_handle_block_browser_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case 27:
            ui_set_mode(ui_ctx, UI_MODE_MENU);
            return true;
        case KEY_LEFT:
            if (ui_ctx->current_block > 0) {
                ui_ctx->current_block--;
                ui_display_block_browser(ui_ctx);
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg), "Block Browser - Block %d", ui_ctx->current_block);
                ui_display_status(ui_ctx, status_msg);
            }
            return true;
        case KEY_RIGHT:
            if ((uint32_t)ui_ctx->current_block < ui_ctx->fs_info->sb.s_blocks_count - 1) {
                ui_ctx->current_block++;
                ui_display_block_browser(ui_ctx);
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg), "Block Browser - Block %d", ui_ctx->current_block);
                ui_display_status(ui_ctx, status_msg);
            }
            return true;
        case KEY_UP:
            if (ui_ctx->current_block >= 10) {
                ui_ctx->current_block -= 10;
                ui_display_block_browser(ui_ctx);
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg), "Block Browser - Block %d", ui_ctx->current_block);
                ui_display_status(ui_ctx, status_msg);
            }
            return true;
        case KEY_DOWN:
            if ((uint32_t)ui_ctx->current_block + 10 < ui_ctx->fs_info->sb.s_blocks_count) {
                ui_ctx->current_block += 10;
                ui_display_block_browser(ui_ctx);
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg), "Block Browser - Block %d", ui_ctx->current_block);
                ui_display_status(ui_ctx, status_msg);
            }
            return true;
        case 'e':
        case 'E':
            ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
            editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_BLOCK, ui_ctx->current_block);
            return true;
        case 'g':
        case 'G': {
            char buffer[32];
            if (ui_prompt(ui_ctx, "Enter block number: ", buffer, sizeof(buffer))) {
                int block = atoi(buffer);
                if ( (uint32_t)block < ui_ctx->fs_info->sb.s_blocks_count) {
                    ui_ctx->current_block = block;
                    ui_display_block_browser(ui_ctx);
                    char status_msg[64];
                    snprintf(status_msg, sizeof(status_msg), "Block Browser - Block %d", ui_ctx->current_block);
                    ui_display_status(ui_ctx, status_msg);
                } else {
                    ui_show_error(ui_ctx, "Invalid block number");
                }
            }
            return true;
        }
        case 'q':
        case 'Q':
            return false;
        default:
            return true;
    }
}

static bool ui_handle_inode_browser_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case 27:
            ui_set_mode(ui_ctx, UI_MODE_MENU);
            return true;
        case KEY_LEFT:
            if (ui_ctx->current_inode > 1) {
                ui_ctx->current_inode--;
                ui_display_inode_browser(ui_ctx);
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg), "Inode Browser - Inode %d", ui_ctx->current_inode);
                ui_display_status(ui_ctx, status_msg);
            }
            return true;
        case KEY_RIGHT:
            if ((uint32_t)ui_ctx->current_inode < ui_ctx->fs_info->sb.s_inodes_count - 1) {
                ui_ctx->current_inode++;
                ui_display_inode_browser(ui_ctx);
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg), "Inode Browser - Inode %d", ui_ctx->current_inode);
                ui_display_status(ui_ctx, status_msg);
            }
            return true;
        case KEY_UP:
            if (ui_ctx->current_inode > 10) {
                ui_ctx->current_inode -= 10;
                ui_display_inode_browser(ui_ctx);
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg), "Inode Browser - Inode %d", ui_ctx->current_inode);
                ui_display_status(ui_ctx, status_msg);
            }
            return true;
        case KEY_DOWN:
            if ((uint32_t)ui_ctx->current_inode + 10 < ui_ctx->fs_info->sb.s_inodes_count) {
                ui_ctx->current_inode += 10;
                ui_display_inode_browser(ui_ctx);
                char status_msg[64];
                snprintf(status_msg, sizeof(status_msg), "Inode Browser - Inode %d", ui_ctx->current_inode);
                ui_display_status(ui_ctx, status_msg);
            }
            return true;
        case 'e':
        case 'E':
            ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
            editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_INODE, ui_ctx->current_inode);
            return true;
        case 'g':
        case 'G': {
            char buffer[32];
            if (ui_prompt(ui_ctx, "Enter inode number: ", buffer, sizeof(buffer))) {
                int inode = atoi(buffer);
                if ((uint32_t)inode > 0 && (uint32_t)inode < ui_ctx->fs_info->sb.s_inodes_count) {
                    ui_ctx->current_inode = inode;
                    ui_display_inode_browser(ui_ctx);
                    char status_msg[64];
                    snprintf(status_msg, sizeof(status_msg), "Inode Browser - Inode %d", ui_ctx->current_inode);
                    ui_display_status(ui_ctx, status_msg);
                } else {
                    ui_show_error(ui_ctx, "Invalid inode number");
                }
            }
            return true;
        }
        case 'q':
        case 'Q':
            return false;
        default:
            return true;
    }
}
