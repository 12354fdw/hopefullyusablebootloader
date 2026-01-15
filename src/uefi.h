#pragma once
#include <efi/efi.h>
#include <efi/efilib.h>


#define EFI_CHECK(x) \
    do { EFI_STATUS _s = (x); if (EFI_ERROR(_s)) return _s; } while (0)

void PANIC(CHAR16 *reason) {
    ST->ConOut->SetAttribute(ST->ConOut,
        EFI_LIGHTRED | EFI_BACKGROUND_BLACK);


    Print(L"\r\nOH SHIT OH FUCK SOMETHING WENT WRONG (REBOOT TO RESET)\r\n %s \r\n",reason);

    while (1) {
        ST->BootServices->Stall(1000000);
    }
}
    
__attribute__((noreturn))
void warn(EFI_SYSTEM_TABLE *ST, CHAR16 *reason) {
    ST->ConOut->SetAttribute(ST->ConOut,
        EFI_YELLOW | EFI_BACKGROUND_BLACK);

    ST->ConOut->OutputString(
        ST->ConOut,
        L"\r\nOH SHIT OH FUCK SOMETHING ALMOST WENT WRONG\r\n"
    );

    ST->ConOut->OutputString(ST->ConOut, reason);
    ST->ConOut->OutputString(ST->ConOut, L"\r\n");

    ST->ConOut->SetAttribute(ST->ConOut,
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);

    ST->BootServices->Stall(1000000);
}

#define WARN(msg) warn(ST, msg)

// GUIDs

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

