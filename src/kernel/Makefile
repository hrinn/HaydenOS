CC := x86_64-elf-gcc
LD := x86_64-elf-ld
CFLAGS := -ffreestanding -Wall -Werror -pedantic -mno-red-zone -fomit-frame-pointer -g -I../lib -fPIC -fno-pie

build_dir := ../../build
linker := linker.ld
kernel := $(build_dir)/img/boot/kernel.bin

asm_src_files := $(wildcard *.asm)
asm_obj_files := $(patsubst %.asm, \
	$(build_dir)/%.o, $(asm_src_files))

c_src_files := $(wildcard *.c)
c_obj_files := $(patsubst %.c, \
	$(build_dir)/%.o, $(c_src_files))

$(kernel): $(asm_obj_files) $(c_obj_files) $(linker)
	@mkdir -p $(build_dir)/img/boot
	@$(LD) -n -T $(linker) -o $(kernel) $(asm_obj_files) $(c_obj_files)

$(build_dir)/%.o: %.asm
	@nasm -felf64 $< -o $@

$(build_dir)/%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@