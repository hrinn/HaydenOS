#include "circ_buff.h"

#define BUFF_BEGIN(state) &state->buff[0]
#define BUFF_END(state) &state->buff[BUFF_SIZE]

void init_buff(buff_state_t *state) {
    state->read_head = BUFF_BEGIN(state);
    state->write_head = BUFF_BEGIN(state);
}

bool is_buffer_empty(buff_state_t *state) {
    return state->read_head == state->write_head;
}

bool is_buffer_full(buff_state_t *state) {
    return (
        state->read_head == state->write_head + 1 ||
        (
            state->read_head == BUFF_BEGIN(state) && 
            state->write_head == (BUFF_END(state) - 1)
        )
    );
}

// Consumes a character from the circular buffer
// Places this character at the character pointer address
// Returns 1 on success, 0 if nothing to read
int consumer_read(buff_state_t *state, char *res) {
    if (is_buffer_empty(state)) return 0;

    *res = *state->read_head++;

    if (state->read_head >= BUFF_END(state)) {
        state->read_head = BUFF_BEGIN(state);
    }
    return 1;
}

// Writes a character to the circular buffer
// Returns 1 on success, 0 if buffer full
int producer_write(char c, buff_state_t *state) {
    if (is_buffer_full(state)) return 0;

    *state->write_head++ = c;

    if (state->write_head >= BUFF_END(state)) {
        state->write_head = BUFF_BEGIN(state);
    }
    return 1;
}