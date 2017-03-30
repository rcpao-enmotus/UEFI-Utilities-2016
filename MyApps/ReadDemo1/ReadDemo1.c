//
//  Copyright (c) 2017  Finnbarr P. Murphy.   All rights reserved.
//
//  Print contents of ASCII file  
//  Demonstrates uss of EFI_SHELL_PROTOCOL functions
//
//  License: BSD License
//


#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>


#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/EfiShell.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/PciRootBridgeIo.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>

#define UTILITY_VERSION L"0.8"
#define LINE_MAX  1024


VOID
Usage(CHAR16 *Str)
{
    Print(L"Usage: %s [--version | --help ]\n", Str);
    Print(L"       %s [--number] [--nocomment] filename\n", Str);
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_STATUS Status = EFI_SUCCESS;
    SHELL_FILE_HANDLE InFileHandle;
    CHAR16  *FileName;
    CHAR16  *FullFileName;
    CHAR16  *ReadLine;
    CHAR16  *Walker;
    BOOLEAN Ascii = TRUE;
    BOOLEAN NoComment = FALSE;
    BOOLEAN LineNumber = FALSE;
    UINTN   Size = LINE_MAX;
    UINTN   LineNo = 0;
    int     i;
 
    if (Argc < 2 || Argc > 4) {
        Usage(Argv[0]);
        return Status;
    }

    for (i = 1; i < Argc; i++) {
        Walker = (CHAR16 *)Argv[i];
        if (!StrCmp(Argv[i], L"--version") ||
            !StrCmp(Argv[i], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        } else if (!StrCmp(Argv[i], L"--help") ||
            !StrCmp(Argv[i], L"-h") ||
            !StrCmp(Argv[i], L"-?")) {
            Usage(Argv[0]);
            return Status;
        } else if (!StrCmp(Argv[i], L"--nocomment")) { 
            NoComment = TRUE;
        } else if (!StrCmp(Argv[i], L"--number")) { 
            LineNumber = TRUE;
        } else if (*Walker != L'-') {
            break;
        } else {
            Print(L"ERROR: Unknown option.\n");
            Usage(Argv[0]);
            return Status;
        }
    }


    FileName = AllocateCopyPool(StrSize(Argv[i]), Argv[i]);
    if (FileName == NULL) {
        Print(L"ERROR: Could not allocate memory\n");
        return (EFI_OUT_OF_RESOURCES);
    }

    FullFileName = ShellFindFilePath(FileName);
    if (FullFileName == NULL) {
        Print(L"ERROR: Could not find %s\n", FileName);
        Status = EFI_NOT_FOUND;
        goto Error;
    }

    // open the file
    Status = ShellOpenFileByName(FullFileName, &InFileHandle, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        Print(L"ERROR: Could not open file\n");
        goto Error;
    }

    // allocate a buffer to read lines into
    ReadLine = AllocateZeroPool(Size);
    if (ReadLine == NULL) {
        Print(L"ERROR: Could not allocate memory\n");
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
    }

    // read file line by line
    for (;!ShellFileHandleEof(InFileHandle); Size = 1024) {

         Status = ShellFileHandleReadLine(InFileHandle, ReadLine, &Size, TRUE, &Ascii);
         if (Status == EFI_BUFFER_TOO_SMALL) {
             Status = EFI_SUCCESS;
         }
         if (EFI_ERROR(Status)) {
             break;
         }

         LineNo++;

         // Skip comment lines
         if (ReadLine[0] == L'#' && NoComment) {
             continue;
         }

         if (LineNumber) {
            Print(L"%0.4d  ", LineNo);
         }
         Print(L"%s\n", ReadLine);
    }

Error:
    if (FileName != NULL) {
       FreePool(FileName);
    }
    if (FullFileName != NULL) {
       FreePool(FullFileName);
    }
    if (ReadLine != NULL) {
        FreePool(ReadLine);
    }
    if (InFileHandle != NULL) {
        ShellCloseFile(&InFileHandle);
    }

    return Status;
}
