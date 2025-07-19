#ifndef UI_H
#define UI_H
#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include "analyzer.h"
#include "editor.h"

typedef enum {
    UI_MODE_MENU,               // Main menu
    UI_MODE_ANALYZER,           // Filesystem analyzer
    UI_MODE_BLOCK_BROWSER,      // Block browser
    UI_MODE_INODE_BROWSER,      // Inode browser
    UI_MODE_BINARY_EDITOR       // Binary editor
} ui_mode_t;

typedef struct {
    WINDOW *main_win;           // Main window
    WINDOW *status_win;         // Status bar window
    WINDOW *help_win;           // Help window
    ui_mode_t current_mode;     // Current UI mode
    fs_info_t *fs_info;         // Filesystem information
    editor_context_t *editor_ctx; // Editor context
    int current_block;          // Current block number (for block browser)
    int current_inode;          // Current inode number (for inode browser)
    int current_group;          // Current block group
} ui_context_t;

ui_context_t *ui_init(fs_info_t *fs_info);

void ui_cleanup(ui_context_t *ui_ctx);

void ui_main_loop(ui_context_t *ui_ctx);

void ui_set_mode(ui_context_t *ui_ctx, ui_mode_t mode);

void ui_display_fs_info(ui_context_t *ui_ctx);

void ui_display_block_browser(ui_context_t *ui_ctx);

void ui_display_inode_browser(ui_context_t *ui_ctx);

void ui_display_binary_editor(ui_context_t *ui_ctx);

void ui_display_help(ui_context_t *ui_ctx);

void ui_show_error(ui_context_t *ui_ctx, const char *message);

bool ui_prompt(ui_context_t *ui_ctx, const char *prompt, char *buffer, size_t max_len);

#endif /* UI_H */
