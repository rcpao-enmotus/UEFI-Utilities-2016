//
//  Copyright (c) 2015  Finnbarr P. Murphy.   All rights reserved.
//
//  Display ESRT entries 
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

#define ESRT_GUID { 0xb122a263, 0x3661, 0x4f68, { 0x99, 0x29, 0x78, 0xf8, 0xb0, 0xd6, 0x21, 0x80 }}

EFI_GUID EsrtGuid = ESRT_GUID;

typedef struct esrt {
    UINT32 fw_resource_count;
    UINT32 fw_resource_count_max;
    UINT64 fw_resource_version;
} __attribute__((__packed__)) esrt_t;

typedef struct esre1 {
    EFI_GUID fw_class;
    UINT32   fw_type;
    UINT32   fw_version;
    UINT32   lowest_supported_fw_version;
    UINT32   capsule_flags;
    UINT32   last_attempt_version;
    UINT32   last_attempt_status;
} __attribute__((__packed__)) esre1_t;

#define CAPSULE_FLAGS_PERSIST_ACROSS_RESET  0x00010000
#define CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE 0x00020000
#define CAPSULE_FLAGS_INITIATE_RESET        0x00040000


void
dump_esrt(VOID *data)
{
    esrt_t *esrt = data;
    esre1_t *esre1 = (esre1_t *)((UINT8 *)data + sizeof (*esrt));
    int i, ob;

    Print(L"Dumping ESRT found at 0x%x\n\n", data);
    Print(L"Firmware Resource Count: %d\n", esrt->fw_resource_count);
    Print(L"Firmware Resource Max Count: %d\n", esrt->fw_resource_count_max);
    Print(L"Firmware Resource Version: %ld\n\n", esrt->fw_resource_version);

    if (esrt->fw_resource_version != 1) {
        Print(L"ERROR: Unknown ESRT version: %d\n", esrt->fw_resource_version);
        return;
    }

    for (i = 0; i < esrt->fw_resource_count; i++) {
        ob = 0;
        Print(L"ENTRY NUMBER: %d\n", i);
        Print(L"Firmware Class GUID: %g\n", &esre1->fw_class);
        Print(L"Firmware Type: %d ", esre1->fw_type);
        switch (esre1->fw_type) {
            case 0:  Print(L"(Unknown)\n");
                     break;
            case 1:  Print(L"(System)\n");
                     break;
            case 2:  Print(L"(Device)\n");
                     break;
            case 3:  Print(L"(UEFI Driver)\n");
                     break;
            default: Print(L"\n");
        }
        Print(L"Firmware Version: 0x%08x\n", esre1->fw_version);
        Print(L"Lowest Supported Firmware Version: 0x%08x\n", esre1->lowest_supported_fw_version);
        Print(L"Capsule Flags: 0x%08x", esre1->capsule_flags);
        if ((esre1->capsule_flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET)) == CAPSULE_FLAGS_PERSIST_ACROSS_RESET) {
            if (!ob) {
                ob = 1;
                Print(L"(");
            }
            Print(L"PERSIST");
        }
        if ((esre1->capsule_flags & (CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) {
            if (!ob) {
                ob = 1;
                Print(L"(");
            } else 
                Print(L", ");
            Print(L"POPULATE");
        }
        if ((esre1->capsule_flags & (CAPSULE_FLAGS_INITIATE_RESET)) == CAPSULE_FLAGS_INITIATE_RESET) {
            if (!ob) {
                ob = 1;
                Print(L"(");
            } else 
                Print(L", ");
            Print(L"RESET");
        }
        if (ob) 
            Print(L")");
        Print(L"\n");
        Print(L"Last Attempt Version: 0x%08x\n", esre1->last_attempt_version);
        Print(L"Last Attempt Status: %d ", esre1->last_attempt_status);
        switch(esre1->last_attempt_status) {
            case 0:  Print(L"(Success)\n");
                     break;
            case 1:  Print(L"(Unsuccessful)\n");
                     break;
            case 2:  Print(L"(Insufficient Resources)\n");
                     break;
            case 3:  Print(L"(Incorrect version)\n");
                     break;
            case 4:  Print(L"(Invalid Format)\n");
                     break;
            case 5:  Print(L"(AC Power Issue)\n");
                     break;
            case 6:  Print(L"(Battery Power Issue)\n");
                     break;
            default: Print(L"\n");
        }
        Print(L"\n");
        esre1++;
    }
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_CONFIGURATION_TABLE *ect = gST->ConfigurationTable;

    for (int i = 0; i < gST->NumberOfTableEntries; i++) {
        if (!CompareMem(&ect->VendorGuid, &EsrtGuid, sizeof(EsrtGuid))) {
            dump_esrt(ect->VendorTable);
            return EFI_SUCCESS;
        }
        ect++;
        continue;
    }

    Print(L"No ESRT found\n");

    return EFI_SUCCESS;
}

