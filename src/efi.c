#include "uefi.h"
#include "boot.h"
#include <efi/efidef.h>
#include <efi/efierr.h>
#include <efi/efiprot.h>
#include <efi/x86_64/efibind.h>

EFI_STATUS EFIAPI efi_main(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
) {
    EFI_STATUS Status;

    UEFI_PRINT(L"Attempting to mount ESP\r\n");

    EFI_FILE_PROTOCOL *Root;
    MountImageVolume(ImageHandle, SystemTable, &Root);
    UEFI_PRINT(L"Mounted ESP\r\n");

    UEFI_PRINT(L"Attempting to open config (if hangs config.txt does not exists) \r\n");
    EFI_FILE_HANDLE config;
    Status = Root->Open(Root, &config, L"\\config.txt",EFI_FILE_MODE_READ,0);

    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to open config!");
    }

    UEFI_PRINT(L"Opened config\r\n");

    config->SetPosition(config, 0);

    UINTN BufferSize = 512;
    UINT8 Buffer[512];

    memset(Buffer,0,512);

    Status = config->Read(config, &BufferSize, Buffer);
    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to read config");
    }
    UEFI_PRINT(L"Read config\r\n");

    struct config conf = parseConfig(SystemTable,Buffer, 512);
    UEFI_PRINT(L"Parsed config\r\n\r\n");

    // config dump
    UEFI_PRINT(L"dumping config\r\n");

    if (conf.instantBoot) {
        UEFI_PRINT(L"instantBoot=TRUE\r\n");
    } else {
        UEFI_PRINT(L"instantBoot=FALSE\r\n");
    }

    UEFI_PRINT(L"kernelPath=");
    UEFI_PRINT_ASCII(SystemTable, conf.kernelPath,128);
    UEFI_PRINT(L"\r\n");

    UEFI_PRINT(L"initrdPath=");
    UEFI_PRINT_ASCII(SystemTable, conf.initrdPath,128);
    UEFI_PRINT(L"\r\n");

    UEFI_PRINT(L"cmdline=");
    UEFI_PRINT_ASCII(SystemTable, conf.cmdline,128);
    UEFI_PRINT(L"\r\n");

    UEFI_PRINT(L"shellPath=");
    UEFI_PRINT_ASCII(SystemTable, conf.shellPath,128);
    UEFI_PRINT(L"\r\n");

    
    EFI_INPUT_KEY key;
    UINTN index;
    
    UEFI_PRINT(L"\r\n\r\nBOOTING\r\n");
    BOOT_KERNEL_EFI(conf, Root, ImageHandle, SystemTable);
    UEFI_PRINT(L"kernel finished");

    // WAIT FOR KEY
    SystemTable->BootServices->WaitForEvent(
        1,
        &SystemTable->ConIn->WaitForKey,
        &index
    );

    // Read the key (required after WaitForKey)
    SystemTable->ConIn->ReadKeyStroke(
        SystemTable->ConIn,
        &key
    );

    //EFI_FILE_HANDLE KernelFile;
    //Status = Root->Ope

    //PANIC(L"WIP");

    // shutdown
    //config->Close(config);
    //Root->Close(Root);

    //SystemTable->BootServices->Stall(5 * 1000 * 1000);
    return EFI_SUCCESS;
}
