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

void BOOT_KERNEL_LBP(struct config conf, EFI_FILE_PROTOCOL *Root, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"Using path 'LBP'\r\n");
    EFI_STATUS Status;

    CHAR16 kernPathCHAR16[128];
    ASCII_TO_CHAR16(conf.kernelPath, kernPathCHAR16, strlena(conf.kernelPath));

    EFI_FILE_HANDLE kernFile;
    Status = uefi_call_wrapper(
        Root->Open,
        5,
        Root,
        &kernFile,
        kernPathCHAR16,
        EFI_FILE_MODE_READ,
        0
    );

    if (EFI_ERROR(Status)) PANIC(L"Unable to open kernFile");

    // allocate space for kernel image
    Print(L"loading kernel into memory\r\n");
    EFI_PHYSICAL_ADDRESS kernFileAddr = 0;

    EFI_FILE_INFO *info;
    UINTN infoSize = SIZE_OF_EFI_FILE_INFO + 256;

    uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, infoSize, (void **)&info);

    Status = uefi_call_wrapper(kernFile->GetInfo, 4,
        kernFile,
        &gEfiFileInfoGuid,
        &infoSize,
        info
    );

    if (EFI_ERROR(Status)) PANIC(L"kernFile getinfo failed");

    UINTN kernFileSize = info->FileSize;

    uefi_call_wrapper(BS->AllocatePages, 4,
        AllocateAnyPages,
        EfiLoaderCode,
        EFI_SIZE_TO_PAGES(kernFileSize),
        &kernFileAddr
    );

    Status = uefi_call_wrapper(kernFile->Read, 3, kernFile, &kernFileSize, kernFileAddr);
    if (EFI_ERROR(Status)) PANIC(L"Unable to read kernFile");

    struct setup_header *setupHeader = (struct setup_header *)((UINT8 *)kernFileAddr + SETUP_HEADER_OFFSET);

    // verify setup_header

    Print(L"verifying kernel setupHeader\r\n");
    if (setupHeader->boot_flag != 0xAA55) PANIC(L"Invalid kerenl: case 1");
    if (setupHeader->header[0] != 'H' || setupHeader->header[1] != 'd' || setupHeader->header[2] != 'r' || setupHeader->header[3] != 'S') PANIC(L"Invalid kernel: case 2");
    if (setupHeader->version < 0x020F) PANIC(L"boot protocol too old! (at least 2.15)");
    if (!(setupHeader->xloadflags & (1 << 0))) PANIC(L"kernel does not support 64 bit!");
    if (!(setupHeader->xloadflags & (1 << 5))) PANIC(L"kernel does not support EFI handover!");
    if (setupHeader->handover_offset == 0) PANIC(L"invalid kernel: case 3");

    // filling out boot params

    Print(L"allocating space for boot_params\r\n");

    EFI_PHYSICAL_ADDRESS bp_phys = 0;
    Status = uefi_call_wrapper(BS->AllocatePages, 4,
        AllocateAnyPages,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(sizeof(struct boot_params)),
        &bp_phys
    );

    if (EFI_ERROR(Status)) PANIC(L"boot params alloc fail");

    Print(L"filling out boot_params\r\n");

    struct boot_params *bp = (struct boot_params *)(UINTN)bp_phys;
    memset(bp,0,sizeof(*bp));

    memcpy(&bp->hdr, setupHeader, sizeof(struct setup_header));

    bp->hdr.type_of_loader = 0xFF;


    Print(L"allocating space for cmdline\r\n");

    UINTN cmdline_len = strlena(conf.cmdline) + 1;
    EFI_PHYSICAL_ADDRESS cmdline_phys;

    Status = uefi_call_wrapper(BS->AllocatePages, 4,
        AllocateAnyPages,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(cmdline_len),
        &cmdline_phys
    );

    if (EFI_ERROR(Status)) PANIC(L"cmdline alloc failed");

    Print(L"filling out cmdline\r\n");
    memcpy((void *)(UINTN)cmdline_phys, conf.cmdline, cmdline_len);

    bp->hdr.cmd_line_ptr = (UINT32)(UINTN)cmdline_phys;
    bp->hdr.cmdline_size = cmdline_len;

    EFI_MEMORY_DESCRIPTOR *memmap = NULL;
    UINTN memmap_size = 0, map_key, desc_size;
    UINT32 desc_version;

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5,
        &memmap_size,
        memmap,
        &map_key,
        &desc_size,
        &desc_version
    );

    if (Status != EFI_BUFFER_TOO_SMALL) PANIC(L"GetMemoryMap size query failed");

    memmap_size += desc_size * 2;

    Status = uefi_call_wrapper(BS->AllocatePool, 3,
        EfiLoaderData,
        memmap_size,
        (void **)&memmap
    );

    if (EFI_ERROR(Status)) PANIC(L"memmap alloc failed");

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5,
        &memmap_size,
        memmap,
        &map_key,
        &desc_size,
        &desc_version
    );

    if (EFI_ERROR(Status)) PANIC(L"GetMemoryMap failed");

    Status = uefi_call_wrapper(BS->ExitBootServices, 2,
        ImageHandle,
        map_key
    );

    if (EFI_ERROR(Status)) PANIC(L"ExitBootServices failed");


    bp->efi_info.efi_loader_signature = 0x34364C45; // "EL64"
    bp->efi_info.efi_systab           = (UINT32)(uintptr_t)SystemTable;
    bp->efi_info.efi_systab_hi        = (UINT32)((uintptr_t)SystemTable >> 32);

    bp->efi_info.efi_memmap           = (UINT32)(uintptr_t)memmap;
    bp->efi_info.efi_memmap_hi        = (UINT32)((uintptr_t)memmap >> 32);
    bp->efi_info.efi_memmap_size      = memmap_size;
    bp->efi_info.efi_memdesc_size     = desc_size;
    bp->efi_info.efi_memdesc_version  = desc_version;


    __asm__ volatile ("cli");
    
    typedef void (__attribute__((ms_abi)) *efi_handover_t)(
        EFI_SYSTEM_TABLE *,
        struct boot_params *
    );


    efi_handover_t handover =
        (efi_handover_t)((UINT8 *)kernFileAddr + setupHeader->handover_offset);

    __asm__ volatile (
        "andq $~0xF, %%rsp\n"
        "subq $32, %%rsp\n"  // shadow space
        ::: "rsp"
    );


    handover(SystemTable, bp);


}


// not used
void BOOT_KERNEL_EFI(struct config conf, EFI_FILE_PROTOCOL *Root, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"Using path 'EFI'");
    EFI_STATUS Status;

    CHAR16 kernPath[128];

    ASCII_TO_CHAR16(conf.kernelPath, kernPath, 128);


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

    CHAR16 cmdline[] = L"earlycon earlyprintk=efi loglevel=7 root=/dev/sda rw";

    Loaded->LoadOptions = cmdline;
    Loaded->LoadOptionsSize =
        (StrLen(cmdline) + 1) * sizeof(CHAR16);

    Status = BS->StartImage(kernHandle, NULL, NULL);
}
