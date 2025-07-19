#ifndef PARENT_OPS_H
#define PARENT_OPS_H
#define _POSIX_C_SOURCE 199309L
#include "globals.h"

void parent_main_loop();
void init_parent();
void cleanup_parent();
void spawn_child_process();
void terminate_last_child();
void terminate_all_children();
void display_process_list();
void handle_user_command(const char* input);
void print_status(const char* message);

void block_all_child_output();
void unblock_all_child_output();
void block_child_output(int child_num);
void unblock_child_output(int child_num);
void request_child_stats(int child_num);
void handle_alarm_signal(int sig);
#endif
