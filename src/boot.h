#include "uefi.h"
#include "lbp.h"

#include "util.h"
#include <efi/efi.h>
#include <efi/efidef.h>
#include <efi/efierr.h>
#include <efi/efilib.h>
#include <efi/x86_64/efibind.h>

struct config {
    BOOLEAN instantBoot;
    char kernelPath[128];
    char initrdPath[128];
    char cmdline[128];
    char shellPath[128];
};

struct config parseConfig(EFI_SYSTEM_TABLE *SystemTable,UINT8* buffer, UINTN len) {
    struct config conf = {};

    // byte walking parsing
    /*
        0 = kernelPath
        1 = initrdPath
        2 = cmdline
        3 = shellpath
    */
    int parsingTarget=0;

    char parsingStr[128];
    SetMem(parsingStr, sizeof(parsingStr), 0);
    int ptr = 0;

    if (buffer[0]=='Y') {
        conf.instantBoot = TRUE;
    } else {
        conf.instantBoot = FALSE;
    }

    UINTN i = 0;

    while (i < len && buffer[i] != '\n') i++;
    if (i < len) i++;

    for (; i < len; i++) {
        UINT8 c = buffer[i];

        if (c == '\r') continue;

        if (c == '\n') {
            if (parsingTarget >= 4) PANIC(L"Too many lines");

            parsingStr[ptr] = '\0';

            if (parsingTarget == 0) memcpy(conf.kernelPath, parsingStr, ptr + 1);
            if (parsingTarget == 1) memcpy(conf.initrdPath, parsingStr, ptr + 1);
            if (parsingTarget == 2) memcpy(conf.cmdline, parsingStr, ptr + 1);
            if (parsingTarget == 3) memcpy(conf.shellPath, parsingStr, ptr + 1);

            ptr = 0;
            //Print(L"%a\r\n",parsingStr);
            SetMem(parsingStr, sizeof(parsingStr), 0);
            parsingTarget++;
            continue;
        }

        if (ptr >= sizeof(parsingStr) - 1)
            PANIC(L"prevented oob on parseConfig!");

        parsingStr[ptr++] = (char)c;
    }


    return conf;
}

#include "boot_lbp.h"


void BOOT_KERNEL_EFI(struct config conf, EFI_FILE_PROTOCOL *Root, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"Using path 'EFI'\r\n");
    EFI_STATUS Status;

    CHAR16 kernPath[128];

    AsciiToChar16(conf.kernelPath, kernPath, 128);


    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

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

    EFI_DEVICE_PATH_PROTOCOL* kernDevPath = FileDevicePath(LoadedImage->DeviceHandle, kernPath);

    EFI_HANDLE kernHandle = NULL;
    Status = uefi_call_wrapper(BS->LoadImage, 6,
        FALSE,
        ImageHandle,
        kernDevPath,
        NULL,
        0,
        &kernHandle
    );

    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to load kernel");
    }

    EFI_LOADED_IMAGE_PROTOCOL *Loaded;

    BS->HandleProtocol(
        kernHandle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&Loaded
    );

    CHAR16* cmdline = AllocatePool(256 * sizeof(CHAR16));
    StrCpy(cmdline,
        L"earlycon=efi earlyprintk=efi loglevel=8");

    UINTN cmdline_len = StrLen(cmdline) + 1;

    Loaded->LoadOptions = cmdline;
    Loaded->LoadOptionsSize = cmdline_len * sizeof(CHAR16);



    Status = BS->StartImage(kernHandle, NULL, NULL);

    Print(L"Kernel returned: %016lx\r\n", (UINT64)Status);
    for (;;); // stop firmware from continuing

}
