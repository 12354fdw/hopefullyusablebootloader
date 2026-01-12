#pragma once
#include <efi/efi.h>
#include <efi/efidef.h>
#include <efi/efierr.h>
#include <efi/efiprot.h>
#include <efi/x86_64/efibind.h>


#define ST   SystemTable
#define BS   SystemTable->BootServices
#define RT   SystemTable->RuntimeServices
#define IH   ImageHandle


#define EFI_CHECK(x) \
    do { EFI_STATUS _s = (x); if (EFI_ERROR(_s)) return _s; } while (0)

#define UEFI_PRINT(str) \
    ST->ConOut->OutputString(ST->ConOut, (str))

void UEFI_PRINT_ASCII(EFI_SYSTEM_TABLE* SystemTable, UINT8* str, UINTN len) {
    CHAR16 destBuffer[513];
    if (len > 512) {len = 512;}

    for (UINTN i = 0; i < len; i++) {
        destBuffer[i] = (CHAR16)str[i];
    }
    destBuffer[len] = L'\0';
    SystemTable->ConOut->OutputString(SystemTable->ConOut, destBuffer);

}

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

    while (1) {
        ST->BootServices->Stall(1000000);
    }
}

#define PANIC(msg) panic(ST, msg)

// GUIDs
EFI_GUID gEfiLoadedImageProtocolGuid =  EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

static EFI_STATUS
MountImageVolume(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable,
    EFI_FILE_PROTOCOL **Root
) {

    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

    EFI_STATUS Status;

    Status = SystemTable->BootServices->OpenProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&LoadedImage,
        ImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to get loadedImage");
    }


    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SFSP;

    BS->HandleProtocol(
    LoadedImage->DeviceHandle,
    &gEfiSimpleFileSystemProtocolGuid,
    (void**)&SFSP
    );
    
    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to get SFSP");
    }

    Status = uefi_call_wrapper(
        SFSP->OpenVolume,
        2,
        SFSP,
        Root
    );


    return EFI_SUCCESS;
}

