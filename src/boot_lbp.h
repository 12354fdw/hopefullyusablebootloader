#include "uefi.h"
#include "lbp.h"

#include "util.h"
#include <efi/efi.h>
#include <efi/efidef.h>
#include <efi/efierr.h>
#include <efi/efilib.h>
#include <efi/x86_64/efibind.h>

#include "config.h"

void BOOT_KERNEL_LBP(struct config conf, EFI_FILE_PROTOCOL *Root, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"Using path 'LBP + EFI handover'\r\n");
    EFI_STATUS Status;

    CHAR16 kernPathCHAR16[128];
    AsciiToChar16(conf.kernelPath, kernPathCHAR16, 128);

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
    if (setupHeader->boot_flag != 0xAA55) PANIC(L"Invalid kerenl: case 1");
    if (setupHeader->header != 0x53726448) PANIC(L"Invalid kernel: case 2");
    if (setupHeader->version < 0x020F) PANIC(L"boot protocol too old! (at least 2.15)");
    if (!(setupHeader->xloadflags & (1 << 0))) PANIC(L"kernel does not support 64 bit!");
    if (!(setupHeader->xloadflags & (1 << 5))) PANIC(L"kernel does not support EFI handover!");
    if (setupHeader->handover_offset == 0) PANIC(L"invalid kernel: case 3");

    // kernel relocation

    Print(L"relocating kernel\r\n");
    UINT64 payload_offset = (setupHeader->setup_sects ? setupHeader->setup_sects + 1 : 5) * 512;
    UINT64 payload_size   = kernFileSize - payload_offset;
    //UINT64 payload_offset = setupHeader->payload_offset;

    EFI_PHYSICAL_ADDRESS kernel_base = 0;

    if (!setupHeader->relocatable_kernel) {
        kernel_base = setupHeader->pref_address;
        Status = uefi_call_wrapper(
            BS->AllocatePages,
            4,
            AllocateAddress,
            EfiLoaderData,
            EFI_SIZE_TO_PAGES(kernFileSize),
            &kernel_base
        );
    } else {
        UINT64 align = setupHeader->kernel_alignment;
        if (align == 0)
            align = 0x200000; // 2 MiB default


        UINTN pages = EFI_SIZE_TO_PAGES(kernFileSize + align);

        Status = uefi_call_wrapper(
            BS->AllocatePages,
            4,
            AllocateAnyPages,
            EfiLoaderData,
            pages,
            &kernel_base
        );

        kernel_base =
    (kernel_base + align - 1) &
    ~(align - 1);

    }

    if (EFI_ERROR(Status)) PANIC(L"kernel alloc failed");

    //memcpy((void *)kernel_base,(void *)((UINT8 *)kernFileAddr + payload_offset),payload_size);
    memcpy((void *)kernel_base, (void *)kernFileAddr, kernFileSize);

    Print(L"relocated to 0x%x\r\n",kernel_base);
    Print(L"allocating boot_params and filling it out\r\n");

    struct boot_params *bp;
    Status = uefi_call_wrapper(BS->AllocatePages, 4,
        AllocateAnyPages,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(sizeof(struct boot_params)),
        (EFI_PHYSICAL_ADDRESS *)&bp
    );
    if (EFI_ERROR(Status)) PANIC(L"boot_params alloc failed");

    memset(bp,0,sizeof(*bp));
    memcpy(&bp->hdr,setupHeader,sizeof(struct setup_header));

    bp->hdr.type_of_loader = 0x20;   // custom loader
    
    Print(L"setting cmdline and initrd\r\n");

    CHAR8* cmdline;
    UINTN cmdlen = strlena(conf.cmdline) + 1;
        
    Status = uefi_call_wrapper(BS->AllocatePages, 4,
        AllocateAnyPages,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(cmdlen),
        (EFI_PHYSICAL_ADDRESS *)&cmdline
    );
    if (EFI_ERROR(Status)) PANIC(L"cmdline alloc failed");

    memcpy(cmdline, conf.cmdline, cmdlen);

    bp->hdr.cmd_line_ptr = (UINT64)cmdline;
    bp->hdr.cmdline_size = cmdlen;

    bp->hdr.ramdisk_image = 0;
    bp->hdr.ramdisk_size  = 0;

    Print(L"getting memorymap\r\n");

    
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

    bp->efi_info.efi_memmap          = (UINT64)(uintptr_t)memmap;
    bp->efi_info.efi_memmap_size     = memmap_size;
    bp->efi_info.efi_memdesc_size    = desc_size;
    bp->efi_info.efi_memdesc_version = desc_version;
    

    bp->efi_info.efi_loader_signature = EFI_LOADER_SIGNATURE;
    bp->efi_info.efi_systab = (UINT64)(uintptr_t)SystemTable;

    typedef void (*efi_handover_t)(
        EFI_HANDLE image_handle,
        EFI_SYSTEM_TABLE *system_table,
        struct boot_params *boot_params
    );

    serial_printf("reached handover!\n");
    serial_printf(
    "debug info:\nHdrS=%x ver=%x setup_sects=%u payload_off=%x handover_off=%x boot_flag=%x\n",
        setupHeader->header,
        setupHeader->version,
        setupHeader->setup_sects,
        payload_offset,
        setupHeader->handover_offset,
        setupHeader->boot_flag
    );

    typedef void (*efi_handover_t)(
        EFI_HANDLE,
        EFI_SYSTEM_TABLE *,
        struct boot_params *
    );

    UINT64 entry = kernel_base + bp->hdr.handover_offset;
    serial_printf("entry point: 0x%x\n", entry);

    efi_handover_t handover = (efi_handover_t)(uintptr_t)entry;

    __asm__ volatile (
        "cli\n"
        "cld\n"
        "jmp *%0\n"
        :
        : "r"(handover),
        "D"(ImageHandle),
        "S"(SystemTable),
        "d"(bp)
        : "memory"
    );

    //__builtin_unreachable();


}

