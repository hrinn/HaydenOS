export CC := x86_64-elf-gcc
export LD := x86_64-elf-ld

CFLAGS := -ffreestanding -Wall -Werror -pedantic -I../include

export out_dir := bin
export obj_dir := obj

disk_img := bin/HaydenOS.img
kernel := $(out_dir)/img/boot/kernel.bin
init := $(out_dir)/img/bin/init.bin

.PHONY: all clean run gdb release

all: bins

gdb: CFLAGS += -g
gdb: DEBUG_FLAGS := -s -S
gdb: run

release: CFLAGS += -Os
release: bins

export CFLAGS

$(kernel): $(obj_dir) $(out_dir)
	@$(MAKE) -C src/kernel

$(init): $(obj_dir) $(out_dir)
	@$(MAKE) -C src/user

$(out_dir):
	@mkdir -p $(out_dir)

$(obj_dir):
	@mkdir -p $(obj_dir)

$(out_dir)/img: src/kernel/boot/grub.cfg
	@mkdir -p $(out_dir)/img/boot/grub
	@cp src/kernel/boot/grub.cfg $(out_dir)/img/boot/grub/grub.cfg
	@mkdir -p $(out_dir)/img/bin

bins: $(out_dir)/img $(kernel) $(init)

$(disk_img): bins tools/make_img.sh
	@tools/make_img.sh

run: $(disk_img)
	qemu-system-x86_64 $(DEBUG_FLAGS) -drive format=raw,file=$(disk_img) -serial stdio

clean:
	@rm -r $(out_dir) $(obj_dir)