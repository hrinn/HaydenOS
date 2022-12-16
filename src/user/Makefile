CFLAGS += -fpic

out_dir := ../../$(out_dir)
obj_dir := ../../$(obj_dir)

linker := linker.ld

$(out_dir)/init.bin: $(obj_dir)/init.o $(obj_dir)/syscall.o
	@$(LD) -n -T $(linker) -o $(out_dir)/img/bin/init.bin $(obj_dir)/init.o $(obj_dir)/syscall.o

$(obj_dir)/init.o: init.c
	@$(CC) $(CFLAGS) -c init.c -o $(obj_dir)/init.o