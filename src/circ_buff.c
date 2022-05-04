#include "circ_buff.h"
#include <stdbool.h>

#define BUFF_BEGIN(state) &state->buff[0]
#define BUFF_END(state) &state->buff[BUFF_SIZE]

void init_buff(buff_state_t *state) {
    state->read_head = BUFF_BEGIN(state);
    state->write_head = BUFF_BEGIN(state);
}

bool buffer_empty(buff_state_t *state) {
    return state->read_head == state->write_head;
}

bool buffer_full(buff_state_t *state) {
    return (
        state->read_head == state->write_head + 1 ||
        (
            state->read_head == BUFF_BEGIN(state) && 
            state->write_head == (BUFF_END(state) - 1)
        )
    );
}

// Consumer a character from the circular buffer
// Passes this character into the consumer function
// Returns 1 on success, 0 if nothing to read
int consumer_read(buff_state_t *state, consumer_t consumer) {
    if (buffer_empty(state)) return 0;

    consumer(*state->read_head++);

    if (state->read_head >= BUFF_END(state)) {
        state->read_head = BUFF_BEGIN(state);
    }
    return 1;
}

// Writes a character to the circular buffer
// Returns 1 on success, 0 if buffer full
int producer_write(char c, buff_state_t *state) {
    if (buffer_full(state)) return 0;

    *state->write_head++ = c;

    if (state->write_head >= BUFF_END(state)) {
        state->write_head = BUFF_BEGIN(state);
    }
    return 1;
}