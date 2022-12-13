export CC := x86_64-elf-gcc
export LD := x86_64-elf-ld
CFLAGS := -ffreestanding -Wall -Werror -pedantic -I../lib

build_dir := bin
img := bin/HaydenOS.img
kernel := bin/img/boot/kernel.bin
init := bin/init.bin
grub_cfg := bin/img/boot/grub/grub.cfg

.PHONY: all clean run gdb release

all: $(img)

gdb: CFLAGS += -DGDB -g
gdb: $(img)

release: CFLAGS += -Os
release: $(img)

export CFLAGS

$(img): $(build_dir) $(grub_cfg) $(kernel) $(init)
	@tools/img.sh

clean:
	@rm -r bin

run: $(img)
	qemu-system-x86_64 -s -drive format=raw,file=$(img) -serial stdio

$(kernel):
	$(MAKE) -C src/kernel

$(init):
	$(MAKE) -C src/user

$(build_dir):
	@mkdir -p bin

$(grub_cfg): src/kernel/grub.cfg
	@mkdir -p bin/img/boot/grub
	@cp src/kernel/grub.cfg $(grub_cfg)