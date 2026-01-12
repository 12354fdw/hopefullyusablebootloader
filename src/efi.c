#include "uefi.h"
#include <efi/efidef.h>
#include <efi/efierr.h>
#include <efi/efiprot.h>
#include <efi/x86_64/efibind.h>

EFI_STATUS EFIAPI efi_main(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
) {

    EFI_STATUS Status;

    EFI_INPUT_KEY key;
    UINTN index;

    UEFI_PRINT(L"Attempting to mount ESP\r\n");

    EFI_FILE_PROTOCOL *Root;
    MountImageVolume(ImageHandle, SystemTable, &Root);
    UEFI_PRINT(L"Mounted ESP\r\n");

    EFI_FILE_HANDLE config;
    Status = Root->Open(Root, &config, L"\\config.txt",EFI_FILE_MODE_READ,0);

    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to open config!");
    }
    UEFI_PRINT(L"Opened config\r\n");

    config->SetPosition(config, 0);

    UINTN BufferSize = 512;
    UINT8 Buffer[512];

    Status = config->Read(config, &BufferSize, Buffer);
    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to read config");
    }
    UEFI_PRINT(L"Read config\r\n");

    CHAR16 UnicodeDest[513];

    // Manual conversion loop
    for (UINTN i = 0; i < BufferSize; i++) {
        UnicodeDest[i] = (CHAR16)Buffer[i]; // Zero-extends 8-bit to 16-bit
    }
    UnicodeDest[BufferSize] = L'\0';
    UEFI_PRINT(L"dumping config\r\n");
    UEFI_PRINT(UnicodeDest);
    UEFI_PRINT(L"\rEOF\r\n\r\n");

    
    

    // shutdown
    config->Close(config);
    Root->Close(Root);


    for (;;);

    //SystemTable->BootServices->Stall(5 * 1000 * 1000);
    return EFI_SUCCESS;
}
