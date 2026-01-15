// i love freestanding
#include "uefi.h"

#ifndef STD_H
#define STD_H

void* memcpy(void* dest,const void* src, size_t len) {
    char* destPtr = (char*)dest;
    char* srcPtr = (char*)src;

    for (size_t i=0;i<len;i++) {
        destPtr[i]=srcPtr[i];
    }
    return dest;
}

void* memset(void* s, int c, size_t len) {
    unsigned char* p = (unsigned char*)s;
    while (len--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

static UINTN StrLen16(const CHAR16 *s) {
    UINTN n = 0;
    while (s[n])
        n++;
    return n;
}

#endif