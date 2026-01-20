DEV?=/dev/sda

all: format

format: compile
	dd if=/dev/zero of=uefi.img bs=1M count=512
	mkfs.fat -F32 uefi.img

	mkdir -p /tmp/uefi-mount
	sudo mount -o loop uefi.img /tmp/uefi-mount

	sudo mkdir -p /tmp/uefi-mount/EFI/BOOT
	sudo cp BOOTX64.EFI /tmp/uefi-mount/EFI/BOOT/BOOTX64.EFI
	sudo cp config /tmp/uefi-mount/config.txt
#	sudo cp /home/bob/Documents/linux/arch/x86/boot/bzImage /tmp/uefi-mount/kern.kern
	sudo cp /boot/vmlinuz-linux-zen /tmp/uefi-mount/kern.kern
#	sudo cp /boot/initramfs-linux-zen.img /tmp/uefi-mount/initrd.img

	sudo umount /tmp/uefi-mount
	rm -rf /tmp/uefi-mount

compile:
	clang \
		-target x86_64-unknown-linux-gnu \
		-ffreestanding \
		-fno-stack-protector \
		-fno-stack-check \
		-fno-pic \
		-fPIC \
		-mno-red-zone \
		-fshort-wchar \
		-I/usr/include \
		-I/usr/include/efi \
		-I/usr/include/efi/x86_64 \
		-c src/efi.c -o build/efi.o \
		-g -O0

	ld.lld \
		-shared \
		-Bsymbolic \
		-nostdlib \
		-znocombreloc \
		-T /usr/lib/elf_x86_64_efi.lds \
		/usr/lib/crt0-efi-x86_64.o \
		build/efi.o \
		/usr/lib/libgnuefi.a \
		/usr/lib/libefi.a \
		-o build/efi.so

	objcopy \
		-j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc \
		--target=efi-app-x86_64 \
		--subsystem=10 \
		build/efi.so BOOTX64.EFI

burn: all
	@echo "/!\ YOU ARE ABOUT TO WRITE uefi.img TO $(DEV) /!\\"
	@echo "    You can specify which device to burn to via DEV=/dev/target"
	@read -p "Type YES to continue: " ans; \
	if [ "$$ans" != "YES" ]; then \
		echo "Aborted."; \
		exit 1; \
	fi
	sudo dd if=uefi.img of=$(DEV) status=progress conv=fsync


run: all
	sudo cp /usr/share/edk2/x64/OVMF_VARS.4m.fd OVMF_VARS.4m.fd
	qemu-system-x86_64 \
	-drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd \
	-drive if=pflash,format=raw,file=OVMF_VARS.4m.fd \
	-drive file=uefi.img,format=raw \
	-serial stdio \
	-m 4G \
	-net none \
	-display gtk

copy: all
	sudo cp BOOTX64.EFI /boot/BOOTX64.EFI