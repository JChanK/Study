#include "queue.h"

void push(node_t** head, node_t** tail) 
{
    if (*head != NULL) 
    {
        node_t *temp = (node_t*)malloc(sizeof(node_t));
        temp->message = (mes_t*)malloc(sizeof(mes_t));
        init_mes(temp->message);
        temp->next = *head;
        temp->prev = *tail;
        (*tail)->next = temp;
        (*head)->prev = temp;
        *tail = temp;
    } 
    else 
    {
        *head = (node_t*)malloc(sizeof(node_t));
        (*head)->message = (mes_t*)malloc(sizeof(mes_t));
        init_mes((*head)->message);
        (*head)->prev = *head;
        (*head)->next = *head;
        *tail = *head;
    }
}

void pop(node_t** head, node_t** tail) 
{
    if (*head != NULL) 
    {
        if (*head != *tail) 
        {
            node_t* temp = *head;
            (*tail)->next = (*head)->next;
            *head = (*head)->next;
            (*head)->prev = *tail;

            if (temp->message != NULL) {
                free(temp->message->data);  
                free(temp->message);      
            }

            free(temp);  
        } 
        else 
        {
            if (*head != NULL && (*head)->message != NULL) {
                free((*head)->message->data);  
                free((*head)->message);        
            }
            free(*head); 
            *head = NULL;
            *tail = NULL;
        }
    }
}

uint16_t crc16(const uint8_t *data, size_t length) 
{
    uint16_t crc = 0xFFFF;  // Initial value
    for (size_t i = 0; i < length; ++i) 
    {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; ++j) 
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021; 
            else
                crc <<= 1;
        }
    }
    return crc;
}

void init_mes(mes_t* message) 
{
    const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    message->type = 0;
    message->size = rand() % 257;
    message->data = (uint8_t*)malloc(message->size * sizeof(uint8_t));
    
    // Заполняем массив данных случайными символами
    for (size_t i = 0; i < message->size; i++) 
    {
        message->data[i] = letters[rand() % (sizeof(letters) - 1)];
    }
    
    message->hash = crc16(message->data, message->size);
}



void print_mes(mes_t* mes) 
{
    if(mes!=NULL){
        printf("Message type: %d, hash: %04x, size: %d, data: ", mes->type, mes->hash, mes->size);
        for(size_t i = 0; i<mes->size; i++)
            printf("%c", mes->data[i]);
        printf("\n");
    }
    else{
        printf("Message is NULL\n");
    }
}

void queue_clear(queue_t* queue) {
    if (!queue) return;

    if (queue->head == NULL) {
        return;
    }

    node_t* current = queue->head;
    do {
        node_t* temp = current;
        current = current->next;

        if (temp->message) {
            mes_clear(temp->message);
            free(temp->message);
        }

        free(temp);
    } while (current != queue->head); 

    queue->head = NULL;
    queue->tail = NULL;
    queue->added = 0;
    queue->deleted = 0;
    queue->cur = 0;
    queue->size = 0;  
}


void mes_clear(mes_t* msg) {
    if (!msg) return;
    
    if (msg->data) {
        free(msg->data);
        msg->data = NULL;
    }
    
    msg->type = 0;
    msg->hash = 0;
    msg->size = 0;
}
