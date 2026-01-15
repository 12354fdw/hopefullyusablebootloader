#include <efi/efi.h>
#include <efi/efilib.h>
#include "uefi.h"

#ifndef UTIL_H
#define UTIL_H

static inline void ASCII_TO_CHAR16(char* src, CHAR16* dst, int len) { 
    if (len > 512) {len = 512;}

    for (UINTN i = 0; i < len; i++) {
        dst[i] = (CHAR16)src[i];
    }
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


#endif