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
    InitializeLib(ImageHandle, SystemTable);

    EFI_STATUS Status;


    // mount ESP
    Print(L"Attempting to mount ESP\r\n");

    EFI_FILE_HANDLE Root = mountESP(ImageHandle);
    if (!Root) {
        PANIC(L"Unable to mount ESP");
    }
    Print(L"Mounted ESP\r\n");

    // open config
    Print(L"Attempting to open config\r\n");
    EFI_FILE_HANDLE config;
    Status = uefi_call_wrapper(
        Root->Open,
        5,
        Root,
        &config,
        L"\\config.txt",
        EFI_FILE_MODE_READ,
        0
    );


    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to open config!");
    }

    Print(L"Opened config\r\n");
    
    uefi_call_wrapper(config->SetPosition, 2, config, 0);

    UINTN BufferSize = 512;
    UINT8 Buffer[512];

    SetMem(Buffer,512,0);

    Status = uefi_call_wrapper(config->Read, 3, config, &BufferSize, Buffer);

    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to read config");
    }
    Print(L"Read config\r\n");

    struct config conf = parseConfig(SystemTable,Buffer, BufferSize);
    Print(L"Parsed config\r\n\r\n");

    // config dump
    Print(L"dumping config\r\n");

    if (conf.instantBoot) {
        Print(L"instantBoot=TRUE\r\n");
    } else {
        Print(L"instantBoot=FALSE\r\n");
    }

    Print(L"kernelPath=%a\r\n", conf.kernelPath);
    Print(L"initrdPath=%a\r\n", conf.initrdPath);
    Print(L"cmdline=%a\r\n", conf.cmdline);
    Print(L"shellPath=%a\r\n", conf.shellPath);

    
    EFI_INPUT_KEY key;
    UINTN index;
    
    Print(L"\r\n\r\nBOOTING\r\n");
    BOOT_KERNEL_LBP(conf, Root, ImageHandle, SystemTable);
    Print(L"You are not supposed to be here \r\n");

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
