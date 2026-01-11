#include "uefi.h"

EFI_STATUS EFIAPI efi_main(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
) {

    EFI_INPUT_KEY key;
    UINTN index;

    UEFI_PRINT(L"hello world!");


    EFI_FILE_PROTOCOL *Root;

    EFI_CHECK(MountImageVolume(ImageHandle, SystemTable, &Root));
    UEFI_PRINT(L"Mounted image filesystem\r\n");

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
    return EFI_SUCCESS;
}
