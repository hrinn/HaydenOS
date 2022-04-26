#include "error.h"
#include "vga.h"
#include "printk.h"

void blue_screen(char *msg) {
    VGA_set_bg_color(VGA_BLUE);
    VGA_paint();

    VGA_set_fg_color(VGA_WHITE);
    VGA_display_str("HaydenOS\n\nCRITICAL ERROR!\n", 27);
    printk("%s\n", msg);
    VGA_display_str("\nHalting...", 11);

    while (1) asm ("hlt");
}