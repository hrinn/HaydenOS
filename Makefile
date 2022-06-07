LD := x86_64-elf-ld
CC := x86_64-elf-gcc
LOAD_CFLAGS := -ffreestanding -Wall -Werror -pedantic -Isrc/lib -g
KERN_CFLAGS = -ffreestanding -Wall -Werror -pedantic -mno-red-zone -fomit-frame-pointer -g -Isrc/lib -mcmodel=large
USER_CFLAGS := -ffreestanding -Wall -Werror -pedantic -Isrc/lib -g

loader := build/img/boot/loader.bin
kernel := build/img/boot/kernel.bin

loader_linker := src/boot/linker.ld
kernel_linker := src/kernel/linker.ld
user_linker := src/user/linker.ld

grub_cfg := build/img/boot/grub/grub.cfg
img := build/HaydenOS.img

.PHONY: all clean run debug gdb clean_img release

all: $(img)

clean:
	@rm -r build

clean_img:
	@sudo umount /mnt/fatgrub
	@sudo losetup -d $(loop0)
	@sudo losetup -d $(loop1)
	@sudo rmdir /mnt/fatgrub

gdb: KERN_CFLAGS+=-DGDB
gdb: run

run: $(img)
	@qemu-system-x86_64 -s -drive format=raw,file=$(img) -serial stdio

$(img): $(grub_cfg) $(loader) $(kernel) build/init.bin
	@tools/img.sh

# part: $(kernel) $(grub_cfg) build/init.bin
# 	tools/part.sh

### Make loader binary ###
loader_asm_src := $(wildcard src/boot/*.asm)
loader_asm_obj += $(patsubst src/boot/%.asm, \
	build/%.o, $(loader_asm_src))
loader_c_src := $(wildcard src/boot/*.c)
loader_c_obj += $(patsubst src/boot/%.c, \
	build/%.o, $(loader_c_src))

$(loader): $(loader_asm_obj) $(loader_c_obj) $(loader_linker)
	@mkdir -p build/img/boot
	@$(LD) -n -T $(loader_linker) -o $(loader) $(loader_asm_obj) $(loader_c_obj)

$(grub_cfg): src/boot/grub.cfg
	@mkdir -p build/img/boot/grub
	@cp src/boot/grub.cfg $(grub_cfg)

build/%.o: src/boot/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@

build/%.o: src/boot/%.c
	@$(CC) $(LOAD_CFLAGS) -g -c $< -o $@

### Make kernel binary ###
kernel_asm_src := $(wildcard src/kernel/*.asm)
kernel_asm_obj += $(patsubst src/kernel/%.asm, \
	build/%.o, $(kernel_asm_src))
kernel_c_src := $(wildcard src/kernel/*.c)
kernel_c_obj += $(patsubst src/kernel/%.c, \
	build/%.o, $(kernel_c_src))

$(kernel): $(kernel_asm_obj) $(kernel_c_obj) $(kernel_linker)
	@mkdir -p build/img/boot
	@$(LD) -n -T $(kernel_linker) -o $(kernel) $(kernel_asm_obj) $(kernel_c_obj) 

# compile kernel assembly files
build/%.o: src/kernel/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@

# compile kernel c files
build/%.o: src/kernel/%.c src/kernel/*.h
	@mkdir -p $(shell dirname $@)
	@$(CC) $(KERN_CFLAGS) -c $< -o $@

### Make user binaries ###
build/init.o: src/user/init.c
	@$(CC) $(USER_CFLAGS) -c src/user/init.c -o build/init.o

build/init.bin: build/init.o
	@$(LD) -n -T $(user_linker) -o build/init.bin build/init.o build/sys_call_ints.o