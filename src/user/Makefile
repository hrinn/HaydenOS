linker := linker.ld
build_dir := ../../bin

$(build_dir)/init.bin: $(build_dir)/init.o $(build_dir)/sys_call_ints.o
	@$(LD) -n -T $(linker) -o $(build_dir)/init.bin $(build_dir)/init.o $(build_dir)/sys_call_ints.o

$(build_dir)/init.o: init.c
	@$(CC) $(CFLAGS) -fpic -c init.c -o $(build_dir)/init.o