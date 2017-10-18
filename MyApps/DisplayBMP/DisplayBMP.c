//
//  Copyright (c) 2015  Finnbarr P. Murphy.   All rights reserved.
//
//  Display a BMP image (Similar to the old LoadBMP utility) 
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

typedef struct {
    CHAR8  CharB;
    CHAR8  CharM;
    UINT32 Size;
    UINT16 Reserved[2];
    UINT32 ImageOffset;
    UINT32 HeaderSize;
    UINT32 PixelWidth;
    UINT32 PixelHeight;
    UINT16 Planes;
    UINT16 BitPerPixel;
    UINT32 CompressionType;
    UINT32 ImageSize;
    UINT32 XPixelsPerMeter;
    UINT32 YPixelsPerMeter;
    UINT32 NumberOfColors;
    UINT32 ImportantColors;
} __attribute__((__packed__)) BMP_IMAGE_HEADER;


static VOID
PressKey(BOOLEAN DisplayText)
{

    EFI_INPUT_KEY Key;
    /* EFI_STATUS    Status; */
    UINTN         EventIndex;

    if (DisplayText) {
        Print(L"\nPress any key to continue ....\n\n");
    }

    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    /* Status = */ gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
}


static VOID
AsciiToUnicodeSize( CHAR8 *String,
                    UINT8 length,
                    CHAR16 *UniString)
{
    int len = length;

    while (*String != '\0' && len > 0) {
        *(UniString++) = (CHAR16) *(String++);
        len--;
    }
    *UniString = '\0';
}


EFI_STATUS
DisplayImage( EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop, 
              EFI_HANDLE *BmpBuffer)
{
    BMP_IMAGE_HEADER *BmpHeader = (BMP_IMAGE_HEADER *) BmpBuffer;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
    EFI_STATUS Status = EFI_SUCCESS;
    UINT8  *BitmapData;
    UINT32 *Palette;
    UINTN   Pixels;
    UINTN   XIndex;
    UINTN   YIndex;
    UINTN   Pos;
    UINTN   BltPos;

    BitmapData = (UINT8*)BmpBuffer + BmpHeader->ImageOffset;
    Palette    = (UINT32*) ((UINT8*)BmpBuffer + 0x36);

    Pixels = BmpHeader->PixelWidth * BmpHeader->PixelHeight;
    BltBuffer = AllocateZeroPool( sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * Pixels);
    if (BltBuffer == NULL) {
        Print(L"ERROR: BltBuffer. No memory resources\n");
        return EFI_OUT_OF_RESOURCES;
    }

    for (YIndex = BmpHeader->PixelHeight; YIndex > 0; YIndex--) {
        for (XIndex = 0; XIndex < BmpHeader->PixelWidth; XIndex++) {
            Pos    = (YIndex - 1) * ((BmpHeader->PixelWidth + 3) / 4) * 4 + XIndex;
            BltPos = (BmpHeader->PixelHeight - YIndex) * BmpHeader->PixelWidth + XIndex;
            BltBuffer[BltPos].Blue     = (UINT8) BitFieldRead32(Palette[BitmapData[Pos]], 0 , 7 );
            BltBuffer[BltPos].Green    = (UINT8) BitFieldRead32(Palette[BitmapData[Pos]], 8 , 15);
            BltBuffer[BltPos].Red      = (UINT8) BitFieldRead32(Palette[BitmapData[Pos]], 16, 23);
            BltBuffer[BltPos].Reserved = (UINT8) BitFieldRead32(Palette[BitmapData[Pos]], 24, 31);
        }
    }


    Status = Gop->Blt( Gop,
                       BltBuffer,
                       EfiBltBufferToVideo,
                       0, 0,            /* Source X, Y */
                       50, 50,          /* Dest X, Y */
                       BmpHeader->PixelWidth, BmpHeader->PixelHeight, 
                       0);

    FreePool(BltBuffer);

    return Status;
}


//
// Print the BMP header details
//
EFI_STATUS
PrintBMP( EFI_HANDLE *BmpBuffer)
{
    BMP_IMAGE_HEADER *BmpHeader = (BMP_IMAGE_HEADER *)BmpBuffer;
    EFI_STATUS Status = EFI_SUCCESS;
    CHAR16 Buffer[100];

    // not BMP format
    if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
        Print(L"ERROR: Unsupported image format\n"); 
        return EFI_UNSUPPORTED;
    }

    // BITMAPINFOHEADER format unsupported
    if (BmpHeader->HeaderSize != sizeof (BMP_IMAGE_HEADER) \
        - ((UINTN) &(((BMP_IMAGE_HEADER *)0)->HeaderSize))) {
        Print(L"ERROR: Unsupported BITMAPFILEHEADER\n");
        return EFI_UNSUPPORTED;
    }

    // compression type not 0
    if (BmpHeader->CompressionType != 0) {
        Print(L"ERROR: Compression type not 0\n");
        return EFI_UNSUPPORTED;
    }

    // unsupported bits per pixel
    if (BmpHeader->BitPerPixel != 4 &&
        BmpHeader->BitPerPixel != 8 &&
        BmpHeader->BitPerPixel != 12 &&
        BmpHeader->BitPerPixel != 24) {
        Print(L"ERROR: Bits per pixel is not one of 4, 8, 12 or 24\n");
        return EFI_UNSUPPORTED;
    }

    Print(L"\n");
    AsciiToUnicodeSize((CHAR8 *)BmpHeader, 2, Buffer);
    Print(L"BMP Signature     : %s\n", Buffer);
    Print(L"Size              : %d\n", BmpHeader->Size);
    Print(L"Image Offset      : %d\n", BmpHeader->ImageOffset);
    Print(L"Header Size       : %d\n", BmpHeader->HeaderSize);
    Print(L"Image Width       : %d\n", BmpHeader->PixelWidth);
    Print(L"Image Height      : %d\n", BmpHeader->PixelHeight);
    Print(L"Planes            : %d\n", BmpHeader->Planes);
    Print(L"Bit Per Pixel     : %d\n", BmpHeader->BitPerPixel);
    Print(L"Compression Type  : %d\n", BmpHeader->CompressionType);
    Print(L"Image Size        : %d\n", BmpHeader->ImageSize);
    Print(L"X Pixels Per Meter: %d\n", BmpHeader->XPixelsPerMeter);
    Print(L"Y Pixels Per Meter: %d\n", BmpHeader->YPixelsPerMeter);
    Print(L"Number of Colors  : %d\n", BmpHeader->NumberOfColors);
    Print(L"Important Colors  : %d\n", BmpHeader->ImportantColors);

    return Status;
}



static void
Usage(void)
{
    Print(L"Usage: DisplayBMP [-v | --verbose] [-l | --lowest] BMPfile\n"); 
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_HANDLE *HandleBuffer = NULL;
    UINTN HandleCount = 0;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
    SHELL_FILE_HANDLE FileHandle;
    EFI_FILE_INFO *FileInfo = NULL;
    EFI_HANDLE *FileBuffer = NULL;
    BOOLEAN Verbose = FALSE;
    UINTN FileSize;
    int OrgMode, NewMode = 0, Pixels = 0;
    BOOLEAN LowerHandle = FALSE;


    if (Argc == 1) {
        Usage();
        return Status;
    }

    for (int i = 1; i < Argc - 1; i++) {
        if (!StrCmp(Argv[i], L"--verbose") ||
            !StrCmp(Argv[i], L"-v")) {
            Verbose = TRUE;
        } else if (!StrCmp(Argv[i], L"--help") ||
            !StrCmp(Argv[i], L"-h") ||
            !StrCmp(Argv[i], L"-?")) {
            Usage();
            return Status;
        } else if (!StrCmp(Argv[i], L"--lowest") ||
            !StrCmp(Argv[i], L"-l")) {
            LowerHandle = TRUE;
        } else {
            Print(L"ERROR: Unknown option.\n");
            Usage();
            return Status;
        }
    }

    // Open the file ( has to be LAST arguement on command line )
    Status = ShellOpenFileByName( Argv[Argc - 1], 
                                  &FileHandle,
                                  EFI_FILE_MODE_READ , 0);
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: Could not open specified file [%d]\n", Status);
        return Status;
    }            

    // Allocate buffer for file contents
    FileInfo = ShellGetFileInfo(FileHandle);    
    FileBuffer = AllocateZeroPool( (UINTN)FileInfo -> FileSize);
    if (FileBuffer == NULL) {
        Print(L"ERROR: File buffer. No memory resources\n");
        return (SHELL_OUT_OF_RESOURCES);   
    }

    // Read file contents into allocated buffer
    FileSize = (UINTN) FileInfo->FileSize;
    Status = ShellReadFile(FileHandle, &FileSize, FileBuffer);
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: ShellReadFile failed [%d]\n", Status);
        goto cleanup;
    }            
  
    ShellCloseFile(&FileHandle);

    if (Verbose) {
         PrintBMP(FileBuffer);
         PressKey(TRUE); 
    }

    // Try locating GOP by handle
    Status = gBS->LocateHandleBuffer( ByProtocol,
                      &gEfiGraphicsOutputProtocolGuid,
                      NULL,
                      &HandleCount,
                      &HandleBuffer);
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: No GOP handles found via LocateHandleBuffer\n");
        goto cleanup;
    } else {
        Print(L"Found %d GOP handles via LocateHandleBuffer\n", HandleCount);
        if (LowerHandle)
            HandleCount = 0;
        else
            HandleCount--;

        Status = gBS->OpenProtocol( HandleBuffer[HandleCount],
                                    &gEfiGraphicsOutputProtocolGuid,
                                    (VOID **)&Gop,
                                    gImageHandle,
                                    NULL,
                                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if (EFI_ERROR (Status)) { 
            Print(L"ERROR: OpenProtocol [%d]\n", Status);
            goto cleanup;
        }

        FreePool(HandleBuffer);
    }

    // Figure out maximum resolution and use it
    OrgMode = Gop->Mode->Mode;
    for (int i = 0; i < Gop->Mode->MaxMode; i++) {
         EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
         UINTN SizeOfInfo;

         Status = Gop->QueryMode(Gop, i, &SizeOfInfo, &Info);
         if (EFI_ERROR(Status) && Status == EFI_NOT_STARTED) {
             Gop->SetMode(Gop, Gop->Mode->Mode);
             Status = Gop->QueryMode(Gop, i, &SizeOfInfo, &Info);
         }
         if (EFI_ERROR(Status)) {
             continue;
         }
         if (Info->PixelsPerScanLine > Pixels) {
              Pixels = Info->PixelsPerScanLine;
              NewMode = i;
         }
    }
   
    // change screen mode 
    Status = Gop->SetMode(Gop, NewMode);
    if (EFI_ERROR (Status)) { 
        Print(L"ERROR: SetMode [%d]\n, Status");
        goto cleanup;
    }
        
    DisplayImage(Gop, FileBuffer);

    // reset screen to original mode
    PressKey(FALSE);
    Status = Gop->SetMode(Gop, OrgMode);


cleanup:

    FreePool(FileBuffer);
    return Status;
}
