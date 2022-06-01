#ifndef CIRC_BUFF_H
#define CIRC_BUFF_H

#include <stdint-gcc.h>
#include <stdbool.h>

#define BUFF_SIZE 1024

typedef struct buff_state buff_state_t;
struct buff_state {
    char buff[BUFF_SIZE];
    char *read_head;
    char *write_head;
};

void init_buff(buff_state_t *);
int consumer_read(buff_state_t *, char *);
int producer_write(char, buff_state_t *);
bool is_buffer_empty(buff_state_t *);
bool is_buffer_full(buff_state_t *);

#endif