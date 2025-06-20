#include "../../PainterEngine/platform/modules/px_file.h"
#include "px_main.h"

EFI_DEVICE_PATH_PROTOCOL  *mCurrentImageDirDp = NULL;

EFI_DEVICE_PATH_PROTOCOL *
GetCurrentImageDirDp (
	EFI_HANDLE  EfiImageHandle
  )
{
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_STATUS                Status;

  Status = gBS->HandleProtocol(
                  EfiImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID**)&LoadedImage
                  );
  if (!EFI_ERROR(Status)) {
    CHAR16 *DirPath = AllocateCopyPool (StrSize (((FILEPATH_DEVICE_PATH *)LoadedImage->FilePath)->PathName), ((FILEPATH_DEVICE_PATH *)LoadedImage->FilePath)->PathName);
    if (PathRemoveLastItem (DirPath)) {
			DevicePath = FileDevicePath(LoadedImage->DeviceHandle, DirPath);
			FreePool (DirPath);
			return DevicePath;
		}
  }
  
  return NULL;
}

/**

  Function gets the file information from an open file descriptor, and stores it
  in a buffer allocated from pool.

  @param FHand           File Handle.
  @param InfoType        Info type need to get.

  @retval                A pointer to a buffer with file information or NULL is returned

**/
VOID *
LibFileInfo (
  IN EFI_FILE_HANDLE  FHand,
  IN EFI_GUID         *InfoType
  )
{
  EFI_STATUS     Status;
  EFI_FILE_INFO  *Buffer;
  UINTN          BufferSize;

  Buffer     = NULL;
  BufferSize = 0;

  Status = FHand->GetInfo (
                    FHand,
                    InfoType,
                    &BufferSize,
                    Buffer
                    );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Buffer = AllocatePool (BufferSize);
    ASSERT (Buffer != NULL);
  }

  Status = FHand->GetInfo (
                    FHand,
                    InfoType,
                    &BufferSize,
                    Buffer
                    );

  return Buffer;
}

PX_IO_Data PX_LoadFileToIOData(const char path[])
{
  PX_IO_Data io;
  EFI_DEVICE_PATH_PROTOCOL  *FileDp;
  EFI_STATUS                Status;
  EFI_FILE_HANDLE           FileHandle;
  CHAR16                    *Char16Path;

  io.buffer=0;
  io.size=0;
  
  if (mCurrentImageDirDp == NULL) {
    mCurrentImageDirDp = GetCurrentImageDirDp (mImageHandle);
	}

  if (mCurrentImageDirDp) {
    Char16Path = AllocateZeroPool (AsciiStrSize(path) * sizeof(CHAR16));
    AsciiStrToUnicodeStrS(path, Char16Path, (AsciiStrSize(path) * sizeof(CHAR16)));
    FileDp = AppendDevicePath(mCurrentImageDirDp, ConvertTextToDevicePath(Char16Path));
    Status = EfiOpenFileByDevicePath (&FileDp, &FileHandle, EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR(Status)) {
      EFI_FILE_INFO * FileInfo = LibFileInfo (FileHandle, &gEfiFileInfoGuid);
      UINTN FileBufferSize = FileInfo->FileSize;
      UINT8 *FileBuffer = AllocateZeroPool(FileBufferSize);
      Status = FileHandle->Read (FileHandle, &FileBufferSize, FileBuffer);
      if (!EFI_ERROR(Status)) {
      	io.buffer = FileBuffer;
      	io.size = FileBufferSize;
      }
    }
  }

  return io;
}

void PX_FreeIOData(PX_IO_Data *io)
{
	if (io->size&&io->buffer)
	{
		FreePool(io->buffer);
		io->size=0;
		io->buffer=0;
	}
}

px_bool PX_LoadJsonFromFile(PX_Json *json,const px_char *path)
{
	PX_IO_Data io=PX_LoadFileToIOData((px_char *)path);
	if (!io.size)
	{
		return PX_FALSE;
	}

	if(!PX_JsonParse(json,(px_char *)io.buffer))goto _ERROR;


	PX_FreeIOData(&io);
	return PX_TRUE;
_ERROR:
	PX_FreeIOData(&io);
	return PX_FALSE;
}

px_bool PX_LoadFontModuleFromTTF(px_memorypool *mp,PX_FontModule* fm, const px_char Path[], PX_FONTMODULE_CODEPAGE codepage, px_int fontsize)
{
	PX_IO_Data io;
	io = PX_LoadFileToIOData(Path);
	if (!io.size)goto _ERROR;
	if (!PX_FontModuleInitializeTTF(mp,fm, codepage, fontsize,io.buffer,io.size)) goto _ERROR;
	PX_FreeIOData(&io);
	return PX_TRUE;
_ERROR:
	PX_FreeIOData(&io);
	return PX_FALSE;
}

