#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EfiShell.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/Tcg2Protocol.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#define EFI_TCG2_PROTOCOL_GUID \
  {0x607f766c, 0x7455, 0x42be, { 0x93, 0x0b, 0xe4, 0xd7, 0x6d, 0xb2, 0x72, 0x0f }}


CHAR16 *
ManufacturerStr(UINT32 ManufacturerID)
{
    static CHAR16 Mcode[5];

    Mcode[0] = (CHAR16) ((ManufacturerID & 0xff000000) >> 24);
    Mcode[1] = (CHAR16) ((ManufacturerID & 0x00ff0000) >> 16);
    Mcode[2] = (CHAR16) ((ManufacturerID & 0x0000ff00) >> 8);
    Mcode[3] = (CHAR16)  (ManufacturerID & 0x000000ff);
    Mcode[4] = (CHAR16) '\0';

    return Mcode;
}


INTN
EFIAPI
ShellAppMain(UINTN Argc, CHAR16 **Argv)
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_TCG2_PROTOCOL *Tcg2Protocol;
    EFI_TCG2_BOOT_SERVICE_CAPABILITY CapabilityData;
    EFI_GUID gEfiTcg2ProtocolGuid = EFI_TCG2_PROTOCOL_GUID;

    Status = gBS->LocateProtocol( &gEfiTcg2ProtocolGuid, 
                                  NULL, 
                                  (VOID **) &Tcg2Protocol);
    if (EFI_ERROR (Status)) {
        Print(L"Failed to locate EFI_TCG2_PROTOCOL [%d]\n", Status);
        return Status;
    }  

 
    CapabilityData.Size = (UINT8)sizeof(CapabilityData);
    Status = Tcg2Protocol->GetCapability(Tcg2Protocol, &CapabilityData);
    if (EFI_ERROR (Status)) {
        Print(L"ERROR: Tcg2Protocol GetCapacity [%d]\n", Status);
        return Status;
    }  

    Print(L"          Structure Version: %d.%d\n",
           CapabilityData.StructureVersion.Major,
           CapabilityData.StructureVersion.Minor);
    Print(L"           Protocol Version: %d.%d\n",
           CapabilityData.ProtocolVersion.Major,
           CapabilityData.ProtocolVersion.Minor);

    Print(L"  Supported Hash Algorithms: ");
    if ((CapabilityData.HashAlgorithmBitmap & EFI_TCG2_BOOT_HASH_ALG_SHA1) != 0) {
        Print(L"SHA1 ");
    }
    if ((CapabilityData.HashAlgorithmBitmap & EFI_TCG2_BOOT_HASH_ALG_SHA256) != 0) {
        Print(L"SHA256 ");
    }
    if ((CapabilityData.HashAlgorithmBitmap & EFI_TCG2_BOOT_HASH_ALG_SHA384) != 0) {
        Print(L"SHA384 ");
    }
    if ((CapabilityData.HashAlgorithmBitmap & EFI_TCG2_BOOT_HASH_ALG_SHA512) != 0) {
        Print(L"SHA512 ");
    }
    if ((CapabilityData.HashAlgorithmBitmap & EFI_TCG2_BOOT_HASH_ALG_SM3_256) != 0) {
        Print(L"SM3_256 ");
    }
    Print(L"\n");

    Print(L"Supported Event Log Formats: ");
    if ((CapabilityData.SupportedEventLogs & EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2) != 0) {
        Print(L"TCG_1.2 ");
    }
    if ((CapabilityData.SupportedEventLogs & EFI_TCG2_EVENT_LOG_FORMAT_TCG_2) != 0) {
        Print(L"TCG_2 ");
    }
    Print(L"\n");

    Print(L"           TPM Present Flag: ");
    if (CapabilityData.TPMPresentFlag) {
        Print(L"True\n");
    } else {
        Print(L"False\n");
    }

    Print(L"       Maximum Command Size: %d\n", CapabilityData.MaxCommandSize);
    Print(L"      Maximum Response Size: %d\n", CapabilityData.MaxResponseSize);

    Print(L"             Manufactuer ID: %s\n", ManufacturerStr(CapabilityData.ManufacturerID));

    Print(L"       Number of PCR Banks: %ld\n", CapabilityData.NumberOfPCRBanks);

    return Status;
}
