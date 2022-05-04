#ifndef CIRC_BUFF_H
#define CIRC_BUFF_H

#include <stdint-gcc.h>

#define BUFF_SIZE 64

typedef struct buff_state buff_state_t;
struct buff_state {
    char buff[BUFF_SIZE];
    char *read_head;
    char *write_head;
};

typedef void (*consumer_t)(char);

void init_buff(buff_state_t *);
int consumer_read(buff_state_t *, consumer_t);
int producer_write(char, buff_state_t *);

#endif