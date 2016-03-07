//
//  Copyright (c) 2015  Finnbarr P. Murphy.   All rights reserved.
//
//  Display MSDM Microsoft Windows License Key
//
//  License: BSD License
//

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>

#include <Protocol/EfiShell.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/AcpiSystemDescriptionTable.h>

#undef DEBUG

#define EFI_ACPI_TABLE_GUID \
    { 0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d }}
#define EFI_ACPI_20_TABLE_GUID \
    { 0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }} 

 
// Microsoft Data Management table structure
typedef struct {
    EFI_ACPI_SDT_HEADER Header;
    UINT32  SlsVersion;
    UINT32  SlsReserved;
    UINT32  SlsDataType;
    UINT32  SlsDataReserved;
    UINT32  SlsDataLength;
    CHAR8   ProductKey[30];
} EFI_ACPI_MSDM;


static VOID AsciiToUnicodeSize(CHAR8 *, UINT8, CHAR16 *);


static VOID
AsciiToUnicodeSize(CHAR8 *String, UINT8 length, CHAR16 *UniString)
{
    int len = length;
 
    while (*String != '\0' && len > 0) {
        *(UniString++) = (CHAR16) *(String++);
        len--;
    }
    *UniString = '\0';
}


static VOID 
ParseMSDM(EFI_ACPI_MSDM *Msdm, int verbose)
{
    CHAR16 Buffer[100];

    Print(L"\n");
    if (verbose) {
        AsciiToUnicodeSize((CHAR8 *)&(Msdm->Header.Signature), 4, Buffer);
        Print(L"Signature         : %s\n", Buffer);
        Print(L"Length            : %d\n", Msdm->Header.Length);
        Print(L"Revision          : %d\n", Msdm->Header.Revision);
        Print(L"Checksum          : %d\n", Msdm->Header.Checksum);
        AsciiToUnicodeSize((CHAR8 *)(Msdm->Header.OemId), 6, Buffer);
        Print(L"Oem ID            : %s\n", Buffer);
        AsciiToUnicodeSize((CHAR8 *)(Msdm->Header.OemTableId), 8, Buffer);
        Print(L"Oem Table ID      : %s\n", Buffer);
        Print(L"Oem Revision      : %d\n", Msdm->Header.OemRevision);
        AsciiToUnicodeSize((CHAR8 *)&(Msdm->Header.CreatorId), 4, Buffer);
        Print(L"Creator ID        : %s\n", Buffer);
        Print(L"Creator Revision  : %d\n", Msdm->Header.CreatorRevision);
        Print(L"SLS Version       : %d\n", Msdm->SlsVersion);
        Print(L"SLS Reserved      : %d\n", Msdm->SlsReserved);
        Print(L"SLS Data Type     : %d\n", Msdm->SlsDataType);
        Print(L"SLS Data Reserved : %d\n", Msdm->SlsDataReserved);
        Print(L"SLS Data Length   : %d\n", Msdm->SlsDataLength);
    }
    AsciiToUnicodeSize((CHAR8 *)(Msdm->ProductKey), 29, Buffer);
    if (verbose) {
        Print(L"Product Key       : %s\n", Buffer);
    } else {
        Print(L"%s\n", Buffer);
    }
    Print(L"\n");
}


static int
ParseRSDP( EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp, CHAR16* GuidStr, int verbose)
{
    EFI_ACPI_SDT_HEADER *Xsdt, *Entry;
    CHAR16 OemStr[20];
    UINT32 EntryCount;
    UINT64 *EntryPtr;

#ifdef DEBUG 
    Print(L"\n\nACPI GUID: %s\n", GuidStr);
#endif
    AsciiToUnicodeSize((CHAR8 *)(Rsdp->OemId), 6, OemStr);

#ifdef DEBUG 
    Print(L"\nFound RSDP. Version: %d  OEM ID: %s\n", (int)(Rsdp->Revision), OemStr);
#endif
    if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) {
        Xsdt = (EFI_ACPI_SDT_HEADER *)(Rsdp->XsdtAddress);
    } else {
#ifdef DEBUG 
        Print(L"ERROR: No ACPI XSDT table found.\n");
#endif
        return 1;
    }

    if (Xsdt->Signature != SIGNATURE_32 ('X', 'S', 'D', 'T')) {
#ifdef DEBUG 
        Print(L"ERROR: Invalid ACPI XSDT table found.\n");
#endif
        return 1;
    }

    AsciiToUnicodeSize((CHAR8 *)(Xsdt->OemId), 6, OemStr);
    EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
#ifdef DEBUG 
    Print(L"Found XSDT. OEM ID: %s  Entry Count: %d\n\n", OemStr, EntryCount);
#endif

    EntryPtr = (UINT64 *)(Xsdt + 1);
    for (int Index = 0; Index < EntryCount; Index++, EntryPtr++) {
        Entry = (EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr));
        if (Entry->Signature == SIGNATURE_32 ('M', 'S', 'D', 'M')) {
            ParseMSDM((EFI_ACPI_MSDM *)((UINTN)(*EntryPtr)), verbose);
        }
    }

    return 0;
}


static void
Usage(void)
{
    Print(L"Usage: ShowMSDM [-v|--verbose]\n");
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
    int Verbose = 0;


    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--verbose") ||
            !StrCmp(Argv[1], L"-v")) {
            Verbose = 1;
        }
        if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h") ||
            !StrCmp(Argv[1], L"-?")) {
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
                ParseRSDP(Rsdp, GuidStr, Verbose); 
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
