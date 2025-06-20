#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/SimplePointer.h>
#include <Protocol/AbsolutePointer.h>

#include <Guid/FileInfo.h>

extern EFI_HANDLE                   mImageHandle;
