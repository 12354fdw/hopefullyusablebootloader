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

__attribute__((noreturn))
void panic(EFI_SYSTEM_TABLE *ST, CHAR16 *reason) {
    ST->ConOut->SetAttribute(ST->ConOut,
        EFI_LIGHTRED | EFI_BACKGROUND_BLACK);

    ST->ConOut->OutputString(
        ST->ConOut,
        L"\r\nOH SHIT OH FUCK SOMETHING WENT WRONG\r\n"
    );

    ST->ConOut->OutputString(ST->ConOut, reason);
    ST->ConOut->OutputString(ST->ConOut, L"\r\n");

    // firmware-friendly halt
    while (1) {
        ST->BootServices->Stall(1000000);
    }
}

#define PANIC(msg) panic(ST, msg)

// GUIDs
// EFI Loaded Image Protocol GUID
EFI_GUID gEfiLoadedImageProtocolGuid = 
    {0x5B1B31A1, 0x9562, 0x11d2, {0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}};

// EFI Simple File System Protocol GUID
EFI_GUID gEfiSimpleFileSystemProtocolGuid =
    {0x0964E5B2, 0x6459, 0x11D2, {0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}};

static EFI_STATUS
MountImageVolume(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable,
    EFI_FILE_PROTOCOL **Root
) {
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *Image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;

    /* Get Loaded Image Protocol */
    EFI_CHECK(BS->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&Image
    ));

    /* Locate all Simple File System handles */
    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0;
    EFI_CHECK(BS->LocateHandleBuffer(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &Handles
    ));

    /* Iterate handles to find one that matches Image->DeviceHandle */
    for (UINTN i = 0; i < HandleCount; i++) {
        EFI_HANDLE h = Handles[i];
        Status = BS->HandleProtocol(
            h,
            &gEfiSimpleFileSystemProtocolGuid,
            (void **)&Fs
        );
        if (EFI_ERROR(Status)) continue;

        // Check if this FS is on the same device as the loaded image
        if (h == Image->DeviceHandle) {
            Status = Fs->OpenVolume(Fs, Root);
            BS->FreePool(Handles);
            return Status;
        }
    }

    // Fall back: nothing matched, just pick first FS
    if (HandleCount > 0) {
        panic(ST, L"Unable to mount ESP");
    }

    if (Handles) BS->FreePool(Handles);
    return Status;
}
