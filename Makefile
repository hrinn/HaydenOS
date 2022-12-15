export CC := x86_64-elf-gcc
export LD := x86_64-elf-ld
CFLAGS := -ffreestanding -Wall -Werror -pedantic -I../lib

build_dir := bin
img := bin/HaydenOS.img
kernel := bin/img/boot/kernel.bin
init := bin/init.bin
grub_cfg := bin/img/boot/grub/grub.cfg

.PHONY: all clean run gdb release

all: boot

gdb: CFLAGS += -g
gdb: DEBUG_FLAGS := -s -S
gdb: run

release: CFLAGS += -Os
release: boot

export CFLAGS

$(kernel):
	@$(MAKE) -C src/kernel

$(init):
	@$(MAKE) -C src/user

$(build_dir):
	@mkdir -p bin

$(grub_cfg): src/kernel/grub.cfg
	@mkdir -p bin/img/boot/grub
	@cp src/kernel/grub.cfg $(grub_cfg)

boot: $(build_dir) $(kernel) $(init) $(grub_cfg)

$(img): boot
	@tools/img.sh

run: $(img)
	qemu-system-x86_64 $(DEBUG_FLAGS) -drive format=raw,file=$(img) -serial stdio

clean:
	@rm -r bin