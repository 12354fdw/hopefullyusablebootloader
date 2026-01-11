all:
	clang \
	-target x86_64-windows \
	-ffreestanding \
	-fno-stack-protector \
	-mno-red-zone \
	-I/usr/include/ \
	-I/usr/include/efi/x86_64 \
	-c src/efi.c -o build/efi.obj

	lld-link \
		/subsystem:efi_application \
		/entry:efi_main \
		/nodefaultlib \
		/out:BOOTX64.EFI \
		/libpath:/usr/lib/gnu-efi \
		build/efi.obj

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
