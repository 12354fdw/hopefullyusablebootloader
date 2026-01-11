all:
	gcc -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol \
    -fpic -fshort-wchar -fno-stack-protector -mno-red-zone \
    -DGNU_EFI_USE_MS_ABI -c src/efi.c -o build/efi.o

	ld -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds \
   -shared -Bsymbolic -L /usr/lib -l:libgnuefi.a -l:libefi.a \
   /usr/lib/crt0-efi-x86_64.o build/efi.o -o efi.so

	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
			-j .rel -j .rela -j .reloc --target=efi-app-x86_64 \
			efi.so BOOTX64.EFI

	dd if=/dev/zero of=uefi.img bs=1M count=64
	mkfs.fat -F32 uefi.img

	mkdir -p /tmp/uefi-mount
	sudo mount -o loop uefi.img /tmp/uefi-mount

	sudo mkdir -p /tmp/uefi-mount/EFI/BOOT
	sudo cp BOOTX64.EFI /tmp/uefi-mount/EFI/BOOT/BOOTX64.EFI

	sudo umount /tmp/uefi-mount
	rm -rf /tmp/uefi-mount




run:
	qemu-system-x86_64 \
	-drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd \
	-drive if=pflash,format=raw,file=OVMF_VARS.4m.fd \
	-drive file=uefi.img,format=raw \
	-m 256M \
	-net none \
	-display gtk
