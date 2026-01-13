#include "uefi.h"
#include "std.h"

typedef enum {
    KERNEL_UEFI,
    KERNEL_VMLINUX,
    KERNEL_VMLINUZ
} KERNEL_TYPE;

typedef struct config {
    BOOLEAN instantBoot;
    char kernelPath[128];
    char initrdPath[128];
    char cmdline[128];
    char shellPath[128];
} config;

config parseConfig(UINT8* buffer, UINTN len) {
    config conf = {};

    // byte walking parsing
    /*
        0 = kernelPath
        1 = initrdPath
        2 = cmdline
        3 = shellpath
    */
    int parsingTarget=0;

    UINT8 parsingStr[128];
    int ptr;

    if (buffer[0]=='Y') {
        conf.instantBoot = TRUE;
    } else {
        conf.instantBoot = FALSE;
    }

    for (UINT8 i=2;i<len;i++) {
        UINT8 c = buffer[i];

        // handle new line
        // a manditory new line at the end is needed lol (this is not a bug)
        if (c == '\n') {

            ptr++;
            parsingStr[ptr] == '\0';

            if (parsingTarget==0) memcpy(conf.kernelPath,parsingStr,ptr);
            if (parsingTarget==1) memcpy(conf.initrdPath,parsingStr,ptr);
            if (parsingTarget==2) memcpy(conf.cmdline,parsingStr,ptr);
            if (parsingTarget==3) memcpy(conf.shellPath,parsingStr,ptr);

            ptr=0;
            memset(parsingStr,0,128);
            parsingTarget++;
            continue;
        }
        parsingStr[ptr] = c;
        ptr++;
    }

    return conf;
}

KERNEL_TYPE detectKernel(UINT8 *buf) {
    if (buf[0] == 'M' && buf[1] == 'Z')
        return KERNEL_UEFI;

    if (buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F') {return KERNEL_VMLINUX;}

    // if none assume vmlinuz
    return KERNEL_VMLINUZ;
}

void BOOT_KERNEL_UEFI() {
    
}
