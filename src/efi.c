#include "uefi.h"

EFI_STATUS EFIAPI efi_main(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
) {

    EFI_INPUT_KEY key;
    UINTN index;

    UEFI_PRINT(L"Attempting to mount ESP\r\n");

    EFI_FILE_PROTOCOL *Root;
    MountImageVolume(ImageHandle, SystemTable, &Root);
    UEFI_PRINT(L"Mounted ESP");

    for (;;);

    //SystemTable->BootServices->Stall(5 * 1000 * 1000);
    return EFI_SUCCESS;
}
