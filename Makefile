arch ?= x86_64

ld := $(arch)-elf-ld
cc := $(arch)-elf-gcc
cflags := -ffreestanding -Wall -Wextra -g

kernel := build/kernel-$(arch).bin
img := build/os-$(arch).img
iso := build/os-$(arch).iso

loop0 := /dev/loop13
loop1 := /dev/loop14

linker_script := src/arch/$(arch)/linker.ld
grub_cfg := src/arch/$(arch)/grub.cfg
assembly_source_files := $(wildcard src/arch/$(arch)/*.asm)
assembly_object_files += $(patsubst src/arch/$(arch)/%.asm, \
	build/arch/$(arch)/%.o, $(assembly_source_files))
c_source_files := $(wildcard src/arch/$(arch)/*.c)
c_object_files += $(patsubst src/arch/$(arch)/%.c, \
	build/arch/$(arch)/%.o, $(c_source_files))

.PHONY: all clean run iso

all: $(kernel)

clean:
	@rm -r build

clean_img:
	@sudo umount build/img
	@sudo losetup -d $(loop0)
	@sudo losetup -d $(loop1)

run: run_iso

run_img: $(img)
	@qemu-system-x86_64 -drive format=raw,file=$(img)

run_iso: $(iso)
	@qemu-system-x86_64 -cdrom $(iso)

img: $(img)

iso: $(iso)

$(img): $(kernel) $(grub_cfg)
# Make an empty 64 MiB disk image
	@dd if=/dev/zero of=$(img) bs=512 count=131072
# Create a DOS partition table, with a bootable 32MB FAT32 partition
	@sudo parted $(img) mklabel msdos
# !!! For some reason when parted partitions the entire disk, the result is not bootable
	@sudo parted $(img) mkpart primary fat32 2048s 67584s
	@sudo parted $(img) set 1 boot on
# Attach the disk image to a loop device
	@sudo losetup $(loop0) $(img)
# Attach the disk image to another loop device, offset to the FAT32 partition (1MB)
	@sudo losetup $(loop1) $(img) -o 1MiB
# Format the partition as FAT32
	@sudo mkdosfs -F32 $(loop1)
# Mount the partition
	@mkdir build/img
	@sudo mount $(loop1) build/img
	@sudo mkdir -p build/img/boot/grub
# Install GRUB and put kernel binary and grub config on the disk
	@sudo cp $(kernel) build/img/boot/kernel.bin
	@sudo cp $(grub_cfg) build/img/boot/grub
	@sudo grub-install --no-floppy --root-directory=build/img \
	--modules="normal part_msdos ext2 multiboot" $(loop0) 
	@sync
# Cleanup
	@sudo umount build/img
	@rmdir build/img
	@sudo losetup -d $(loop0)
	@sudo losetup -d $(loop1)

$(iso): $(kernel) $(grub_cfg)
	@mkdir -p build/isofiles/boot/grub
	@cp $(kernel) build/isofiles/boot/kernel.bin
	@cp $(grub_cfg) build/isofiles/boot/grub
	@grub-mkrescue -o $(iso) build/isofiles 2> /dev/null
	@rm -r build/isofiles

$(kernel): $(assembly_object_files) $(c_object_files) $(linker_script)
	@$(ld) -n -T $(linker_script) -o $(kernel) $(assembly_object_files) $(c_object_files) 

# compile assembly files
build/arch/$(arch)/%.o: src/arch/$(arch)/%.asm
	@mkdir -p $(shell dirname $@)
	@nasm -felf64 $< -o $@

# compile c files
build/arch/$(arch)/%.o: src/arch/$(arch)/%.c
	@mkdir -p $(shell dirname $@)
	@$(cc) $(cflags) -c $< -o $@