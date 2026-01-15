#include "uefi.h"
#include "std.h"
#include "util.h"
#include <efi/efidef.h>
#include <efi/efidevp.h>
#include <efi/efiprot.h>
#include <efi/x86_64/efibind.h>

#include "devPath.h"

typedef struct config {
    BOOLEAN instantBoot;
    char kernelPath[128];
    char initrdPath[128];
    char cmdline[128];
    char shellPath[128];
} config;

config parseConfig(EFI_SYSTEM_TABLE *SystemTable,UINT8* buffer, UINTN len) {
    config conf = {};

    // byte walking parsing
    /*
        0 = kernelPath
        1 = initrdPath
        2 = cmdline
        3 = shellpath
    */
    int parsingTarget=0;

    UINT8 parsingStr[128];
    memset(parsingStr, 0, sizeof(parsingStr));
    int ptr = 0;

    if (buffer[0]=='Y') {
        conf.instantBoot = TRUE;
    } else {
        conf.instantBoot = FALSE;
    }

    for (UINTN i=2;i<len;i++) {
        UINT8 c = buffer[i];

        // handle new line
        // a manditory new line at the end is needed lol (this is not a bug)

        if (c == '\r') continue;
        if (c == '\n') {

            parsingStr[ptr] = '\0';
            ptr++;
            if (parsingTarget==0) memcpy(conf.kernelPath,parsingStr,ptr);
            if (parsingTarget==1) memcpy(conf.initrdPath,parsingStr,ptr);
            if (parsingTarget==2) memcpy(conf.cmdline,parsingStr,ptr);
            if (parsingTarget==3) memcpy(conf.shellPath,parsingStr,ptr);

            ptr=0;
            memset(parsingStr,0,128);
            parsingTarget++;
            continue;
        }

        if (c == 0) continue;
        if (ptr > sizeof(parsingStr) - 1) PANIC(L"prevented oob on parseConfig!");
        parsingStr[ptr] = c;
        ptr++;
    }

    return conf;
}

void BOOT_KERNEL_EFI(struct config conf, EFI_FILE_PROTOCOL *Root, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;

    CHAR16 kernPath[128];

    ASCII_TO_CHAR16(conf.kernelPath, kernPath, 128);
    EFI_DEVICE_PATH_PROTOCOL* kernDevPath = MakeFileDevicePath(ImageHandle, SystemTable, L"EFI\\BOOT\\BOOTX64.EFI");

    EFI_HANDLE kernHandle = NULL;
    CHAR16 *CmdLine;
    CONST CHAR16 *CmdTemplate =
        L"root=/dev/sda rw "
        L"earlycon=efifb "
        L"earlyprintk=efi "
        L"debug";

    UINTN CmdSize = (StrLen16(CmdTemplate) + 1) * sizeof(CHAR16);

    Status = BS->AllocatePool(
        EfiBootServicesData,
        CmdSize,
        (void **)&CmdLine
    );

    if (EFI_ERROR(Status)) {
        PANIC(L"AllocatePool failed");
    }

    memcpy(CmdLine, CmdTemplate, CmdSize);
    
    Status = BS->LoadImage(
        FALSE,
        ImageHandle,
        kernDevPath,
        NULL,
        0,
        &kernHandle
    );

    EFI_LOADED_IMAGE_PROTOCOL *Loaded;

    BS->HandleProtocol(
        kernHandle,
        &gEfiLoadedImageProtocolGuid,
        (void **)&Loaded
    );

    Loaded->LoadOptions     = CmdLine;
    Loaded->LoadOptionsSize = CmdSize;


    if (EFI_ERROR(Status)) {
        PANIC(L"Unable to load kernel");
    }


    Status = BS->StartImage(kernHandle, NULL, NULL);
}
