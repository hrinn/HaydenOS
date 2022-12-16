#ifndef VGA_H
#define VGA_H

void VGA_clear();
void VGA_display_char(char);
void VGA_display_str(const char *, int);
void VGA_set_bg_color(char);
void VGA_set_fg_color(char);
void VGA_paint();

int VGA_row_count();
int VGA_col_count();
void VGA_display_attr_char(int x, int y, char c, int fg, int bg);

// Colors
#define VGA_BLACK 0x0
#define VGA_BLUE 0x1
#define VGA_GREEN 0x2
#define VGA_CYAN 0x3
#define VGA_RED 0x4
#define VGA_PURPLE 0x5
#define VGA_ORANGE 0x6
#define VGA_LIGHT_GREY 0x7
#define VGA_DARK_GRAY 0x8
#define VGA_BRIGHT_BLUE 0x9
#define VGA_BRIGHT_GREEN 0xA
#define VGA_BRIGHT_CYAN 0xB
#define VGA_MAGENTA 0xC
#define VGA_BRIGHT_PURPLE 0xD
#define VGA_YELLOW 0xE
#define VGA_WHITE 0xF

#endif