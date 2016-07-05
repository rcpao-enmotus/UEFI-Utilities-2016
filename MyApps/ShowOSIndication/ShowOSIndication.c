//
//  Copyright (c) 2016  Finnbarr P. Murphy.   All rights reserved.
//
//  Display UEFI OsIndications information
//
//  License: BSD License
//

#include <Uefi.h>

#include <Uefi/UefiSpec.h>
#include <Guid/GlobalVariable.h>

#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EfiShell.h>
#include <Protocol/LoadedImage.h>

#define UTILITY_VERSION L"0.1"
#undef DEBUG

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI                    0x0000000000000001
#define EFI_OS_INDICATIONS_TIMESTAMP_REVOCATION             0x0000000000000002
#define EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED  0x0000000000000004
#define EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED            0x0000000000000008
#define EFI_OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED     0x0000000000000010
#define EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY          0x0000000000000040

extern EFI_RUNTIME_SERVICES  *gRT;


VOID
Usage(CHAR16 *Str)
{
    Print(L"Usage: %s [-V|--version]\n", Str);
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{

    EFI_STATUS Status = EFI_SUCCESS;
    UINT64     OsIndicationSupport;
    UINT64     OsIndication;
    UINTN      DataSize;
    UINT32     Attributes;
    BOOLEAN    BootFwUi;
    BOOLEAN    PlatformRecovery;
    BOOLEAN    TimeStampRevocation;
    BOOLEAN    FileCapsuleDelivery;
    BOOLEAN    FMPCapsuleSupported; 
    BOOLEAN    CapsuleResultVariable;


    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version") || 
            !StrCmp(Argv[1], L"-V")) {
            Print(L"Version: %s\n", UTILITY_VERSION);
            return Status;
        }
        if (!StrCmp(Argv[1], L"--help") ||
            !StrCmp(Argv[1], L"-h") ||
            !StrCmp(Argv[1], L"-?")) {
            Usage(Argv[0]);
            return Status;
        }
    }

    // catchall for all other cases
    if (Argc > 1) {
        Usage(Argv[0]);
        return Status;
    }


    Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
    DataSize = sizeof(UINT64);

    Status = gRT->GetVariable( EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME,
                               &gEfiGlobalVariableGuid,
                               &Attributes,
                               &DataSize,
                               &OsIndicationSupport);
    if (Status == EFI_NOT_FOUND) {
        Print(L"ERROR: OSIndicationSupport variable not found\n");
        return Status;
    }

#ifdef DEBUG
    Print(L"OSIndicationSupport variable found: %016x\n", OsIndicationSupport );
#endif

    Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
    DataSize = sizeof(UINT64);

    Status = gRT->GetVariable( EFI_OS_INDICATIONS_VARIABLE_NAME,
                               &gEfiGlobalVariableGuid,
                               &Attributes,
                               &DataSize,
                               &OsIndication);
    if (Status == EFI_NOT_FOUND) {
        Print(L"ERROR: OSIndication variable not found\n");
        return Status;
    }

#ifdef DEBUG
    Print(L"OSIndication variable found: %016x\n", OsIndication);
#endif

    BootFwUi = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) != 0);
    TimeStampRevocation = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_TIMESTAMP_REVOCATION) != 0);
    FileCapsuleDelivery = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0);
    FMPCapsuleSupported  = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED) != 0);
    CapsuleResultVariable = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED) != 0);
    PlatformRecovery = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY) != 0);

    Print(L"\n");
    Print(L"                      Boot Firmware: %s\n", BootFwUi ? L"Yes" : L"No");
    Print(L"               Timestamp Revocation: %s\n", TimeStampRevocation ? L"Yes" : L"No");
    Print(L"    File Capsule Delivery Supported: %s\n", FileCapsuleDelivery ? L"Yes" : L"No");
    Print(L"              FMP Capsule Supported: %s\n", FMPCapsuleSupported ? L"Yes" : L"No");
    Print(L"  Capsule Result Variable Supported: %s\n", CapsuleResultVariable ? L"Yes" : L"No");
    Print(L"            Start Platform Recovery: %s\n", PlatformRecovery ? L"Yes" : L"No");
    Print(L"\n");

    return EFI_SUCCESS;
}
