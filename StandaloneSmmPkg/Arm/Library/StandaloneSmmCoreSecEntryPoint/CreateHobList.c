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

extern EFI_HOB_HANDOFF_INFO_TABLE*
HobConstructor (
  IN VOID   *EfiMemoryBegin,
  IN UINTN  EfiMemoryLength,
  IN VOID   *EfiFreeMemoryBottom,
  IN VOID   *EfiFreeMemoryTop
  );

// GUID to identify HOB with whereabouts of communication buffer with Normal
// World
extern EFI_GUID gEfiStandaloneMmNonSecureBufferGuid;

// GUID to identify HOB where the entry point of the CPU driver will be
// populated to allow this entry point driver to invoke it upon receipt of an
// event
extern EFI_GUID gEfiArmTfCpuDriverEpDescriptorGuid;

//
// Cache copy of HobList pointer.
//
VOID *gHobList = NULL;

/**
  Use the boot information passed by privileged firmware to populate a HOB list
  suitable for consumption by the MM Core and drivers.

  @param  PayloadBootInfo    Boot information passed by privileged firmware

**/
VOID *
CreateHobListFromBootInfo (
  IN  OUT  PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT *CpuDriverEntryPoint,
  IN       EFI_SECURE_PARTITION_BOOT_INFO     *PayloadBootInfo
)
{
  EFI_HOB_HANDOFF_INFO_TABLE      *HobStart;
  EFI_RESOURCE_ATTRIBUTE_TYPE     Attributes;
  UINT32                          Index;
  UINT32                          BufferSize;
  UINT32                          Flags;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *SmramRangesHob;
  EFI_SMRAM_DESCRIPTOR            *SmramRanges;
  EFI_SMRAM_DESCRIPTOR            *NsCommBufSmramRange;
  MP_INFORMATION_HOB_DATA         *MpInformationHobData;
  EFI_PROCESSOR_INFORMATION       *ProcInfoBuffer;
  EFI_SECURE_PARTITION_CPU_INFO   *CpuInfo;
  ARM_TF_CPU_DRIVER_EP_DESCRIPTOR *CpuDriverEntryPointDesc;

  // Create a hoblist with a PHIT and EOH
  HobStart = HobConstructor ((VOID *) PayloadBootInfo->SpMemBase,
    (UINTN)  PayloadBootInfo->SpMemLimit - PayloadBootInfo->SpMemBase,
    (VOID *) PayloadBootInfo->SpHeapBase,
    (VOID *) (PayloadBootInfo->SpHeapBase + PayloadBootInfo->SpHeapSize));

  // Check that the Hoblist starts at the bottom of the Heap
  ASSERT (HobStart == (VOID *) PayloadBootInfo->SpHeapBase);

  // Update Global copy of Hob list pointer to allow the remaining Hob
  // manipulation code to work
  gHobList = HobStart;

  // TODO: Not building a stack HOB as expected by the PI spec. since the GUID
  // is not currently allocated.

  // Build a Boot Firmware Volume HOB
  BuildFvHob(PayloadBootInfo->SpImageBase, PayloadBootInfo->SpImageSize);

  // Build a resource descriptor Hob that describes the available physical
  // memory range
  Attributes =(
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_TESTED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
  );

  BuildResourceDescriptorHob (EFI_RESOURCE_SYSTEM_MEMORY,
    Attributes,
    (UINTN) PayloadBootInfo->SpMemBase,
    PayloadBootInfo->SpMemLimit - PayloadBootInfo->SpMemBase);

  // Find the size of the GUIDed HOB with MP information
  BufferSize = sizeof (MP_INFORMATION_HOB_DATA);
  BufferSize += sizeof (EFI_PROCESSOR_INFORMATION) * PayloadBootInfo->NumCpus;

  // Create a Guided MP information HOB to enable the ARM TF CPU driver to
  // perform per-cpu allocations.
  MpInformationHobData = BuildGuidHob(&gMpInformationHobGuid, BufferSize);

  // Populate the MP information HOB with the topology information passed by
  // privileged firmware
  MpInformationHobData->NumberOfProcessors = PayloadBootInfo->NumCpus;
  MpInformationHobData->NumberOfEnabledProcessors = PayloadBootInfo->NumCpus;
  ProcInfoBuffer = MpInformationHobData->ProcessorInfoBuffer;
  CpuInfo = PayloadBootInfo->CpuInfo;

  for (Index = 0; Index < PayloadBootInfo->NumCpus; Index++) {
    ProcInfoBuffer[Index].ProcessorId      = CpuInfo[Index].Mpidr;
    ProcInfoBuffer[Index].Location.Package = GET_CLUSTER_ID(CpuInfo[Index].Mpidr);
    ProcInfoBuffer[Index].Location.Core    = GET_CORE_ID(CpuInfo[Index].Mpidr);
    ProcInfoBuffer[Index].Location.Thread  = GET_CORE_ID(CpuInfo[Index].Mpidr);

    Flags = PROCESSOR_ENABLED_BIT | PROCESSOR_HEALTH_STATUS_BIT;
    if (CpuInfo[Index].Flags & CPU_INFO_FLAG_PRIMARY_CPU) {
      Flags |= PROCESSOR_AS_BSP_BIT;
    }
    ProcInfoBuffer[Index].StatusFlag = Flags;
  }

  // Create a Guided HOB to tell the ARM TF CPU driver the location and length
  // of the communication buffer shared with the Normal world.
  NsCommBufSmramRange = (EFI_SMRAM_DESCRIPTOR *) BuildGuidHob (&gEfiStandaloneMmNonSecureBufferGuid, sizeof(EFI_SMRAM_DESCRIPTOR));
  NsCommBufSmramRange->PhysicalStart = PayloadBootInfo->SpNsCommBufBase;
  NsCommBufSmramRange->CpuStart      = PayloadBootInfo->SpNsCommBufBase;
  NsCommBufSmramRange->PhysicalSize  = PayloadBootInfo->SpNsCommBufSize;
  NsCommBufSmramRange->RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // Create a Guided HOB to enable the ARM TF CPU driver to share its entry
  // point and populate it with the address of the shared buffer
  CpuDriverEntryPointDesc = (ARM_TF_CPU_DRIVER_EP_DESCRIPTOR *) BuildGuidHob (&gEfiArmTfCpuDriverEpDescriptorGuid, sizeof(ARM_TF_CPU_DRIVER_EP_DESCRIPTOR));

  *CpuDriverEntryPoint = NULL;
  CpuDriverEntryPointDesc->ArmTfCpuDriverEpPtr = CpuDriverEntryPoint;

  // Find the size of the GUIDed HOB with SRAM ranges
  BufferSize = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK);
  BufferSize += PayloadBootInfo->NumSpMemRegions *
    sizeof(EFI_SMRAM_DESCRIPTOR);

  // Create a GUIDed HOB with SRAM ranges
  SmramRangesHob = BuildGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid, BufferSize);

  // Fill up the number of SMRAM memory regions
  SmramRangesHob->NumberOfSmmReservedRegions = PayloadBootInfo->NumSpMemRegions;
  // Fill up the SMRAM ranges
  SmramRanges = &SmramRangesHob->Descriptor[0];

  // Base and size of memory occupied by the Standalone MM image
  SmramRanges[0].PhysicalStart = PayloadBootInfo->SpImageBase;
  SmramRanges[0].CpuStart      = PayloadBootInfo->SpImageBase;
  SmramRanges[0].PhysicalSize  = PayloadBootInfo->SpImageSize;
  SmramRanges[0].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // Base and size of buffer shared with privileged Secure world software
  SmramRanges[1].PhysicalStart = PayloadBootInfo->SpSharedBufBase;
  SmramRanges[1].CpuStart      = PayloadBootInfo->SpSharedBufBase;
  SmramRanges[1].PhysicalSize  = PayloadBootInfo->SpPcpuSharedBufSize * PayloadBootInfo->NumCpus;
  SmramRanges[1].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // Base and size of buffer used for synchronous communication with Normal
  // world software
  SmramRanges[2].PhysicalStart = PayloadBootInfo->SpNsCommBufBase;
  SmramRanges[2].CpuStart      = PayloadBootInfo->SpNsCommBufBase;
  SmramRanges[2].PhysicalSize  = PayloadBootInfo->SpNsCommBufSize;
  SmramRanges[2].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // Base and size of memory allocated for stacks for all cpus
  SmramRanges[3].PhysicalStart = PayloadBootInfo->SpStackBase;
  SmramRanges[3].CpuStart      = PayloadBootInfo->SpStackBase;
  SmramRanges[3].PhysicalSize  = PayloadBootInfo->SpPcpuStackSize * PayloadBootInfo->NumCpus;
  SmramRanges[3].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // Base and size of heap memory shared by all cpus
  SmramRanges[4].PhysicalStart = (EFI_PHYSICAL_ADDRESS) HobStart;
  SmramRanges[4].CpuStart      = (EFI_PHYSICAL_ADDRESS) HobStart;
  SmramRanges[4].PhysicalSize  = HobStart->EfiFreeMemoryBottom - (EFI_PHYSICAL_ADDRESS) HobStart;
  SmramRanges[4].RegionState   = EFI_CACHEABLE | EFI_ALLOCATED;

  // Base and size of heap memory shared by all cpus
  SmramRanges[5].PhysicalStart = HobStart->EfiFreeMemoryBottom;
  SmramRanges[5].CpuStart      = HobStart->EfiFreeMemoryBottom;
  SmramRanges[5].PhysicalSize  = HobStart->EfiFreeMemoryTop - HobStart->EfiFreeMemoryBottom;
  SmramRanges[5].RegionState   = EFI_CACHEABLE;

  return HobStart;
}
