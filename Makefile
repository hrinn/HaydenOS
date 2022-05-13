LD := x86_64-elf-ld
CC := x86_64-elf-gcc
CFLAGS := -ffreestanding -Wall -Werror -pedantic -mno-red-zone -fPIC

kernel := build/img/boot/kernel.bin
img := build/HaydenOS.img
iso := build/HaydenOS.iso

loop0 := $(shell losetup -f)

linker_script := src/linker.ld
grub_cfg := build/img/boot/grub/grub.cfg
assembly_source_files := $(wildcard src/*.asm)
assembly_object_files += $(patsubst src/%.asm, \
	build/%.o, $(assembly_source_files))
c_source_files := $(wildcard src/*.c)
c_object_files += $(patsubst src/%.c, \
	build/%.o, $(c_source_files))

.PHONY: all clean run iso img debug gdb run_img run_iso clean_img

all: $(kernel)

clean:
	@rm -r build

clean_img:
	@sudo umount /mnt/fatgrub
	@sudo losetup -d $(loop0)
	@sudo losetup -d $(loop1)
	@sudo rmdir /mnt/fatgrub

debug: CFLAGS+=-DDEBUG
debug: run

gdb: CFLAGS+=-DGDB -g
gdb: run

release: CFLAGS+=-Os
release: $(kernel)

run: run_img

run_img: $(img)
	@qemu-system-x86_64 -s -drive format=raw,file=$(img) -serial stdio

run_iso: $(iso)
	@qemu-system-x86_64 -s -cdrom $(iso) -serial stdio #-soundhw pcspk

img: $(img)

iso: $(iso)

$(img): $(kernel) $(grub_cfg)
	@tools/img.sh

$(iso): $(kernel) $(grub_cfg)
	@grub-mkrescue -o $(iso) build/img 2> /dev/null

$(kernel): $(assembly_object_files) $(c_object_files) $(linker_script)
	@mkdir -p build/img/boot
	@$(LD) -n -T $(linker_script) -o $(kernel) $(assembly_object_files) $(c_object_files) 

$(grub_cfg): src/grub.cfg
	@mkdir -p build/img/boot/grub
	@cp src/grub.cfg $(grub_cfg)

build/%.o: src/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@

# compile c files
build/%.o: src/%.c src/*.h
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) -c $< -o $@