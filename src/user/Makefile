CC := x86_64-elf-gcc
LD := x86_64-elf-ld
CFLAGS := -ffreestanding -Wall -Werror -pedantic -I../lib -g

build_dir := ../../build
linker := linker.ld
init := $(build_dir)/init.bin

asm_src_files := $(wildcard *.asm)
asm_obj_files := $(patsubst %.asm, \
	$(build_dir)/%.o, $(asm_src_files))

c_src_files := $(wildcard *.c)
c_obj_files := $(patsubst %.c, \
	$(build_dir)/%.o, $(c_src_files))

$(init): $(asm_obj_files) $(c_obj_files) $(linker)
	@mkdir -p $(build_dir)/img/boot
	@$(LD) -n -T $(linker) -o $(init) $(asm_obj_files) $(c_obj_files) $(build_dir)/sys_call_ints.o

$(build_dir)/%.o: %.asm
	@nasm -felf64 $< -o $@

$(build_dir)/%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@