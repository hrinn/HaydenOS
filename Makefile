loader := build/img/boot/loader.bin
kernel := build/img/boot/kernel.bin
init := build/init.bin
build_dir := build

img := build/HaydenOS.img

.PHONY: clean run

$(img): $(build_dir) $(kernel) $(loader) $(init)
	@tools/img.sh

clean:
	@rm -r build

run: $(img)
	@qemu-system-x86_64 -s -drive format=raw,file=$(img) -serial stdio

$(loader):
	$(MAKE) -C src/boot

$(kernel):
	$(MAKE) -C src/kernel

$(init):
	$(MAKE) -C src/user

$(build_dir):
	@mkdir -p build
