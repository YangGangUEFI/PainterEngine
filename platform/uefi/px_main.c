#include "../../runtime/PainterEngine_Application.h"
#include "Uefi/UefiBaseType.h"
#include "px_main.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL *mEfiGop = NULL;
EFI_HANDLE                   mImageHandle = NULL;

px_bool PX_AudioInitialize(PX_SoundPlay *soundPlay)
{
  return FALSE;
}

EFI_ABSOLUTE_POINTER_PROTOCOL  *mAbsPointer = NULL;
EFI_SIMPLE_POINTER_PROTOCOL    *mSimplePointer = NULL;

typedef struct {
  INTN                           LastCursorX;
  INTN                           LastCursorY;
  BOOLEAN                        LeftButton;
  BOOLEAN                        Click;
  BOOLEAN                        CursorMove;
} PX_UEFI_MOUSE_DATA;

PX_UEFI_MOUSE_DATA  mPxUefiMouseData;

EFI_STATUS
EfiMouseInit (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_ABSOLUTE_POINTER_PROTOCOL  *AbsPointer = NULL;
  EFI_SIMPLE_POINTER_PROTOCOL    *SimplePointer = NULL;
  EFI_HANDLE                     *HandleBuffer = NULL;
  UINTN                          HandleCount, Index;
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;
  BOOLEAN                        AbsPointerSupport = FALSE;
  BOOLEAN                        SimplePointerSupport = FALSE;

  HandleCount = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiAbsolutePointerProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
    if (!EFI_ERROR (Status)) {
      AbsPointerSupport = TRUE;
      break;
    }
  }
  if (HandleBuffer!= NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  if (AbsPointerSupport) {
    goto StartInit;
  }

  HandleCount = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimplePointerProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
    if (!EFI_ERROR (Status)) {
      SimplePointerSupport = TRUE;
      break;
    }
  }
  if (HandleBuffer!= NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  if (!AbsPointerSupport && !SimplePointerSupport) {
    return EFI_UNSUPPORTED;
  }

StartInit:

  if (AbsPointerSupport) {
    Status = gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiAbsolutePointerProtocolGuid, (VOID **)&AbsPointer);
    if (!EFI_ERROR (Status) && AbsPointer != NULL) {
      // AbsPointer->Reset(AbsPointer, TRUE);
      mAbsPointer = AbsPointer;
    }
  }

  if (SimplePointerSupport && !AbsPointerSupport) {
    Status = gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiSimplePointerProtocolGuid, (VOID **)&SimplePointer);
    if (!EFI_ERROR (Status) && SimplePointer != NULL) {
      // SimplePointer->Reset(SimplePointer, TRUE);
      mSimplePointer = SimplePointer;
    }
  }

  mPxUefiMouseData.CursorMove = FALSE;
  mPxUefiMouseData.LeftButton = FALSE;
  mPxUefiMouseData.Click = FALSE;
  mPxUefiMouseData.LastCursorX = 0;
  mPxUefiMouseData.LastCursorY = 0;

  return Status;
}


EFI_STATUS
CheckEfiMouseEvent (
  VOID
  )
{
  EFI_STATUS                     Status = EFI_NOT_READY;
  EFI_ABSOLUTE_POINTER_STATE     AbsState;
  EFI_SIMPLE_POINTER_STATE       SimpleState;
  INTN                           LastCursorX, LastCursorY, LastLeft;

  if (!mAbsPointer && !mSimplePointer) {
    return EFI_UNSUPPORTED;
  }

  mPxUefiMouseData.CursorMove = FALSE;
  mPxUefiMouseData.Click = FALSE;
  LastCursorX = mPxUefiMouseData.LastCursorX;
  LastCursorY = mPxUefiMouseData.LastCursorY;
  LastLeft    = mPxUefiMouseData.LeftButton;
  if (mAbsPointer != NULL) {
    Status = mAbsPointer->GetState (mAbsPointer, &AbsState);
    if (!EFI_ERROR (Status)) {
      mPxUefiMouseData.LastCursorX = (AbsState.CurrentX * mEfiGop->Mode->Info->HorizontalResolution) / (mAbsPointer->Mode->AbsoluteMaxX - mAbsPointer->Mode->AbsoluteMinX);
      if (mPxUefiMouseData.LastCursorX >= mEfiGop->Mode->Info->HorizontalResolution) {
        mPxUefiMouseData.LastCursorX = mEfiGop->Mode->Info->HorizontalResolution;
      }
      mPxUefiMouseData.LastCursorY = (AbsState.CurrentY * mEfiGop->Mode->Info->VerticalResolution) / (mAbsPointer->Mode->AbsoluteMaxY - mAbsPointer->Mode->AbsoluteMinY);
      if (mPxUefiMouseData.LastCursorY >= mEfiGop->Mode->Info->VerticalResolution) {
        mPxUefiMouseData.LastCursorY = mEfiGop->Mode->Info->VerticalResolution;
      }
      mPxUefiMouseData.LeftButton = AbsState.ActiveButtons & BIT0;
    }
  } else if (mSimplePointer != NULL) {
    Status = mSimplePointer->GetState (mSimplePointer, &SimpleState);
    if (!EFI_ERROR (Status)) {
      mPxUefiMouseData.LastCursorX += SimpleState.RelativeMovementX / (INT32)mSimplePointer->Mode->ResolutionX;
      if (mPxUefiMouseData.LastCursorX >= mEfiGop->Mode->Info->HorizontalResolution) {
        mPxUefiMouseData.LastCursorX = mEfiGop->Mode->Info->HorizontalResolution;
      }
      if (mPxUefiMouseData.LastCursorX <= 0) {
        mPxUefiMouseData.LastCursorX = 0;
      }
      mPxUefiMouseData.LastCursorY += SimpleState.RelativeMovementY / (INT32)mSimplePointer->Mode->ResolutionY;
      if (mPxUefiMouseData.LastCursorY >= mEfiGop->Mode->Info->VerticalResolution) {
        mPxUefiMouseData.LastCursorY = mEfiGop->Mode->Info->VerticalResolution;
      }
      if (mPxUefiMouseData.LastCursorY <= 0) {
        mPxUefiMouseData.LastCursorY = 0;
      }

      mPxUefiMouseData.LeftButton = SimpleState.LeftButton;
    }
  }
  
  if (LastCursorX == mPxUefiMouseData.LastCursorX && LastCursorY == mPxUefiMouseData.LastCursorY) {
    if (LastLeft && !mPxUefiMouseData.LeftButton) {
      mPxUefiMouseData.Click = TRUE;
    }
  } else {
    mPxUefiMouseData.CursorMove = TRUE;
  }

  return Status;
}


void uefi_gop_render(px_color* gram, px_int width, px_int height)
{
  EFI_STATUS                         Status;
  UINTN                              Delta;

  Delta = mEfiGop->Mode->Info->HorizontalResolution * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

  Status = mEfiGop->Blt (
                      mEfiGop,
                      (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gram,
                      EfiBltBufferToVideo,
                      0,
                      0,
                      0,
                      0,
                      width,
                      height,
                      Delta
                      );
}


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  PX_Object_Event e;
  px_int screen_width=240;
  px_int screen_height=320;
  px_dword timelasttime = 0;
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &mEfiGop);
  if (!EFI_ERROR(Status)) {
    screen_width = mEfiGop->Mode->Info->HorizontalResolution;
    screen_height = mEfiGop->Mode->Info->VerticalResolution;
  }

  mImageHandle = ImageHandle;

  EfiMouseInit();
  
  if (!PX_ApplicationInitialize(&App,screen_width,screen_height))
  {
    return 0;
  }
  
  timelasttime=PX_TimeGetTime();
  while (1)
  {
    e.Event=PX_OBJECT_EVENT_ANY;

    Status = CheckEfiMouseEvent();
    if (!EFI_ERROR(Status)) {
      if (mPxUefiMouseData.CursorMove) {
        e.Event = PX_OBJECT_EVENT_CURSORMOVE;
      }

      if (mPxUefiMouseData.LeftButton) {
        if (mPxUefiMouseData.CursorMove) {
          e.Event = PX_OBJECT_EVENT_CURSORDRAG;
        } else {
          e.Event = PX_OBJECT_EVENT_CURSORDOWN;
        }
      } else {
        e.Event = PX_OBJECT_EVENT_CURSORUP;
      }

      if (mPxUefiMouseData.Click) {
        e.Event = PX_OBJECT_EVENT_CURSORCLICK;
      }

      PX_Object_Event_SetCursorX (&e, mPxUefiMouseData.LastCursorX);
      PX_Object_Event_SetCursorY (&e, mPxUefiMouseData.LastCursorY);
    }

    if(e.Event!=PX_OBJECT_EVENT_ANY)
    {
    	PX_ApplicationPostEvent(&App,e);
    }

    px_dword now=PX_TimeGetTime();
    px_dword elapsed=now-timelasttime;
    timelasttime= now;
    PX_ApplicationUpdate(&App,elapsed);
    PX_ApplicationRender(&App,elapsed);
    uefi_gop_render(App.runtime.RenderSurface.surfaceBuffer, App.runtime.RenderSurface.width, App.runtime.RenderSurface.height);
  }
  
  return 0;
}

