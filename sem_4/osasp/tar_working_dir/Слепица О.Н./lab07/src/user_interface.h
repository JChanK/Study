#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "data_structures.h"

void show_menu(void);
void clear_input(void);
int get_user_choice(void);
void handle_list_command(void);
void handle_get_command(void);
void handle_modify_command(void);
void handle_put_command(void);
void run_main_loop(void);

#endif
