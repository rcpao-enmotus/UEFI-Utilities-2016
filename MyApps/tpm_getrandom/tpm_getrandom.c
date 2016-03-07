//
//  Copyright (c) 2016  Finnbarr P. Murphy.   All rights reserved.
//
//  Demo accessing TPM 1.2 Random Number Generator 
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
#define NO_RANDOM_BYTES 20 

#pragma pack(1)

typedef struct {
    TPM_RQU_COMMAND_HDR Header;
    UINT32              BytesRequested;
} TPM_COMMAND;


typedef struct {
    TPM_RSP_COMMAND_HDR Header;
    UINT32              RandomBytesSize;
    UINT8               RandomBytes[NO_RANDOM_BYTES];
} TPM_RESPONSE;

#pragma pack()


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

    TPM_COMMAND  InBuffer;
    TPM_RESPONSE OutBuffer;
    UINT32       InBufferSize;
    UINT32       OutBufferSize;
    int          RandomBytesSize = 0;

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

    InBufferSize = sizeof(TPM_COMMAND);
    OutBufferSize = sizeof(TPM_RESPONSE);

    InBuffer.Header.tag       = SwapBytes16(TPM_TAG_RQU_COMMAND);
    InBuffer.Header.paramSize = SwapBytes32(InBufferSize);
    InBuffer.Header.ordinal   = SwapBytes32(TPM_ORD_GetRandom);
    InBuffer.BytesRequested   = SwapBytes32(NO_RANDOM_BYTES);

    Status = TcgProtocol->PassThroughToTpm( TcgProtocol,
                                            InBufferSize,
                                            (UINT8 *)&InBuffer,
                                            OutBufferSize,
                                            (UINT8 *)&OutBuffer);
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: PassThroughToTpm failed [%d]\n", Status);
        return Status;
    }

    if ((OutBuffer.Header.tag != SwapBytes16 (TPM_TAG_RSP_COMMAND)) || (OutBuffer.Header.returnCode != 0)) {
        Print(L"ERROR: TPM command result [%d]\n", SwapBytes32(OutBuffer.Header.returnCode));
        return EFI_DEVICE_ERROR;
    }

    RandomBytesSize = SwapBytes32(OutBuffer.RandomBytesSize);

    Print(L"Number of Random Bytes Requested: %d\n", SwapBytes32(InBuffer.BytesRequested));
    Print(L" Number of Random Bytes Received: %d\n", RandomBytesSize);
    Print(L"           Ramdom Bytes Received: ");
    for (int i = 0; i < RandomBytesSize; i++) {
        Print(L"%02x ", OutBuffer.RandomBytes[i]);
    }
    Print(L"\n");

    return Status;
}
