//
//  Copyright (c) 2016  Finnbarr P. Murphy.   All rights reserved.
//
//  Display UEFI RNG (Random Number Generator) algorithm information
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
#include <Library/MemoryAllocationLib.h>

#include <Protocol/EfiShell.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/Rng.h>

#define UTILITY_VERSION L"0.1"
#undef DEBUG


//  from Microsoft document - not part of EDK2 at present!
#define EFI_RNG_SERVICE_BINDING_PROTOCOL_GUID \
    {0xe417a4a2, 0x0843, 0x4619, {0xbf, 0x11, 0x5c, 0xe8, 0x2a, 0xfc, 0xfc, 0x59}}

#define EFI_RNG_ALGORITHM_RAW_GUID \
    {0xe43176d7, 0xb6e8, 0x4827, {0xb7, 0x84, 0x7f, 0xfd, 0xc4, 0xb6, 0x85, 0x61 }}


EFI_GUID gEfiRngAlgorithmRawGuid = EFI_RNG_ALGORITHM_RAW_GUID;    
EFI_GUID gEfiRngAlgorithmSp80090Ctr256Guid = EFI_RNG_ALGORITHM_SP800_90_CTR_256_GUID;


VOID
PrintAlg(EFI_RNG_ALGORITHM *RngAlg) 
{
    Print (L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x   ", RngAlg->Data1,
            RngAlg->Data2, RngAlg->Data3, RngAlg->Data4[0], RngAlg->Data4[1],
            RngAlg->Data4[2], RngAlg->Data4[3], RngAlg->Data4[4],
            RngAlg->Data4[5], RngAlg->Data4[6], RngAlg->Data4[7]);

    if (CompareGuid(RngAlg, &gEfiRngAlgorithmRawGuid)) {
         Print(L"Raw");
    }

    if (CompareGuid(RngAlg, &gEfiRngAlgorithmSp80090Ctr256Guid)) {
         Print(L"NIST SP800-90 AES-CTR-256");
    }

    // Add other algorithm here when implemented in EDK2
}


EFI_STATUS
PrintDeviceRng( UINTN Index,
                EFI_RNG_PROTOCOL *Rng)
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_RNG_ALGORITHM RngAlgList[10];
    EFI_RNG_ALGORITHM *RngAlg;
    UINTN RngAlgCount = 0;
    UINTN RngAlgListSize = 0;
    UINT8 *Rand;
    UINTN RandSize = 32;

    Status = Rng->GetInfo( Rng, 
                           &RngAlgListSize,
                           RngAlgList);
    if (Status != EFI_BUFFER_TOO_SMALL) {
        Print (L"ERROR: Rng->GetInfo [%n]\n", Status);
        return Status;
    }

#ifdef DEBUG
    Print(L"Rng->GetInfo Size: %d %d\n", RngAlgListSize, sizeof(EFI_RNG_ALGORITHM));
#endif

    RngAlgCount = RngAlgListSize / sizeof(EFI_RNG_ALGORITHM);
    if (RngAlgCount == 0) {
        //
        // This is a hack for poor IBV firmware on Lenovo T450
        //
        Rand = AllocatePool(RandSize);
        if (Rand == NULL) {
            Print (L"ERROR: AllocatePool\n");
            return Status;
        }

        Status = Rng->GetRNG(Rng, NULL, RandSize, Rand);
        FreePool(Rand);
        if (EFI_ERROR (Status)) {
            Print (L"ERROR: GetRNG default failed [%d]", Status);
            return Status;
        }
        
        Print (L"Device: %d  Number of supported algorithms: %d\n", Index, 1);
        Print (L"\n    %d) ", 1);
        PrintAlg((EFI_RNG_ALGORITHM *)&gEfiRngAlgorithmRawGuid);
    } else {
        Print (L"Device: %d  Number of supported algorithms: %d\n", Index, RngAlgCount);

        Status = Rng->GetInfo (Rng, &RngAlgListSize, RngAlgList);
        if (EFI_ERROR (Status)) {
            Print (L"ERROR: GetInfo failed [%d]", Status);
            return Status;
        }

        for (int index = 0; index < RngAlgCount; index++) {
             RngAlg = (EFI_RNG_ALGORITHM *)(&RngAlgList[index]);
             Print (L"\n    %d) ", index);
             PrintAlg(RngAlg);
        }
    }
    
    Print (L"\n\n");

    return Status;
}


VOID
Usage(CHAR16 *Str)
{
    Print(L"Usage: %s [-V|--version]\n", Str);
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_GUID gEfiRngProtocolGuid = EFI_RNG_PROTOCOL_GUID;
    EFI_GUID gEfiRngServiceProtocolGuid = EFI_RNG_SERVICE_BINDING_PROTOCOL_GUID;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_RNG_PROTOCOL *Rng;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount = 0;


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

    // Try locating EFI_RNG_SERVICE_BINDING handles
    Status = gBS->LocateHandleBuffer( ByProtocol,
                                      &gEfiRngServiceProtocolGuid,
                                      NULL,
                                      &HandleCount,
                                      &HandleBuffer);
    if (EFI_ERROR (Status)) {
        Print(L"No EFI_RNG_SERVICE_BINDING_PROTOCOL handles found");
    } else {
        Print(L"RNG service binding protocol handles found [%d]", HandleCount);
    }
    Print(L"\n\n");


    // Try locating EFI_RNG_PROTOCOL handles
    Status = gBS->LocateHandleBuffer( ByProtocol,
                                      &gEfiRngProtocolGuid,
                                      NULL,
                                      &HandleCount,
                                      &HandleBuffer);
    if (EFI_ERROR (Status) || HandleCount == 0 ) {
        Print(L"ERROR: No EFI_RNG_PROTOCOL handles found\n");
        return Status;
    }

#ifdef DEBUG
    Print(L"RNG protocol found [%d]\n", HandleCount);
#endif

    for (int Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol( HandleBuffer[Index],
                                      &gEfiRngProtocolGuid,
                                      (VOID*) &Rng);
        if (!EFI_ERROR (Status)) {
            PrintDeviceRng(Index, Rng);
        } else {
            Print(L"ERROR: HandleProtocol [%d]\n", Status);
        }
    }
    FreePool(HandleBuffer);

    return EFI_SUCCESS;
} 
