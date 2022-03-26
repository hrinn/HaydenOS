extern void VGA_clear();
extern void VGA_display_str(const char *);

void kmain() {
    VGA_clear();
    VGA_display_str("Welcome to HaydenOS\n");
    VGA_display_str("Enjoy your stay!\n");

    while (1) {
        asm("hlt");
    }
}