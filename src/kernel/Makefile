CFLAGS += -mno-red-zone -fomit-frame-pointer -fpic -fno-pie -mcmodel=kernel -Iinclude

out_dir := ../../$(out_dir)
obj_dir := ../../$(obj_dir)

linker := linker.ld
kernel := $(out_dir)/img/boot/kernel.bin

sub_dirs := arch boot dev fs irq lib mem proc test

c_srcs := $(wildcard *.c $(foreach fd, $(sub_dirs), $(fd)/*.c))
c_objs := $(addprefix $(obj_dir)/, $(c_srcs:c=o))

asm_srcs := $(wildcard *.asm $(foreach fd, $(sub_dirs), $(fd)/*.asm))
asm_objs := $(addprefix $(obj_dir)/, $(asm_srcs:asm=o))

$(kernel): $(asm_objs) $(c_objs) $(linker)
	@$(LD) -n -T $(linker) -o $(kernel) $(c_objs) $(asm_objs)

$(obj_dir)/%.o: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@

$(obj_dir)/%.o: %.asm
	@mkdir -p $(@D)
	@nasm -felf64 $< -o $@

echo:
	@echo "c_srcs: $(c_srcs)"
	@echo "c_objs: $(c_objs)"
	@echo "asm_srcs: $(asm_srcs)"
	@echo "asm_objs: $(asm_objs)"