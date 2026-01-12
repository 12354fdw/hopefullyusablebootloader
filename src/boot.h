#include "uefi.h"

typedef enum {
    KERNEL_UEFI,
    KERNEL_VMLINUX,
    KERNEL_VMLINUZ
} KERNEL_TYPE;

KERNEL_TYPE detect_kernel(UINT8 *buf) {
    if (buf[0] == 'M' && buf[1] == 'Z')
        return KERNEL_UEFI;

    if (buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F') {return KERNEL_VMLINUX;}

    // if none assume vmlinuz
    return KERNEL_VMLINUZ;
}

void BOOT_KERNEL_UEFI() {
    
}