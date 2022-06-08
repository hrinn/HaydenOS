CC := x86_64-elf-gcc
LD := x86_64-elf-ld
CFLAGS := -ffreestanding -Wall -Werror -pedantic -I../lib -g

build_dir := ../../build
linker := linker.ld
loader := $(build_dir)/img/boot/loader.bin
grub_cfg := $(build_dir)/img/boot/grub/grub.cfg

asm_src_files := $(wildcard *.asm)
asm_obj_files := $(patsubst %.asm, \
	$(build_dir)/%.o, $(asm_src_files))

c_src_files := $(wildcard *.c)
c_obj_files := $(patsubst %.c, \
	$(build_dir)/%.o, $(c_src_files))

.PHONY: all

all: $(loader) $(grub_cfg)

$(loader): $(asm_obj_files) $(c_obj_files) $(linker)
	@mkdir -p $(build_dir)/img/boot
	@$(LD) -n -T $(linker) -o $(loader) $(asm_obj_files) $(c_obj_files)

$(grub_cfg): grub.cfg
	@mkdir -p $(build_dir)/img/boot/grub
	@cp grub.cfg $(grub_cfg)

$(build_dir)/%.o: %.asm
	@nasm -felf64 $< -o $@

$(build_dir)/%.o: %.c
	@$(CC) ($CFLAGS) -c $< -o $@