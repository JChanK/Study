#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

typedef struct message
{
    uint8_t    type;
    uint16_t   hash;
    uint8_t    size;
    uint8_t*   data;
} mes_t;

typedef struct node 
{
    mes_t*  message;
    struct node* next;
    struct node* prev;
} node_t;

typedef struct queue
{
    node_t* head;
    node_t* tail;
    size_t  added;
    size_t  deleted;
    size_t  cur;
    size_t  size;
} queue_t;

void push(node_t**, node_t**);
void pop(node_t**, node_t**);
void init_mes(mes_t*);
void print_mes(mes_t*);
void mes_clear(mes_t*);
void queue_clear(queue_t*);
