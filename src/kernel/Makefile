CFLAGS += -mno-red-zone -fPIC -fomit-frame-pointer

build_dir := ../../bin
linker := linker.ld
kernel := $(build_dir)/img/boot/kernel.bin

asm_src_files := $(wildcard *.asm)
asm_obj_files := $(patsubst %.asm, \
	$(build_dir)/%.o, $(asm_src_files))

c_src_files := $(wildcard *.c)
c_obj_files := $(patsubst %.c, \
	$(build_dir)/%.o, $(c_src_files))

$(kernel): $(asm_obj_files) $(c_obj_files) $(linker)
	mkdir -p $(build_dir)/img/boot
	$(LD) -n -T $(linker) -o $(kernel) $(asm_obj_files) $(c_obj_files)

$(build_dir)/%.o: %.asm
	nasm -felf64 $< -o $@

$(build_dir)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@