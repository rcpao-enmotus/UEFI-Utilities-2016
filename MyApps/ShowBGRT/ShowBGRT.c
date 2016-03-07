//
//  Copyright (c) 2015  Finnbarr P. Murphy.   All rights reserved.
//
//  Show BGRT info, save image to file if option selected
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
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>


#define EFI_ACPI_TABLE_GUID \
    { 0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d }}
#define EFI_ACPI_20_TABLE_GUID \
    { 0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }} 

 
// Boot Graphics Resource Table definition
typedef struct {
    EFI_ACPI_SDT_HEADER Header;
    UINT16 Version;
    UINT8 Status;
    UINT8 ImageType;
    UINT64 ImageAddress;
    UINT32 ImageOffsetX;
    UINT32 ImageOffsetY;
} EFI_ACPI_BGRT;

#define EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE_REVISION 1
#define EFI_ACPI_5_0_BGRT_VERSION              0x01
#define EFI_ACPI_5_0_BGRT_STATUS_NOT_DISPLAYED 0x00
#define EFI_ACPI_5_0_BGRT_STATUS_DISPLAYED     0x01
#define EFI_ACPI_5_0_BGRT_STATUS_INVALID       EFI_ACPI_5_0_BGRT_STATUS_NOT_DISPLAYED
#define EFI_ACPI_5_0_BGRT_STATUS_VALID         EFI_ACPI_5_0_BGRT_STATUS_DISPLAYED
#define EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP       0x00

typedef struct {
    CHAR8   CharB;
    CHAR8   CharM;
    UINT32  Size;
    UINT16  Reserved[2];
    UINT32  ImageOffset;    
    UINT32  HeaderSize;      
    UINT32  PixelWidth; 
    UINT32  PixelHeight; 
    UINT16  Planes; 
    UINT16  BitPerPixel;  
    UINT32  CompressionType;
    UINT32  ImageSize; 
    UINT32  XPixelsPerMeter;
    UINT32  YPixelsPerMeter;
    UINT32  NumberOfColors;
    UINT32  ImportantColors;
} __attribute__((__packed__)) BMP_IMAGE_HEADER;


int Verbose = 0;
int SaveImage = 0;


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



//
// Save Boot Logo image as a BMP file
//
EFI_STATUS 
SaveBMP( CHAR16 *FileName,
         UINT8 *FileData,
         UINTN FileDataLength)
{
    EFI_STATUS          Status;
    EFI_FILE_HANDLE     FileHandle;
    UINTN               BufferSize;
    EFI_FILE_PROTOCOL   *Root;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
    
    Status = gBS->LocateProtocol( &gEfiSimpleFileSystemProtocolGuid, 
                                  NULL,
                                  (VOID **)&SimpleFileSystem);
    if (EFI_ERROR(Status)) {
        Print(L"Cannot find EFI_SIMPLE_FILE_SYSTEM_PROTOCOL \r\n");
        return Status;    
    }

    Status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Volume open\n");
        return Status;    
    }

    Status = Root->Open( Root, 
                         &FileHandle, 
                         FileName,
                         EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 
                         0);
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: File open\n");
        return Status;
    }    
    
    BufferSize = FileDataLength;
    Status = FileHandle->Write(FileHandle, &BufferSize, FileData);
    FileHandle->Close(FileHandle);
    
    Print(L"Successfully saved bootlogo.bmp\n");

    return Status;
}


//
// Parse the in-memory BMP header
//
EFI_STATUS
ParseBMP(UINT64 BmpImage)
{
    BMP_IMAGE_HEADER *BmpHeader = (BMP_IMAGE_HEADER *)BmpImage;
    EFI_STATUS Status = EFI_SUCCESS;
    CHAR16 Buffer[100];

    if (Verbose) {
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
            Print(L"ERROR: Compression Type not 0\n");
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
    } // Verbose
    
    // save the boot logo to a file
    if (SaveImage) {
        Status = SaveBMP(L"bootlogo.bmp", (UINT8 *)BmpImage, BmpHeader->Size);
        if (EFI_ERROR(Status)) {
            Print(L"ERROR: Saving boot logo file: %x\n", Status);
        }
    }

    return EFI_SUCCESS;
}


//
// Parse Boot Graphic Resource Table
//
static VOID 
ParseBGRT(EFI_ACPI_BGRT *Bgrt)
{
    CHAR16 Buffer[100];

    Print(L"\n");
    AsciiToUnicodeSize((CHAR8 *)&(Bgrt->Header.Signature), 4, Buffer);
    Print(L"Signature         : %s\n", Buffer);
    Print(L"Length            : %d\n", Bgrt->Header.Length);
    Print(L"Revision          : %d\n", Bgrt->Header.Revision);
    Print(L"Checksum          : %d\n", Bgrt->Header.Checksum);
    AsciiToUnicodeSize((CHAR8 *)(Bgrt->Header.OemId), 6, Buffer);
    Print(L"Oem ID            : %s\n", Buffer);
    AsciiToUnicodeSize((CHAR8 *)(Bgrt->Header.OemTableId), 8, Buffer);
    Print(L"Oem Table ID      : %s\n", Buffer);
    Print(L"Oem Revision      : %d\n", Bgrt->Header.OemRevision);
    AsciiToUnicodeSize((CHAR8 *)&(Bgrt->Header.CreatorId), 4, Buffer);
    Print(L"Creator ID        : %s\n", Buffer);
    Print(L"Creator Revision  : %d\n", Bgrt->Header.CreatorRevision);
    Print(L"Version           : %d\n", Bgrt->Version);
    Print(L"Status            : %d", Bgrt->Status);
    if (Bgrt->Status == EFI_ACPI_5_0_BGRT_STATUS_NOT_DISPLAYED)
        Print(L" (Not displayed)");
    if (Bgrt->Status == EFI_ACPI_5_0_BGRT_STATUS_DISPLAYED)
        Print(L" (Displayed)");
    Print(L"\n"); 
    Print(L"Image Type        : %d", Bgrt->ImageType);
    if (Bgrt->ImageType == EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP)
        Print(L" (BMP format)");
    Print(L"\n"); 
    Print(L"Offset Y          : %ld\n", Bgrt->ImageOffsetY);
    Print(L"Offset X          : %ld\n", Bgrt->ImageOffsetX);

    if (Verbose) {
        Print(L"Physical Address  : %lld\n", Bgrt->ImageAddress);
    }

    ParseBMP(Bgrt->ImageAddress);
 
    Print(L"\n");
}


static int
ParseRSDP( EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp, CHAR16* GuidStr)
{
    EFI_ACPI_SDT_HEADER *Xsdt, *Entry;
    CHAR16 OemStr[20];
    UINT32 EntryCount;
    UINT64 *EntryPtr;

    AsciiToUnicodeSize((CHAR8 *)(Rsdp->OemId), 6, OemStr);
    if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) {
        Xsdt = (EFI_ACPI_SDT_HEADER *)(Rsdp->XsdtAddress);
    } else {
        return 1;
    }

    if (Xsdt->Signature != SIGNATURE_32 ('X', 'S', 'D', 'T')) {
        return 1;
    }

    AsciiToUnicodeSize((CHAR8 *)(Xsdt->OemId), 6, OemStr);
    EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);

    EntryPtr = (UINT64 *)(Xsdt + 1);
    for (int Index = 0; Index < EntryCount; Index++, EntryPtr++) {
        Entry = (EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr));
        if (Entry->Signature == SIGNATURE_32 ('B', 'G', 'R', 'T')) {
            ParseBGRT((EFI_ACPI_BGRT *)((UINTN)(*EntryPtr)));
        }
    }

    return 0;
}


static void
Usage(void)
{
    Print(L"Usage: ShowBGRT [-v|--verbose] [-s|--save]\n");
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_CONFIGURATION_TABLE *ect = gST->ConfigurationTable;
    EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp = NULL;
    EFI_GUID AcpiTableGuid = EFI_ACPI_TABLE_GUID;
    EFI_GUID Acpi20TableGuid = EFI_ACPI_20_TABLE_GUID;
    EFI_STATUS Status = EFI_SUCCESS;
    CHAR16 GuidStr[100];


    for (int i = 1; i < Argc; i++) {
        if (!StrCmp(Argv[i], L"--verbose") ||
            !StrCmp(Argv[i], L"-v")) {
            Verbose = 1;
        } else if (!StrCmp(Argv[i], L"--help") ||
            !StrCmp(Argv[i], L"-h") ||
            !StrCmp(Argv[i], L"-?")) {
            Usage();
            return Status;
        } else if (!StrCmp(Argv[i], L"--save") ||
            !StrCmp(Argv[1], L"-s")) {
            SaveImage = 1;
        } else {
            Print(L"ERROR: Unknown option.\n");
            Usage();
            return Status;
        }
    }

            
    // locate RSDP (Root System Description Pointer) 
    for (int i = 0; i < gST->NumberOfTableEntries; i++) {
	if ((CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &AcpiTableGuid)) ||
	    (CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &Acpi20TableGuid))) {
	    if (!AsciiStrnCmp("RSD PTR ", (CHAR8 *)(ect->VendorTable), 8)) {
		UnicodeSPrint(GuidStr, sizeof(GuidStr), L"%g", &(gST->ConfigurationTable[i].VendorGuid));
		Rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)ect->VendorTable;
			ParseRSDP(Rsdp, GuidStr); 
	    }        
	}
	ect++;
    }

    if (Rsdp == NULL) {
	if (Verbose) {
	    Print(L"ERROR: Could not find an ACPI RSDP table.\n");
	}
	return EFI_NOT_FOUND;
    }

    return Status;

}
