//
//  Copyright (c) 2016  Finnbarr P. Murphy.   All rights reserved.
//
//  Retrieve TPM 1.2 permanent flags
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
#include <Protocol/TcgService.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#define UTILITY_VERSION L"0.8"


VOID
Usage(CHAR16 *Str)
{
    Print(L"Usage: %s [--version]\n", Str);
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_TCG_PROTOCOL *TcgProtocol;
    EFI_GUID gEfiTcgProtocolGuid = EFI_TCG_PROTOCOL_GUID;

    TPM_RSP_COMMAND_HDR *TpmRsp;
    UINT32              TpmSendSize;
    TPM_PERMANENT_FLAGS *TpmPermanentFlags;
    UINT8               CmdBuf[64];


    if (Argc == 2) {
        if (!StrCmp(Argv[1], L"--version")) {
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

    Status = gBS->LocateProtocol( &gEfiTcgProtocolGuid, 
                                  NULL, 
                                  (VOID **) &TcgProtocol);
    if (EFI_ERROR (Status)) {
        Print(L"Failed to locate EFI_TCG_PROTOCOL [%d]\n", Status);
        return Status;
    }  

    TpmSendSize           = sizeof (TPM_RQU_COMMAND_HDR) + sizeof (UINT32) * 3;
    *(UINT16*)&CmdBuf[0]  = SwapBytes16 (TPM_TAG_RQU_COMMAND);
    *(UINT32*)&CmdBuf[2]  = SwapBytes32 (TpmSendSize);
    *(UINT32*)&CmdBuf[6]  = SwapBytes32 (TPM_ORD_GetCapability);
    *(UINT32*)&CmdBuf[10] = SwapBytes32 (TPM_CAP_FLAG);
    *(UINT32*)&CmdBuf[14] = SwapBytes32 (sizeof (TPM_CAP_FLAG_PERMANENT));
    *(UINT32*)&CmdBuf[18] = SwapBytes32 (TPM_CAP_FLAG_PERMANENT);

    Status = TcgProtocol->PassThroughToTpm( TcgProtocol,
                                            TpmSendSize,
                                            CmdBuf,
                                            sizeof (CmdBuf),
                                            CmdBuf);
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: PassThroughToTpm failed [%d]\n", Status);
        return Status;
    }
    TpmRsp = (TPM_RSP_COMMAND_HDR *) &CmdBuf[0];
    if ((TpmRsp->tag != SwapBytes16(TPM_TAG_RSP_COMMAND)) || (TpmRsp->returnCode != 0)) {
        Print(L"ERROR: TPM command result [%d]\n", SwapBytes16(TpmRsp->returnCode));
        return EFI_DEVICE_ERROR;
    }

    TpmPermanentFlags = (TPM_PERMANENT_FLAGS *) &CmdBuf[sizeof (TPM_RSP_COMMAND_HDR) + sizeof (UINT32)];

    Print(L"\nTPM Permanent Flags\n\n");
    Print(L"Disabled: %s\n", TpmPermanentFlags->disable ? L"Yes" : L"No");
    Print(L"Ownership: %s\n", TpmPermanentFlags->ownership ? L"Yes" : L"No");
    Print(L"Deactivated: %s\n", TpmPermanentFlags->deactivated ? L"Yes" : L"No");
    Print(L"ReadPubEK: %s\n", TpmPermanentFlags->readPubek ? L"Yes" : L"No");
    Print(L"DisableOwnerClear: %s\n", TpmPermanentFlags->disableOwnerClear ? L"Yes" : L"No");
    Print(L"AllowMaintenance: %s\n", TpmPermanentFlags->allowMaintenance ? L"Yes" : L"No");
    Print(L"PhysicalPresenceLifetimeLock: %s\n", TpmPermanentFlags->physicalPresenceLifetimeLock ? L"Yes" : L"No");
    Print(L"PhysicalPresenceHWEnable: %s\n", TpmPermanentFlags->physicalPresenceHWEnable ? L"Yes" : L"No");
    Print(L"PhysicalPresenceCMDEnable: %s\n", TpmPermanentFlags->physicalPresenceCMDEnable ? L"Yes" : L"No");
    Print(L"CEKPUsed: %s\n", TpmPermanentFlags->CEKPUsed ? L"Yes" : L"No");
    Print(L"TPMpost: %s\n", TpmPermanentFlags->TPMpost ? L"Yes" : L"No");
    Print(L"TPMpostLock: %s\n", TpmPermanentFlags->TPMpostLock ? L"Yes" : L"No");
    Print(L"FIPS: %s\n", TpmPermanentFlags->FIPS ? L"Yes" : L"No");
    Print(L"Operator: %s\n", TpmPermanentFlags->operator ? L"Yes" : L"No");
    Print(L"EnableRevokeEK: %s\n", TpmPermanentFlags->enableRevokeEK ? L"Yes" : L"No");
    Print(L"NvLocked: %s\n", TpmPermanentFlags->nvLocked ? L"Yes" : L"No");
    Print(L"ReadSRKPub: %s\n", TpmPermanentFlags->readSRKPub ? L"Yes" : L"No");
    Print(L"TpmEstablished: %s\n", TpmPermanentFlags->tpmEstablished ? L"Yes" : L"No");
    Print(L"MaintenanceDone: %s\n", TpmPermanentFlags->maintenanceDone ? L"Yes" : L"No");
    Print(L"DisableFullDALogicInfo: %s\n", TpmPermanentFlags->disableFullDALogicInfo ? L"Yes" : L"No");
    Print(L"\n");

    return Status;
}
