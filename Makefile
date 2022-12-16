export CC := x86_64-elf-gcc
export LD := x86_64-elf-ld

CFLAGS := -ffreestanding -Wall -Werror -pedantic -I../include

export out_dir := bin
export obj_dir := obj

img := bin/HaydenOS.img
kernel := bin/img/boot/kernel.bin
init := bin/init.bin
grub_cfg := bin/img/boot/grub/grub.cfg

.PHONY: all clean run gdb release

all: binaries

gdb: CFLAGS += -g
gdb: DEBUG_FLAGS := -s -S
gdb: run

release: CFLAGS += -Os
release: binaries

export CFLAGS

$(kernel): $(obj_dir) $(out_dir)
	@$(MAKE) -C src/kernel

$(init): $(obj_dir) $(out_dir)
	@$(MAKE) -C src/user

$(out_dir):
	@mkdir -p $(out_dir)/img/boot

$(obj_dir):
	@mkdir -p $(obj_dir)

$(grub_cfg): src/kernel/boot/grub.cfg
	@mkdir -p bin/img/boot/grub
	@cp src/kernel/boot/grub.cfg $(grub_cfg)

binaries: $(kernel) $(init) $(grub_cfg)

$(img): binaries
	@tools/img.sh

run: $(img)
	qemu-system-x86_64 $(DEBUG_FLAGS) -drive format=raw,file=$(img) -serial stdio

clean:
	@rm -r $(out_dir) $(obj_dir)