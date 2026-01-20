#include "uefi.h"
#define SETUP_HEADER_OFFSET 0x1F1

#define EFI_LOADER_SIGNATURE 0x34364c45  /* "EL64" */

#ifndef LBP_H
#define LBP_H
struct setup_header {
    UINT8  setup_sects;
    UINT16 root_flags;
    UINT32 syssize;
    UINT16 ram_size;
    UINT16 vid_mode;
    UINT16 root_dev;
    UINT16 boot_flag;
    UINT16 jump;
    UINT32 header;              // "HdrS"
    UINT16 version;
    UINT32 realmode_swtch;
    UINT16 start_sys_seg;
    UINT16 kernel_version;
    UINT8  type_of_loader;
    UINT8  loadflags;
    UINT16 setup_move_size;
    UINT32 code32_start;
    UINT32 ramdisk_image;
    UINT32 ramdisk_size;
    UINT32 bootsect_kludge;
    UINT16 heap_end_ptr;
    UINT8  ext_loader_ver;
    UINT8  ext_loader_type;
    UINT32 cmd_line_ptr;
    UINT32 initrd_addr_max;
    UINT32 kernel_alignment;
    UINT8  relocatable_kernel;
    UINT8  min_alignment;
    UINT16 xloadflags;
    UINT32 cmdline_size;
    UINT32 hardware_subarch;
    UINT64 hardware_subarch_data;
    UINT32 payload_offset;
    UINT32 payload_length;
    UINT64 setup_data;
    UINT64 pref_address;
    UINT32 init_size;
    UINT32 handover_offset;
} __attribute__((packed));

struct screen_info {
        UINT8  orig_x;                /* 0x00 */
        UINT8  orig_y;                /* 0x01 */
        UINT16 ext_mem_k;        /* 0x02 */
        UINT16 orig_video_page;        /* 0x04 */
        UINT8  orig_video_mode;        /* 0x06 */
        UINT8  orig_video_cols;        /* 0x07 */
        UINT16 unused2;                /* 0x08 */
        UINT16 orig_video_ega_bx;/* 0x0a */
        UINT16 unused3;                /* 0x0c */
        UINT8  orig_video_lines;        /* 0x0e */
        UINT8  orig_video_isVGA;        /* 0x0f */
        UINT16 orig_video_points;/* 0x10 */

        /* VESA graphic mode -- linear frame buffer */
        UINT16 lfb_width;        /* 0x12 */
        UINT16 lfb_height;        /* 0x14 */
        UINT16 lfb_depth;        /* 0x16 */
        UINT32 lfb_base;                /* 0x18 */
        UINT32 lfb_size;                /* 0x1c */
        UINT16 cl_magic, cl_offset; /* 0x20 */
        UINT16 lfb_linelength;        /* 0x24 */
        UINT8  red_size;                /* 0x26 */
        UINT8  red_pos;                /* 0x27 */
        UINT8  green_size;        /* 0x28 */
        UINT8  green_pos;        /* 0x29 */
        UINT8  blue_size;        /* 0x2a */
        UINT8  blue_pos;                /* 0x2b */
        UINT8  rsvd_size;        /* 0x2c */
        UINT8  rsvd_pos;                /* 0x2d */
        UINT16 vesapm_seg;        /* 0x2e */
        UINT16 vesapm_off;        /* 0x30 */
        UINT16 pages;                /* 0x32 */
        UINT16 vesa_attributes;        /* 0x34 */
        UINT32 capabilities;     /* 0x36 */
        UINT8  _reserved[6];        /* 0x3a */
} __attribute__((packed));

struct apm_bios_info {
        UINT16        version;
        UINT16        cseg;
        UINT32        offset;
        UINT16        cseg_16;
        UINT16        dseg;
        UINT16        flags;
        UINT16        cseg_len;
        UINT16        cseg_16_len;
        UINT16        dseg_len;
} __attribute__ ((packed));

struct ist_info {
    UINT32 signature;
    UINT32 command;
    UINT32 event;
    UINT32 perf_level;
} __attribute__ ((packed));

struct sys_desc_table {
    UINT16 length;
    UINT8 table[14];
} __attribute__ ((packed));

struct olpc_ofw_header {
    UINT32 ofw_magic;
    UINT32 ofw_version;
    UINT32 cif_handler;
    UINT32 irq_desc_table;
} __attribute__ ((packed));

struct edid_info {
    unsigned char dummy[128];
} __attribute__ ((packed));

struct efi_info {
    UINT32 efi_loader_signature;
    UINT32 efi_loader_version;
    UINT64 efi_systab;
    UINT64 efi_memmap;
    UINT32 efi_memmap_size;
    UINT32 efi_memdesc_size;
    UINT32 efi_memdesc_version;
};


struct boot_e820_entry {
    UINT64 addr;
    UINT64 size;
    UINT32 type;
} __attribute__ ((packed));

struct edd_device_params {
	UINT16 length;
	UINT16 info_flags;
	UINT32 num_default_cylinders;
	UINT32 num_default_heads;
	UINT32 sectors_per_track;
	UINT64 number_of_sectors;
	UINT16 bytes_per_sector;
	UINT32 dpte_ptr;		/* 0xFFFFFFFF for our purposes */
	UINT16 key;		/* = 0xBEDD */
	UINT8 device_path_info_length;	/* = 44 */
	UINT8 reserved2;
	UINT16 reserved3;
	UINT8 host_bus_type[4];
	UINT8 interface_type[8];
	union {
		struct {
			UINT16 base_address;
			UINT16 reserved1;
			UINT32 reserved2;
		} __attribute__ ((packed)) isa;
		struct {
			UINT8 bus;
			UINT8 slot;
			UINT8 function;
			UINT8 channel;
			UINT32 reserved;
		} __attribute__ ((packed)) pci;
		/* pcix is same as pci */
		struct {
			UINT64 reserved;
		} __attribute__ ((packed)) ibnd;
		struct {
			UINT64 reserved;
		} __attribute__ ((packed)) xprs;
		struct {
			UINT64 reserved;
		} __attribute__ ((packed)) htpt;
		struct {
			UINT64 reserved;
		} __attribute__ ((packed)) unknown;
	} interface_path;
	union {
		struct {
			UINT8 device;
			UINT8 reserved1;
			UINT16 reserved2;
			UINT32 reserved3;
			UINT64 reserved4;
		} __attribute__ ((packed)) ata;
		struct {
			UINT8 device;
			UINT8 lun;
			UINT8 reserved1;
			UINT8 reserved2;
			UINT32 reserved3;
			UINT64 reserved4;
		} __attribute__ ((packed)) atapi;
		struct {
			UINT16 id;
			UINT64 lun;
			UINT16 reserved1;
			UINT32 reserved2;
		} __attribute__ ((packed)) scsi;
		struct {
			UINT64 serial_number;
			UINT64 reserved;
		} __attribute__ ((packed)) usb;
		struct {
			UINT64 eui;
			UINT64 reserved;
		} __attribute__ ((packed)) i1394;
		struct {
			UINT64 wwid;
			UINT64 lun;
		} __attribute__ ((packed)) fibre;
		struct {
			UINT64 identity_tag;
			UINT64 reserved;
		} __attribute__ ((packed)) i2o;
		struct {
			UINT32 array_number;
			UINT32 reserved1;
			UINT64 reserved2;
		} __attribute__ ((packed)) raid;
		struct {
			UINT8 device;
			UINT8 reserved1;
			UINT16 reserved2;
			UINT32 reserved3;
			UINT64 reserved4;
		} __attribute__ ((packed)) sata;
		struct {
			UINT64 reserved1;
			UINT64 reserved2;
		} __attribute__ ((packed)) unknown;
	} device_path;
	UINT8 reserved4;
	UINT8 checksum;
} __attribute__ ((packed));

struct edd_info {
	UINT8 device;
	UINT8 version;
	UINT16 interface_support;
	UINT16 legacy_max_cylinder;
	UINT8 legacy_max_head;
	UINT8 legacy_sectors_per_track;
	struct edd_device_params params;
} __attribute__ ((packed));

#define E820_MAX_ENTRIES_ZEROPAGE 128
#define EDD_MBR_SIG_MAX 16
#define EDDMAXNR 6

/* The so-called "zeropage" */
struct boot_params {
	struct screen_info screen_info;			/* 0x000 */
	struct apm_bios_info apm_bios_info;		/* 0x040 */
	UINT8  _pad2[4];					/* 0x054 */
	UINT64  tboot_addr;				/* 0x058 */
	struct ist_info ist_info;			/* 0x060 */
	UINT64 acpi_rsdp_addr;				/* 0x070 */
	UINT8  _pad3[8];					/* 0x078 */
	UINT8  hd0_info[16];	/* obsolete! */		/* 0x080 */
	UINT8  hd1_info[16];	/* obsolete! */		/* 0x090 */
	struct sys_desc_table sys_desc_table; /* obsolete! */	/* 0x0a0 */
	struct olpc_ofw_header olpc_ofw_header;		/* 0x0b0 */
	UINT32 ext_ramdisk_image;			/* 0x0c0 */
	UINT32 ext_ramdisk_size;				/* 0x0c4 */
	UINT32 ext_cmd_line_ptr;				/* 0x0c8 */
	UINT8  _pad4[112];				/* 0x0cc */
	UINT32 cc_blob_address;				/* 0x13c */
	struct edid_info edid_info;			/* 0x140 */
	struct efi_info efi_info;			/* 0x1c0 */
	UINT32 alt_mem_k;				/* 0x1e0 */
	UINT32 scratch;		/* Scratch field! */	/* 0x1e4 */
	UINT8  e820_entries;				/* 0x1e8 */
	UINT8  eddbuf_entries;				/* 0x1e9 */
	UINT8  edd_mbr_sig_buf_entries;			/* 0x1ea */
	UINT8  kbd_status;				/* 0x1eb */
	UINT8  secure_boot;				/* 0x1ec */
	UINT8  _pad5[2];					/* 0x1ed */
	/*
	 * The sentinel is set to a nonzero value (0xff) in header.S.
	 *
	 * A bootloader is supposed to only take setup_header and put
	 * it into a clean boot_params buffer. If it turns out that
	 * it is clumsy or too generous with the buffer, it most
	 * probably will pick up the sentinel variable too. The fact
	 * that this variable then is still 0xff will let kernel
	 * know that some variables in boot_params are invalid and
	 * kernel should zero out certain portions of boot_params.
	 */
	UINT8  sentinel;					/* 0x1ef */
	UINT8  _pad6[1];					/* 0x1f0 */
	struct setup_header hdr;    /* setup header */	/* 0x1f1 */
	UINT8  _pad7[0x290-0x1f1-sizeof(struct setup_header)];
	UINT32 edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];	/* 0x290 */
	struct boot_e820_entry e820_table[E820_MAX_ENTRIES_ZEROPAGE]; /* 0x2d0 */
	UINT8  _pad8[48];				/* 0xcd0 */
	struct edd_info eddbuf[EDDMAXNR];		/* 0xd00 */
	UINT8  _pad9[276];				/* 0xeec */
} __attribute__((packed));

#endif
