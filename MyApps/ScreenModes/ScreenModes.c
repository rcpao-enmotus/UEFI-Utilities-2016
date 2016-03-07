//
//  Copyright (c) 2015  Finnbarr P. Murphy.   All rights reserved.
//
//  Display TEXT and GRAPHIC screen mode information 
//
//  License: BSD License
//


#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/EfiShell.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include "ConsoleControl.h"
#include "UgaDraw.h"


static int 
memcmp(const void *s1, const void *s2, UINTN n)
{
    const unsigned char *c1 = s1, *c2 = s2;
    int d = 0;

    if (!s1 && !s2)
       return 0;
    if (s1 && !s2)
       return 1;
    if (!s1 && s2)
       return -1;

    while (n--) {
        d = (int)*c1++ - (int)*c2++;
        if (d)
             break;
    }

    return d;
}


EFI_STATUS
PrintUGA(EFI_UGA_DRAW_PROTOCOL *Uga)
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINT32 HorzResolution = 0;
    UINT32 VertResolution = 0;
    UINT32 ColorDepth = 0;
    UINT32 RefreshRate = 0;


    Status = Uga->GetMode( Uga, &HorzResolution, &VertResolution, 
                           &ColorDepth, &RefreshRate);

    if (EFI_ERROR (Status)) {
        Print(L"ERROR: UGA GetMode failed [%d]\n", Status );
    } else {
        Print(L"Horizontal Resolution: %d\n", HorzResolution);
        Print(L"Vertical Resolution: %d\n", VertResolution);
        Print(L"Color Depth: %d\n", ColorDepth);
        Print(L"Refresh Rate: %d\n", RefreshRate);
        Print(L"\n");
    }

    return Status;
}


EFI_STATUS
CheckUGA(BOOLEAN Verbose)
{
    EFI_HANDLE *HandleBuffer = NULL;
    UINTN HandleCount = 0;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_UGA_DRAW_PROTOCOL *Uga;


    // get from ConsoleOutHandle?
    Status = gBS->HandleProtocol( gST->ConsoleOutHandle, 
                                  &gEfiUgaDrawProtocolGuid, 
                                  (VOID **) &Uga);
    if (EFI_ERROR (Status)) {
        Print(L"No UGA handle found via HandleProtocol\n");
    } else {
        Print(L"UGA handle found via HandleProtocol\n");
        if (Verbose)
            PrintUGA(Uga);
    }

    // try locating directly
    Status = gBS->LocateProtocol( &gEfiUgaDrawProtocolGuid,
                                  NULL,
                                  (VOID **) &Uga);
    if (EFI_ERROR(Status) || Uga == NULL) {
        Print(L"No UGA handle found via LocateProtocol\n");
    } else {
        Print(L"Found UGA handle via LocateProtocol\n");
        if (Verbose)
            PrintUGA(Uga);
    }

    // try locating by handle
    Status = gBS->LocateHandleBuffer( ByProtocol,
                      &gEfiUgaDrawProtocolGuid,
                      NULL,
                      &HandleCount,
                      &HandleBuffer);
    if (EFI_ERROR (Status)) {
        Print(L"No UGA handles found via LocateHandleBuffer\n");
    } else {
        Print(L"Found %d UGA handles via LocateHandleBuffer\n", HandleCount);
        for (int i = 0; i < HandleCount; i++) {
            Status = gBS->HandleProtocol( HandleBuffer[i],
                                          &gEfiUgaDrawProtocolGuid,
                                          (VOID*) &Uga);
            if (!EFI_ERROR (Status)) { 
                if (Verbose)
                    PrintUGA(Uga);
            }
        }
        FreePool(HandleBuffer);
    }

    Print(L"\n");

    return Status;
}


EFI_STATUS
PrintGOP(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop)
{
    int i, imax;
    EFI_STATUS Status;

    imax = gop->Mode->MaxMode;

    Print(L"GOP reports MaxMode %d\n", imax);

    for (i = 0; i < imax; i++) {
         EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
         UINTN SizeOfInfo;

         Status = gop->QueryMode(gop, i, &SizeOfInfo, &Info);
         if (EFI_ERROR(Status) && Status == EFI_NOT_STARTED) {
             gop->SetMode(gop, gop->Mode->Mode);
             Status = gop->QueryMode(gop, i, &SizeOfInfo, &Info);
         }

         if (EFI_ERROR(Status)) {
             Print(L"ERROR: Bad response from QueryMode: %d\n", Status);
             continue;
         }
         Print(L"%c%d: %dx%d ", memcmp(Info,gop->Mode->Info,sizeof(*Info)) == 0 ? '*' : ' ', i,
                        Info->HorizontalResolution,
                        Info->VerticalResolution);
         switch(Info->PixelFormat) {
             case PixelRedGreenBlueReserved8BitPerColor:
                  Print(L"RGBRerserved");
                  break;
             case PixelBlueGreenRedReserved8BitPerColor:
                  Print(L"BGRReserved");
                  break;
             case PixelBitMask:
                  Print(L"Red:%08x Green:%08x Blue:%08x Reserved:%08x",
                          Info->PixelInformation.RedMask,
                          Info->PixelInformation.GreenMask,
                          Info->PixelInformation.BlueMask,
                          Info->PixelInformation.ReservedMask);
                          break;
             case PixelBltOnly:
                  Print(L"(blt only)");
                  break;
             default:
                  Print(L"(Invalid pixel format)");
                 break;
        }
        Print(L" Pixels %d\n", Info->PixelsPerScanLine);
    }
    Print(L"\n");

    return EFI_SUCCESS;
}


EFI_STATUS
CheckGOP(BOOLEAN Verbose)
{
    EFI_HANDLE *HandleBuffer = NULL;
    UINTN HandleCount = 0;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;


    // get from ConsoleOutHandle?
    Status = gBS->HandleProtocol( gST->ConsoleOutHandle, 
                                  &gEfiGraphicsOutputProtocolGuid, 
                                  (VOID **) &Gop);
    if (EFI_ERROR (Status)) {
        Print(L"No GOP handle found via HandleProtocol\n");
    } else {
        Print(L"GOP handle found via HandleProtocol\n");
        if (Verbose)
            PrintGOP(Gop);
    }

    // try locating directly
    Status = gBS->LocateProtocol( &gEfiGraphicsOutputProtocolGuid,
                                  NULL,
                                  (VOID **) &Gop);
    if (EFI_ERROR(Status) || Gop == NULL) {
        Print(L"No GOP handle found via LocateProtocol\n");
    } else {
        Print(L"Found GOP handle via LocateProtocol\n");
        if (Verbose)
            PrintGOP(Gop);
    }

    // try locating by handle
    Status = gBS->LocateHandleBuffer( ByProtocol,
                      &gEfiGraphicsOutputProtocolGuid,
                      NULL,
                      &HandleCount,
                      &HandleBuffer);
    if (EFI_ERROR (Status)) {
        Print(L"No GOP handles found via LocateHandleBuffer\n");
    } else {
        Print(L"Found %d GOP handles via LocateHandleBuffer\n", HandleCount);
        for (int i = 0; i < HandleCount; i++) {
            Status = gBS->HandleProtocol( HandleBuffer[i],
                                          &gEfiGraphicsOutputProtocolGuid,
                                          (VOID*) &Gop);
            if (!EFI_ERROR (Status)) { 
                if (Verbose)
                    PrintGOP(Gop);
            }
        }
        FreePool(HandleBuffer);
    }

    Print(L"\n");

    return Status;
}


EFI_STATUS
PrintCCP(EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl)
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_CONSOLE_CONTROL_SCREEN_MODE Mode;
    BOOLEAN GopUgaExists;
    BOOLEAN StdInLocked;

    Status = ConsoleControl->GetMode(ConsoleControl, &Mode, &GopUgaExists, &StdInLocked);
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: ConsoleControl GetMode failed [%d]\n", Status );
        return Status;
    }

    Print(L"Screen mode: ");
    switch (Mode) {
       case EfiConsoleControlScreenText:
              Print(L"Text");
              break;
       case EfiConsoleControlScreenGraphics:
              Print(L"Graphics");
              break;
       case EfiConsoleControlScreenMaxValue:
              Print(L"MaxValue");
              break;
    }
    Print(L"\n");
    Print(L"Graphics Support Avalaiable: ");
    if (GopUgaExists) 
        Print(L"Yes");
    else
        Print(L"No");
    Print(L"\n");

    Print(L"\n");

    return EFI_SUCCESS;
}


EFI_STATUS
CheckCCP(BOOLEAN Verbose)
{
    EFI_GUID gEfiConsoleControlProtocolGuid = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
    EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl = NULL;
    EFI_HANDLE *HandleBuffer = NULL;
    UINTN HandleCount = 0;
    EFI_STATUS Status = EFI_SUCCESS;

    // get from ConsoleOutHandle?
    Status = gBS->HandleProtocol( gST->ConsoleOutHandle,
                                  &gEfiConsoleControlProtocolGuid,
                                  (VOID **) &ConsoleControl);
    if (EFI_ERROR (Status)) {
        Print(L"No ConsoleControl handle found via HandleProtocol\n");
    } else {
        Print(L"ConsoleControl handle found via HandleProtocol\n");
        if (Verbose)
            PrintCCP(ConsoleControl);
    }

    // try locating directly
    Status = gBS->LocateProtocol( &gEfiConsoleControlProtocolGuid,
                                  NULL,
                                  (VOID **) &ConsoleControl);
    if (EFI_ERROR(Status) || ConsoleControl == NULL) {
        Print(L"No ConsoleControl handle found via LocateProtocol\n");
    } else {
        Print(L"Found ConsoleControl handle via LocateProtocol\n");
        if (Verbose)
            PrintCCP(ConsoleControl);
    }

    // try locating by handle
    Status = gBS->LocateHandleBuffer( ByProtocol,
                                      &gEfiConsoleControlProtocolGuid,
                                      NULL,
                                      &HandleCount,
                                      &HandleBuffer);
    if (EFI_ERROR (Status)) {
        Print(L"No ConsoleControl handles found via LocateHandleBuffer\n");
    } else {
        Print(L"Found %d ConsoleControl handles via LocateHandleBuffer\n", HandleCount);
        for (int i = 0; i < HandleCount; i++) {
            Status = gBS->HandleProtocol( HandleBuffer[i],
                                          &gEfiConsoleControlProtocolGuid,
                                          (VOID*) &ConsoleControl);
            if (!EFI_ERROR (Status)) 
                if (Verbose)
                    PrintCCP(ConsoleControl);
        }
        FreePool(HandleBuffer);
    }

    Print(L"\n");

    return Status;
}



static void
Usage(void)
{
    Print(L"Usage: ScreenModes [-v|--verbose]\n");
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_STATUS Status = EFI_SUCCESS;
    BOOLEAN Verbose = FALSE;

    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--verbose") ||
            !StrCmp(Argv[1], L"-v")) {
            Verbose = TRUE;
        }
        if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h") ||
            !StrCmp(Argv[1], L"-?")) {
            Usage();
            return Status;
        }
    }


    // First check for older EDK ConsoleControl protocol support
    CheckCCP(Verbose);

    // Next check for UGA support (probably none)
    CheckUGA(Verbose);

    // Finally check for GOP support 
    CheckGOP(Verbose);

    return Status;
}
