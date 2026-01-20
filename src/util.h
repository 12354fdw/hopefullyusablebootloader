#include <efi/efi.h>
#include <efi/efilib.h>
#include "uefi.h"

#ifndef UTIL_H
#define UTIL_H

static inline void AsciiToChar16(const char *src, CHAR16 *dst, UINTN dst_len) {
    UINTN i = 0;

    if (!dst || dst_len == 0)
        return;

    while (src[i] && i < dst_len - 1) {
        dst[i] = (CHAR16)(UINT8)src[i];
        i++;
    }

    dst[i] = 0;
}

static inline EFI_FILE_HANDLE mountESP(EFI_HANDLE ImageHandle) {
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
    EFI_FILE_HANDLE Root;

    Status = uefi_call_wrapper(
        BS->HandleProtocol,
        3,
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&LoadedImage
    );
    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to get loadedImage");
    }

    Status = uefi_call_wrapper(
        BS->HandleProtocol,
        3,
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&Fs
    );
    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to get SFSP");
    }

    Status = uefi_call_wrapper(
        Fs->OpenVolume,
        2,
        Fs,
        &Root
    );
    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to open volume");
    }

    return Root;
}

static inline void* memcpy(void* dest,const void* src, size_t len) {
    char* destPtr = (char*)dest;
    char* srcPtr = (char*)src;

    for (size_t i=0;i<len;i++) {
        destPtr[i]=srcPtr[i];
    }
    return dest;
}

static inline void* memset(void* s, int c, size_t len) {
    unsigned char* p = (unsigned char*)s;
    while (len--) {
        *p++ = (unsigned char)c;
    }
    return s;
}


static inline void outb(UINT16 port, UINT8 val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}

static void serial_putc(char c) {
    outb(0x3F8, c);
}

static void serial_print(const char *s) {
    while (*s) serial_putc(*s++);
}


static void serial_print_u64(UINT64 v, int base) {
    char buf[32];
    int i = 0;

    if (v == 0) {
        serial_putc('0');
        return;
    }

    while (v) {
        int d = v % base;
        buf[i++] = (d < 10) ? '0' + d : 'a' + (d - 10);
        v /= base;
    }

    while (i--)
        serial_putc(buf[i]);
}

static void serial_print_i64(INT64 v) {
    if (v < 0) {
        serial_putc('-');
        v = -v;
    }
    serial_print_u64((UINT64)v, 10);
}

#include <stdarg.h>

static void serial_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            serial_putc(*fmt++);
            continue;
        }

        fmt++;  // skip '%'

        switch (*fmt) {
        case '%':
            serial_putc('%');
            break;

        case 'c':
            serial_putc((char)va_arg(ap, int));
            break;

        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            serial_print(s);
            break;
        }

        case 'd':
        case 'i':
            serial_print_i64(va_arg(ap, INT64));
            break;

        case 'u':
            serial_print_u64(va_arg(ap, UINT64), 10);
            break;

        case 'x':
            serial_print_u64(va_arg(ap, UINT64), 16);
            break;

        case 'p':
            serial_print("0x");
            serial_print_u64((UINT64)va_arg(ap, void *), 16);
            break;

        default:
            serial_print("[?]");
            break;
        }

        fmt++;
    }

    va_end(ap);
}


#endif