#include "uefi.h"
#include "lbp.h"

#include "util.h"
#include <efi/efi.h>
#include <efi/efidef.h>
#include <efi/efierr.h>
#include <efi/efilib.h>
#include <efi/x86_64/efibind.h>

#ifndef CONFIG_H
#define CONFIG_H

struct config {
    BOOLEAN instantBoot;
    char kernelPath[128];
    char initrdPath[128];
    char cmdline[128];
    char shellPath[128];
};

static inline struct config parseConfig(EFI_SYSTEM_TABLE *SystemTable,UINT8* buffer, UINTN len) {
    struct config conf = {};

    // byte walking parsing
    /*
        0 = kernelPath
        1 = initrdPath
        2 = cmdline
        3 = shellpath
    */
    int parsingTarget=0;

    char parsingStr[128];
    SetMem(parsingStr, sizeof(parsingStr), 0);
    int ptr = 0;

    if (buffer[0]=='Y') {
        conf.instantBoot = TRUE;
    } else {
        conf.instantBoot = FALSE;
    }

    UINTN i = 0;

    while (i < len && buffer[i] != '\n') i++;
    if (i < len) i++;

    for (; i < len; i++) {
        UINT8 c = buffer[i];

        if (c == '\r') continue;

        if (c == '\n') {
            if (parsingTarget >= 4) PANIC(L"Too many lines");

            parsingStr[ptr] = '\0';

            if (parsingTarget == 0) memcpy(conf.kernelPath, parsingStr, ptr + 1);
            if (parsingTarget == 1) memcpy(conf.initrdPath, parsingStr, ptr + 1);
            if (parsingTarget == 2) memcpy(conf.cmdline, parsingStr, ptr + 1);
            if (parsingTarget == 3) memcpy(conf.shellPath, parsingStr, ptr + 1);

            ptr = 0;
            //Print(L"%a\r\n",parsingStr);
            SetMem(parsingStr, sizeof(parsingStr), 0);
            parsingTarget++;
            continue;
        }

        if (ptr >= sizeof(parsingStr) - 1)
            PANIC(L"prevented oob on parseConfig!");

        parsingStr[ptr++] = (char)c;
    }


    return conf;
}

#endif