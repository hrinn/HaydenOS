#ifndef VGA_H
#define VGA_H

void VGA_clear();
void VGA_display_char(char);
void VGA_display_str(const char *);
void VGA_display_strn(const char *s, int n);
void VGA_set_bg_color(char);
void VGA_set_fg_color(char);

// Colors
#define BLACK 0x0
#define BLUE 0x1
#define GREEN 0x2
#define CYAN 0x3
#define DARK_RED 0x4
#define PURPLE 0x5
#define GOLD 0x6
#define LIGHT_GRAY 0x7
#define GRAY 0x8
#define PALE_BLUE 0x9
#define MINT 0xA
#define LIGHT_BLUE 0xB
#define RED 0xC
#define PINK 0xD
#define PALE_YELLOW 0xE
#define WHITE 0xF

#endif