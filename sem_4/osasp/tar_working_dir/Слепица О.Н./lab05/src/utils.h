#pragma once

void* consumer_routine();
void* producer_routine();
void* producer_routine_cond(void* arg);
void* consumer_routine_cond(void* arg);
