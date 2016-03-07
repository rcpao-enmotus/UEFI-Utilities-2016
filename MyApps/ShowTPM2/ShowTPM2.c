//
//  Copyright (c) 2016  Finnbarr P. Murphy.   All rights reserved.
//
//  Show ACPI TPM2 table details
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


#pragma pack (1)
typedef struct _MY_EFI_TPM2_ACPI_TABLE {
    EFI_ACPI_SDT_HEADER  Header;
    UINT16               PlatformClass;
    UINT16               Reserved;
    UINT64               ControlAreaAddress;
    UINT32               StartMethod;
 // UINT8                PlatformSpecificParameters[];
} MY_EFI_TPM2_ACPI_TABLE;
#pragma pack()


#define EFI_ACPI_20_TABLE_GUID \
    { 0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }} 

 

int Verbose = 0;


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
// Print Start Method details
//
static VOID
PrintStartMethod( UINT32 StartMethod)
{
    Print(L"        Start Method : %d (", StartMethod);
    switch (StartMethod) {
        case 0:  Print(L"Not allowed");
                 break;
        case 1:  Print(L"vnedor specific legacy use");
                 break;
        case 2:  Print(L"ACPI start method");
                 break;
        case 3:
        case 4:
        case 5:  Print(L"Verndor specific legacy use");
                 break;
        case 6:  Print(L"Memory mapped I/O");
                 break;
        case 7:  Print(L"Command response buffer interface");
                 break;
        case 8:  Print(L"Command response buffer interface, ACPI start method");
                 break;
        default: Print(L"Reserved for future use");
    } 
    Print(L")\n"); 
}


//
// Parse and print TCM2 Table details
//
static VOID 
ParseTPM2(MY_EFI_TPM2_ACPI_TABLE *Tpm2)
{
    CHAR16 Buffer[100];
    UINT8  PlatformSpecificMethodsSize = Tpm2->Header.Length - 52;

    Print(L"\n");
    AsciiToUnicodeSize((CHAR8 *)&(Tpm2->Header.Signature), 4, Buffer);
    Print(L"           Signature : %s\n", Buffer);
    Print(L"              Length : %d\n", Tpm2->Header.Length);
    Print(L"            Revision : %d\n", Tpm2->Header.Revision);
    Print(L"            Checksum : %d\n", Tpm2->Header.Checksum);
    AsciiToUnicodeSize((CHAR8 *)(Tpm2->Header.OemId), 6, Buffer);
    Print(L"              Oem ID : %s\n", Buffer);
    AsciiToUnicodeSize((CHAR8 *)(Tpm2->Header.OemTableId), 8, Buffer);
    Print(L"        Oem Table ID : %s\n", Buffer);
    Print(L"        Oem Revision : %d\n", Tpm2->Header.OemRevision);
    AsciiToUnicodeSize((CHAR8 *)&(Tpm2->Header.CreatorId), 4, Buffer);
    Print(L"          Creator ID : %s\n", Buffer);
    Print(L"    Creator Revision : %d\n", Tpm2->Header.CreatorRevision);
    Print(L"      Platform Class : %d\n", Tpm2->PlatformClass);
    Print(L"Control Area Address : %lld\n", Tpm2->ControlAreaAddress);
    PrintStartMethod(Tpm2->StartMethod);
    Print(L"  Platform S.P. Size : %d\n", PlatformSpecificMethodsSize);
    Print(L"\n"); 
}


static int
ParseRSDP( EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp, 
           CHAR16* GuidStr)
{
    EFI_ACPI_SDT_HEADER *Xsdt, *Entry;
    CHAR16 OemStr[20];
    UINT32 EntryCount;
    UINT64 *EntryPtr;

    AsciiToUnicodeSize((CHAR8 *)(Rsdp->OemId), 6, OemStr);
    if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) {
        Xsdt = (EFI_ACPI_SDT_HEADER *)(Rsdp->XsdtAddress);
    } else {
	if (Verbose) {
	    Print(L"ERROR: Invalid RSDP revision number.\n");
	}
        return 1;
    }

    if (Xsdt->Signature != SIGNATURE_32 ('X', 'S', 'D', 'T')) {
	if (Verbose) {
	    Print(L"ERROR: XSDT table signature not found.\n");
	}
        return 1;
    }

    AsciiToUnicodeSize((CHAR8 *)(Xsdt->OemId), 6, OemStr);
    EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);

    EntryPtr = (UINT64 *)(Xsdt + 1);
    for (int Index = 0; Index < EntryCount; Index++, EntryPtr++) {
        Entry = (EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr));
        if (Entry->Signature == SIGNATURE_32 ('T', 'P', 'M', '2')) {
            ParseTPM2((MY_EFI_TPM2_ACPI_TABLE *)((UINTN)(*EntryPtr)));
        }
    }

    return 0;
}


static void
Usage(void)
{
    Print(L"Usage: ShowTPM2 [-v|--verbose]\n");
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_CONFIGURATION_TABLE *ect = gST->ConfigurationTable;
    EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp = NULL;
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
        } else {
            Print(L"ERROR: Unknown option.\n");
            Usage();
            return Status;
        }
    }

            
    // locate RSDP (Root System Description Pointer) 
    for (int i = 0; i < gST->NumberOfTableEntries; i++) {
	if (CompareGuid (&(gST->ConfigurationTable[i].VendorGuid), &Acpi20TableGuid)) {
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
