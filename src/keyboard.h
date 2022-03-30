#ifndef KEYBOARD_H
#define KEYBOARD_H

int init_ps2_controller(void);
int init_keyboard(void);
char poll_keyboard();

#endif