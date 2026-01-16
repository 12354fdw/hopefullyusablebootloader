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
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(kernFileSize),
        &kernFileAddr
    );

    Status = uefi_call_wrapper(kernFile->Read, 3, kernFile, &kernFileSize, kernFileAddr);
    if (EFI_ERROR(Status)) PANIC(L"Unable to read kernFile");

    struct setup_header *setupHeader = (struct setup_header *)((UINT8 *)kernFileAddr + SETUP_HEADER_OFFSET);

    // verify setup_header

    Print(L"verifying kernel setupHeader\r\n");
    if (setupHeader->boot_flag != 0xAA55) PANIC(L"setupHeader->boot_flag != 0xAA55");
    if (setupHeader->header[0] != 'H' || setupHeader->header[1] != 'd' || setupHeader->header[2] != 'r' || setupHeader->header[3] != 'S') PANIC(L"setupHeader->header != \"HdrS\"");
    if (setupHeader->version < 0x020B) PANIC(L"kernel too old!");
    if (!(setupHeader->xloadflags & (1 << 0))) PANIC(L"kernel does not support 64 bit!");

    UINT16 *mz = (UINT16 *)kernFileAddr;
    if (*mz == 0x5A4D) Print(L"MZ header detected (EFI kernel)\r\n");


    if (setupHeader->handover_offset == 0) PANIC(L"No EFI handover");

    UINT64 kernel_entry = kernFileAddr + setupHeader->handover_offset;
    typedef void (*handover_fn)(struct boot_params *);

    // kernel entry
    handover_fn entry = (handover_fn)kernel_entry;

    // allocating boot_params

    Print(L"filling out boot_params\r\n");

    struct boot_params *bp;

    Status = uefi_call_wrapper(BS->AllocatePages, 4,
        AllocateAnyPages,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(sizeof(struct boot_params)),
        (EFI_PHYSICAL_ADDRESS *)&bp
    );

    if (EFI_ERROR(Status)) PANIC(L"unable to allocate pages for boot_params");

    if ((UINT64)bp >= 0x100000000) PANIC(L"boot_params is above 4GiB!");

    SetMem(bp, sizeof(*bp), 0);

    // filling out boot_params

    memcpy(&bp->hdr, setupHeader, sizeof(struct setup_header));

    bp->hdr.type_of_loader = 0xFF;
    bp->hdr.loadflags |= (1 << 5);   // KEEP_SEGMENTS
    bp->hdr.heap_end_ptr = 0xFE00;   // safe default
    bp->hdr.ext_loader_type = 0xFF;
    bp->hdr.ext_loader_ver  = 0x01;

    bp->efi_info.efi_loader_signature = 0x34364C45; // "EL64"
    bp->efi_info.efi_systab = (UINT32)(UINTN)SystemTable;
    bp->efi_info.efi_systab_hi = (UINT32)(((UINTN)SystemTable) >> 32);

    // cmdline

    Print(L"making cmdline\r\n");

    EFI_PHYSICAL_ADDRESS cmdline_phys;

    UINTN cmdline_len = strlena(conf.cmdline) + 1;

    Status = uefi_call_wrapper(BS->AllocatePages, 4,
        AllocateAnyPages,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(cmdline_len),
        &cmdline_phys
    );
    if (EFI_ERROR(Status)) PANIC(L"cmdline alloc failed");

    memcpy((void*)cmdline_phys, conf.cmdline, cmdline_len);

    bp->hdr.cmd_line_ptr = (UINT32)cmdline_phys;
    bp->hdr.cmdline_size = cmdline_len;

    Print(L"getting memory map\r\nand also Goodbye UEFI!\r\n");

    // memory map


    EFI_MEMORY_DESCRIPTOR *memmap = NULL;
    UINTN memmap_size = 0;
    UINTN map_key;
    UINTN desc_size;
    UINT32 desc_version;

    get_memory_map:
    memmap = NULL;
    memmap_size = 0;

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5,
        &memmap_size,
        memmap,
        &map_key,
        &desc_size,
        &desc_version
    );
    if (Status != EFI_BUFFER_TOO_SMALL) PANIC(L"GetMemoryMap did not return BUFFER_TOO_SMALL");
    memmap_size += 2 * desc_size;

    Status = uefi_call_wrapper(BS->AllocatePool, 3,
        EfiLoaderData,
        memmap_size,
        (void**)&memmap
    );

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5,
        &memmap_size,
        memmap,
        &map_key,
        &desc_size,
        &desc_version
    );

    Status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, map_key);
    if (Status == EFI_INVALID_PARAMETER) {
        Print(L"fail\r\n");
        uefi_call_wrapper(BS->FreePool, 1, memmap);
        goto get_memory_map;
    }

    // no more UEFI

    bp->efi_info.efi_memmap = (UINT32)(UINTN)memmap;
    bp->efi_info.efi_memmap_hi = (UINT32)(((UINTN)memmap) >> 32);
    bp->efi_info.efi_memmap_size = memmap_size;
    bp->efi_info.efi_memdesc_size = desc_size;
    bp->efi_info.efi_memdesc_version = desc_version;

    UINTN entries = memmap_size / desc_size;
    EFI_MEMORY_DESCRIPTOR *d = memmap;

    for (UINTN i = 0; i < entries && bp->e820_entries < E820_MAX_ENTRIES_ZEROPAGE; i++) {
        struct boot_e820_entry *e = &bp->e820_table[bp->e820_entries];

        e->addr = d->PhysicalStart;
        e->size = d->NumberOfPages * 4096;

        switch (d->Type) {
            case EfiConventionalMemory:
                e->type = 1; // RAM
                break;
            case EfiACPIReclaimMemory:
                e->type = 3; // ACPI
                break;
            case EfiACPIMemoryNVS:
                e->type = 4; // NVS
                break;
            default:
                e->type = 2; // RESERVED
        }

        bp->e820_entries++;
        d = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)d + desc_size);
    }

    entry(bp);
    __builtin_unreachable();


}

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
