#include "uefi.h"
#define SETUP_HEADER_OFFSET 0x1F1

struct setup_header {
    UINT8  setup_sects;        // 0x1F1
    UINT16 root_flags;         // 0x1F2
    UINT32 syssize;            // 0x1F4
    UINT16 ram_size;           // 0x1F8
    UINT16 vid_mode;           // 0x1FA
    UINT16 root_dev;           // 0x1FC
    UINT16 boot_flag;          // 0x1FE 0xAA55
    UINT16 jump;               // 0x200
    UINT8  header[4];          // 0x202 "HdrS"
    UINT16 version;            // 0x206
    UINT32 realmode_swtch;     // 0x208
    UINT16 start_sys_seg;      // 0x20C
    UINT16 kernel_version;     // 0x20E
    UINT8  type_of_loader;     // 0x210
    UINT8  loadflags;          // 0x211
    UINT16 setup_move_size;    // 0x212
    UINT32 code32_start;       // 0x214
    UINT32 ramdisk_image;      // 0x218
    UINT32 ramdisk_size;       // 0x21C
    UINT32 bootsect_kludge;    // 0x220
    UINT16 heap_end_ptr;       // 0x224
    UINT8  ext_loader_ver;     // 0x226
    UINT8  ext_loader_type;    // 0x227
    UINT32 cmd_line_ptr;       // 0x228
    UINT32 initrd_addr_max;    // 0x22C
    UINT32 kernel_alignment;   // 0x230
    UINT8  relocatable_kernel;// 0x234
    UINT8  min_alignment;      // 0x235
    UINT16 xloadflags;         // 0x236
    UINT32 cmdline_size;       // 0x238
    UINT32 hardware_subarch;   // 0x23C
    UINT64 pref_address;       // 0x240
    UINT32 init_size;          // 0x248
    UINT32 handover_offset;    // 0x24C
} __attribute__((packed));
