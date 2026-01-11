#pragma once
#include <efi/efi.h>


#define ST   SystemTable
#define BS   SystemTable->BootServices
#define RT   SystemTable->RuntimeServices
#define IH   ImageHandle

#define EFI_CHECK(x) \
    do { EFI_STATUS _s = (x); if (EFI_ERROR(_s)) return _s; } while (0)

#define UEFI_PRINT(str) \
    ST->ConOut->OutputString(ST->ConOut, (str))

extern EFI_GUID gEfiLoadedImageProtocolGuid;
extern EFI_GUID gEfiSimpleFileSystemProtocolGuid;

static EFI_STATUS
MountImageVolume(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable,
    EFI_FILE_PROTOCOL **Root
) {
    EFI_LOADED_IMAGE_PROTOCOL *Image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;

    /* Get Loaded Image Protocol */
    EFI_CHECK(BS->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&Image
    ));

    /* Get Simple FS from the image's device */
    EFI_CHECK(BS->HandleProtocol(
        Image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void **)&Fs
    ));

    /* Open root directory */
    return Fs->OpenVolume(Fs, Root);
}
