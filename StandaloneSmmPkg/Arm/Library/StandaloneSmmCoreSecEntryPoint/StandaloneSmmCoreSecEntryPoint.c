/** @file
  Entry point to the Standalone MM Foundation when initialised during the SEC
  phase on ARM platforms

Copyright (c) 2017, ARM Ltd. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiSmm.h>

#include <Arm/Include/Library/StandaloneSmmCoreSecEntryPoint.h>

#include <PiPei.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/MpInformation.h>

#include <Library/ArmMmuLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>

#include <IndustryStandard/ArmStdSmc.h>

/**
  Retrieve a pointer to and print the boot information passed by privileged
  secure firmware

  @param  SharedBufAddress The pointer memory shared with privileged firmware

**/
EFI_SPM_PAYLOAD_BOOT_INFO *
GetAndPrintBootinformation (
  IN VOID                      *SharedBufAddress
)
{
  EFI_SPM_PAYLOAD_BOOT_INFO *PayloadBootInfo;
  EFI_SPM_PAYLOAD_CPU_INFO  *PayloadCpuInfo;
  UINTN                     Index;

  PayloadBootInfo = (EFI_SPM_PAYLOAD_BOOT_INFO *) SharedBufAddress;

  DEBUG((EFI_D_INFO, "NumSpMemRegions - 0x%x\n", PayloadBootInfo->NumSpMemRegions));
  DEBUG((EFI_D_INFO, "SpMemBase       - 0x%lx\n", PayloadBootInfo->SpMemBase));
  DEBUG((EFI_D_INFO, "SpMemLimit      - 0x%lx\n", PayloadBootInfo->SpMemLimit));
  DEBUG((EFI_D_INFO, "SpImageBase     - 0x%lx\n", PayloadBootInfo->SpImageBase));
  DEBUG((EFI_D_INFO, "SpStackBase     - 0x%lx\n", PayloadBootInfo->SpStackBase));
  DEBUG((EFI_D_INFO, "SpHeapBase      - 0x%lx\n", PayloadBootInfo->SpHeapBase));
  DEBUG((EFI_D_INFO, "SpNsCommBufBase - 0x%lx\n", PayloadBootInfo->SpNsCommBufBase));
  DEBUG((EFI_D_INFO, "SpSharedBufBase - 0x%lx\n", PayloadBootInfo->SpSharedBufBase));

  DEBUG((EFI_D_INFO, "SpImageSize     - 0x%x\n", PayloadBootInfo->SpImageSize));
  DEBUG((EFI_D_INFO, "SpPcpuStackSize - 0x%x\n", PayloadBootInfo->SpPcpuStackSize));
  DEBUG((EFI_D_INFO, "SpHeapSize      - 0x%x\n", PayloadBootInfo->SpHeapSize));
  DEBUG((EFI_D_INFO, "SpNsCommBufSize - 0x%x\n", PayloadBootInfo->SpNsCommBufSize));
  DEBUG((EFI_D_INFO, "SpSharedBufSize - 0x%x\n", PayloadBootInfo->SpSharedBufSize));

  DEBUG((EFI_D_INFO, "NumCpus         - 0x%x\n", PayloadBootInfo->NumCpus));
  DEBUG((EFI_D_INFO, "CpuInfo         - 0x%p\n", PayloadBootInfo->CpuInfo));

  PayloadCpuInfo = (EFI_SPM_PAYLOAD_CPU_INFO *) PayloadBootInfo->CpuInfo;

  for (Index = 0; Index < PayloadBootInfo->NumCpus; Index++) {
    DEBUG((EFI_D_INFO, "Mpidr           - 0x%lx\n", PayloadCpuInfo[Index].Mpidr));
    DEBUG((EFI_D_INFO, "LinearId        - 0x%x\n", PayloadCpuInfo[Index].LinearId));
    DEBUG((EFI_D_INFO, "Flags           - 0x%x\n", PayloadCpuInfo[Index].Flags));
  }

  return PayloadBootInfo;
}

/**
  The entry point of Standalone MM Foundation.

  @param  HobStart  The pointer to the beginning of the HOB List.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN VOID    *SharedBufAddress,
  IN UINT64  SharedBufSize,
  IN UINT64  cookie1,
  IN UINT64  cookie2
  )
{
  PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT      CpuDriverEntryPoint = NULL;
  PE_COFF_LOADER_IMAGE_CONTEXT            ImageContext;
  EFI_SPM_PAYLOAD_BOOT_INFO               *PayloadBootInfo;
  ARM_SVC_ARGS                            InitMmFoundationSvcArgs = {0};
  EFI_STATUS                              Status;
  UINT32                                  SectionHeaderOffset;
  UINTN                                   NumberOfSections;
  VOID                                    *HobStart;
  VOID                                    *TeData;
  UINTN                                   TeDataSize;

  Status = SerialPortInitialize ();
  ASSERT_EFI_ERROR (Status);

  PayloadBootInfo = GetAndPrintBootinformation(SharedBufAddress);
  ASSERT_EFI_ERROR (PayloadBootInfo);

  // Locate PE/COFF File information for the Standalone MM core module
  Status = LocateStandaloneSmmCorePeCoffData (
            (EFI_FIRMWARE_VOLUME_HEADER *) PayloadBootInfo->SpImageBase,
	    &TeData,
	    &TeDataSize);
  if (EFI_ERROR(Status)) {
    goto finish;
  }

  // Obtain the PE/COFF Section information for the Standalone MM core module
  Status = GetStandaloneSmmCorePeCoffSections (
            TeData,
            &ImageContext,
            &SectionHeaderOffset,
            &NumberOfSections);
  if (EFI_ERROR(Status)) {
    goto finish;
  }

  // Update the memory access permissions of individual sections in the
  // Standalone MM core module
  Status = UpdateMmFoundationPeCoffPermissions(
            &ImageContext,
            SectionHeaderOffset,
            NumberOfSections,
            ArmSetMemoryRegionNoExec,
            ArmSetMemoryRegionReadOnly,
            ArmClearMemoryRegionReadOnly);
  if (EFI_ERROR(Status)) {
    goto finish;
  }

  //
  // Create Hoblist based upon boot information passed by privileged software
  //
  HobStart = CreateHobListFromBootInfo(&CpuDriverEntryPoint, PayloadBootInfo);

  //
  // Call the SMM Core entry point
  //
  ProcessModuleEntryPointList (HobStart);

  ASSERT_EFI_ERROR (CpuDriverEntryPoint);
  DEBUG ((EFI_D_INFO, "Shared Cpu Driver EP 0x%lx\n", (UINT64) CpuDriverEntryPoint));

finish:
  InitMmFoundationSvcArgs.Arg0 = ARM_SVC_ID_SP_EVENT_COMPLETE_AARCH64;
  InitMmFoundationSvcArgs.Arg1 = Status;
  ArmCallSvc(&InitMmFoundationSvcArgs);

  DEBUG ((EFI_D_INFO, "Received delegated event\n"));
  DEBUG ((EFI_D_INFO, "X0 :  0x%x\n", (UINT32) InitMmFoundationSvcArgs.Arg0));
  DEBUG ((EFI_D_INFO, "X1 :  0x%x\n", (UINT32) InitMmFoundationSvcArgs.Arg1));
  DEBUG ((EFI_D_INFO, "X2 :  0x%x\n", (UINT32) InitMmFoundationSvcArgs.Arg2));
  DEBUG ((EFI_D_INFO, "X3 :  0x%x\n", (UINT32) InitMmFoundationSvcArgs.Arg3));

  Status = CpuDriverEntryPoint(
    InitMmFoundationSvcArgs.Arg0,
    InitMmFoundationSvcArgs.Arg3,
    InitMmFoundationSvcArgs.Arg1);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Failed delegated event 0x%x, Status 0x%x\n", InitMmFoundationSvcArgs.Arg0, Status));
  }

  goto finish;
}
