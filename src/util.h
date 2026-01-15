#include <efi/efi.h>
#include <efi/efidef.h>
#include <efi/efierr.h>
#include <efi/efiprot.h>
#include <efi/x86_64/efibind.h>
#include <efi/x86_64/efibind.h>

#ifndef UTIL_H
#define UTIL_H

static inline void ASCII_TO_CHAR16(char* src, CHAR16* dst, int len) { 
    if (len > 512) {len = 512;}

    for (UINTN i = 0; i < len; i++) {
        dst[i] = (CHAR16)src[i];
    }
}

#endif