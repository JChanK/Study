#ifndef CHILD_OPS_H
#define CHILD_OPS_H
#define _POSIX_C_SOURCE 199309L
#include "globals.h"

void run_child_process();
void init_child();
void update_pair_stats();
void ask_parent_for_output();
void output_stats_report();

#endif
