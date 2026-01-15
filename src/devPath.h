#include "uefi.h"
#include "std.h"

EFI_DEVICE_PATH_PROTOCOL *
MakeFileDevicePath(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, CHAR16 *Path)
{
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath, *FilePath;

    BS->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImage
    );

    DevicePath = DevicePathFromHandle(LoadedImage->DeviceHandle);
    FilePath   = FileDevicePath(NULL, Path);

    return AppendDevicePath(DevicePath, FilePath);
}
