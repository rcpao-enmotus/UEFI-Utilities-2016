//
//  Copyright (c) 2015  Finnbarr P. Murphy.   All rights reserved.
//
//  Display EFI Configuration Table entries 
//
//  License: BSD License
//

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EfiShell.h>
#include <Protocol/LoadedImage.h>


// per UEFI Specification 2.5
#define EFI_ACPI_20_TABLE_GUID  \
   {0x8868e871, 0xe4f1, 0x11d3, {0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}}
#define ACPI_TABLE_GUID \
   {0xeb9d2d30, 0x2d88, 0x11d3, {0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}}
#define SAL_SYSTEM_TABLE_GUID \
   {0xeb9d2d32, 0x2d88, 0x11d3, {0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}}
#define SMBIOS_TABLE_GUID \
   {0xeb9d2d31, 0x2d88, 0x11d3, {0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}}
#define SMBIOS3_TABLE_GUID \
   {0xf2fd1544, 0x9794, 0x4a2c, {0x99,0x2e,0xe5,0xbb,0xcf,0x20,0xe3,0x94}}
#define MPS_TABLE_GUID \
   {0xeb9d2d2f, 0x2d88, 0x11d3, {0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d}}
#define EFI_PROPERTIES_TABLE_GUID \
   {0x880aaca3, 0x4adc, 0x4a04, {0x90,0x79,0xb7,0x47,0x34,0x8,0x25,0xe5}}
#define EFI_SYSTEM_RESOURCES_TABLE_GUID \
   {0xb122a263, 0x3661, 0x4f68, {0x99,0x29,0x78,0xf8,0xb0,0xd6,0x21,0x80}}
#define EFI_SECTION_TIANO_COMPRESS_GUID \
   {0xa31280ad, 0x481e, 0x41b6, {0x95,0xe8,0x12,0x7f,0x4c,0x98,0x47,0x79}}
#define EFI_SECTION_LZMA_COMPRESS_GUID  \
   {0xee4e5898, 0x3914, 0x4259, {0x9d,0x6e,0xdc,0x7b,0xd7,0x94,0x03,0xcf}}
#define EFI_DXE_SERVICES_TABLE_GUID \
   {0x5ad34ba, 0x6f02, 0x4214, {0x95,0x2e,0x4d,0xa0,0x39,0x8e,0x2b,0xb9}}
#define EFI_HOB_LIST_GUID \
   {0x7739f24c, 0x93d7, 0x11d4, {0x9a,0x3a,0x00,0x90,0x27,0x3f,0xc1,0x4d}}
#define MEMORY_TYPE_INFORMATION_GUID \
   {0x4c19049f, 0x4137, 0x4dd3, {0x9c,0x10,0x8b,0x97,0xa8,0x3f,0xfd,0xfa}}

static EFI_GUID Guid1  = EFI_ACPI_20_TABLE_GUID;
static EFI_GUID Guid2  = ACPI_TABLE_GUID;
static EFI_GUID Guid3  = SAL_SYSTEM_TABLE_GUID;
static EFI_GUID Guid4  = SMBIOS_TABLE_GUID;
static EFI_GUID Guid5  = SMBIOS3_TABLE_GUID;
static EFI_GUID Guid6  = MPS_TABLE_GUID;
static EFI_GUID Guid7  = EFI_PROPERTIES_TABLE_GUID;
static EFI_GUID Guid8  = EFI_SYSTEM_RESOURCES_TABLE_GUID;
static EFI_GUID Guid9  = EFI_SECTION_TIANO_COMPRESS_GUID;
static EFI_GUID Guid10 = EFI_SECTION_LZMA_COMPRESS_GUID;
static EFI_GUID Guid11 = EFI_DXE_SERVICES_TABLE_GUID;
static EFI_GUID Guid12 = EFI_HOB_LIST_GUID;
static EFI_GUID Guid13 = MEMORY_TYPE_INFORMATION_GUID;

static struct {
    EFI_GUID  *Guid;
    CHAR16    *GuidStr;
} KnownGuids[] = {
    { &Guid1,  L"ACPI 2.0 table" },
    { &Guid2,  L"ACPI 1.0 table" },
    { &Guid3,  L"SAL system table" },
    { &Guid4,  L"SMBIOS table" },
    { &Guid5,  L"SMBIOS 3 table" },
    { &Guid6,  L"MPS table" },
    { &Guid7,  L"EFI properties table" },
    { &Guid8,  L"ESRT table" },
    { &Guid9,  L"Tiano compress" },
    { &Guid10, L"LZMA compress" },
    { &Guid11, L"DXE services" },
    { &Guid12, L"HOB list" },
    { &Guid13, L"Memory type information" },
    { NULL }
};


static void
Usage(void)
{
    Print(L"Usage: ShowECT [-v|--verbose]\n");
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_CONFIGURATION_TABLE *ect = gST->ConfigurationTable;
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
            return EFI_SUCCESS;
        }
    }


    for (int i = 0; i < gST->NumberOfTableEntries; i++) {
        Print(L"%03d  %g", i, ect->VendorGuid);
        if (Verbose) {
            for (int j=0; KnownGuids[j].Guid; j++) {
                if (!CompareMem(&ect->VendorGuid, KnownGuids[j].Guid, sizeof(Guid1))) {
                    Print(L"  %s", KnownGuids[j].GuidStr);
                    break;
                }
            }
        }
        Print(L"\n");
        ect++;
    }

    return EFI_SUCCESS;
}

