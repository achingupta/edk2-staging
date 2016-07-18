/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011, ARM Limited. All rights reserved.
  Copyright (c) 2016 HP Development Company, L.P.
  Copyright (c) 2016, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Pi/PiSmmCis.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>

#include <Protocol/MmConfiguration.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/DebugSupport.h> // for EFI_SYSTEM_CONTEXT

#include <Guid/SmramMemoryReserve.h>
#include <Guid/MpInformation.h>
EFI_STATUS
EFIAPI
ArmRegisterMmFoundationEntry (
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                  MmEntryPoint
  );

// TODO: Non standard flag defined by ARM TF to mark the SMRAM descriptor that
// contains the extents of the buffer to be used for communication with the
// normal world
#define EFI_SECURE_NS_MEM             0x00000080

//
// Private copy of the MM system table for future use
//
EFI_SMM_SYSTEM_TABLE2 *mSmst = NULL;

//
// Globals used to initialize the protocol
//
EFI_HANDLE            mMmCpuHandle = NULL;

// Descriptor with whereabouts of memory used for communication with the normal world
EFI_SMRAM_DESCRIPTOR  mNsCommBuffer;

MP_INFORMATION_HOB_DATA *mMpInformationHobData;

EFI_MM_CONFIGURATION_PROTOCOL mMmConfig = {
  ArmRegisterMmFoundationEntry
};

EFI_SMM_ENTRY_POINT     mMmEntryPoint = NULL;

EFI_STATUS
GetGuidedHobData (
  IN  VOID *HobList,
  IN  CONST EFI_GUID *HobGuid,
  OUT VOID **HobData)
{
  EFI_HOB_GUID_TYPE *Hob;

  if (!HobList || !HobGuid || !HobData)
    return EFI_INVALID_PARAMETER;

  Hob = GetNextGuidHob (HobGuid, HobList);
  if (!Hob)
    return EFI_NOT_FOUND;

  *HobData = GET_GUID_HOB_DATA (Hob);
  if (!HobData)
    return EFI_NOT_FOUND;

  return EFI_SUCCESS;
}

EFI_STATUS
PiMmCpuStandaloneMmInitialize (
  IN EFI_HANDLE         ImageHandle,  // not actual imagehandle
  IN EFI_SMM_SYSTEM_TABLE2   *SystemTable  // not actual systemtable
  )
{
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *SmramRangesHobData;
  EFI_CONFIGURATION_TABLE         *ConfigurationTable;
  MP_INFORMATION_HOB_DATA         *MpInformationHobData;
  EFI_SMRAM_DESCRIPTOR            *SmramRanges;
  EFI_STATUS                       Status;
  UINT32                           SmramRangeCount;
  UINT32                           MpInfoSize;
  UINTN                            Index;
  VOID                            *HobStart;

  ASSERT (SystemTable != NULL);
  mSmst = SystemTable;

  // publish the SMM config protocol so the SMM core can register its entry point
  Status = mSmst->SmmInstallProtocolInterface(&mMmCpuHandle,
    &gEfiMmConfigurationProtocolGuid, EFI_NATIVE_INTERFACE, &mMmConfig);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Retrieve the Hoblist from the SMST to extract the details of the NS
  // communication buffer that has been reserved by S-EL1/EL3
  ConfigurationTable = mSmst->SmmConfigurationTable;
  for (Index = 0; Index < mSmst->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gEfiHobListGuid, &(ConfigurationTable[Index].VendorGuid))) {
      break;
    }
  }

  // Bail out if the Hoblist could not be found
  // TODO: This could also mean that
  // the normal world will never interact synchronously with the MM environment
  if (Index >= mSmst->NumberOfTableEntries) {
    DEBUG ((EFI_D_INFO, "Hoblist not found - 0x%x\n", Index));
    return EFI_OUT_OF_RESOURCES;
  }

  HobStart = ConfigurationTable[Index].VendorTable;

  //
  // Extract the SMRAM ranges from the SMRAM descriptor HOB
  //
  Status = GetGuidedHobData (HobStart,
			     &gEfiSmmPeiSmramMemoryReserveGuid,
			     (VOID **) &SmramRangesHobData);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_INFO, "SmramRangesHobData extraction failed - 0x%x\n", Status));
    return Status;
  }

  SmramRanges = SmramRangesHobData->Descriptor;
  SmramRangeCount = SmramRangesHobData->NumberOfSmmReservedRegions;
  ASSERT (SmramRanges);
  ASSERT (SmramRangeCount);

  DEBUG ((EFI_D_INFO, "SmramRangeCount - 0x%x\n", SmramRangeCount));
  for (Index = 0; Index < SmramRangeCount; Index++) {
    DEBUG ((EFI_D_INFO, "SmramRanges[%d]: 0x%016lx - 0x%lx - 0x%lx\n", Index,
	    SmramRanges[Index].CpuStart,
	    SmramRanges[Index].PhysicalSize,
	    SmramRanges[Index].RegionState));
  }

  // Find the descriptor that contains the whereabouts of the buffer for
  // communication with the Normal world.
  for (Index = 0; Index < SmramRangeCount; Index++) {
     if (SmramRanges[Index].RegionState & EFI_SECURE_NS_MEM) {
	     break;
     }
  }

  DEBUG ((EFI_D_INFO, "mNsCommBuffer SmramRangeindex %d\n", Index));

  if (Index >= SmramRangeCount)
    return EFI_OUT_OF_RESOURCES;

  CopyMem(&mNsCommBuffer, &SmramRanges[Index], sizeof(EFI_SMRAM_DESCRIPTOR));
  DEBUG ((EFI_D_INFO, "mNsCommBuffer: 0x%016lx - 0x%lx\n",
	  mNsCommBuffer.CpuStart,
	  mNsCommBuffer.PhysicalSize));

  //
  // Extract the MP information from the Hoblist
  //
  Status = GetGuidedHobData (HobStart,
			     &gMpInformationHobGuid,
			     (VOID **) &MpInformationHobData);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_INFO, "MpInformationHob extraction failed - 0x%x\n", Status));
    return Status;
  }

  //
  // Allocate memory for the MP information and copy over the MP information
  // passed by Trusted Firmware. Use the number of processors passed in the HOB
  // to copy the processor information
  //
  MpInfoSize = sizeof (MP_INFORMATION_HOB_DATA) +
	  (sizeof(EFI_PROCESSOR_INFORMATION) *
	   MpInformationHobData->NumberOfProcessors);
  Status = mSmst->SmmAllocatePool(EfiRuntimeServicesData,
				  MpInfoSize,
				  (void **) &mMpInformationHobData);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_INFO, "mMpInformationHobData mem alloc failed - 0x%x\n", Status));
    return Status;
  }

  CopyMem(mMpInformationHobData, MpInformationHobData, MpInfoSize);

  // Print MP information
  DEBUG ((EFI_D_INFO, "mMpInformationHobData: 0x%016lx - 0x%lx\n",
	  mMpInformationHobData->NumberOfProcessors,
	  mMpInformationHobData->NumberOfEnabledProcessors));
  for (Index = 0; Index < mMpInformationHobData->NumberOfProcessors; Index++) {
    DEBUG ((EFI_D_INFO, "mMpInformationHobData[0x%lx]: %d, %d, %d\n",
	    mMpInformationHobData->ProcessorInfoBuffer[Index].ProcessorId,
	    mMpInformationHobData->ProcessorInfoBuffer[Index].Location.Package,
	    mMpInformationHobData->ProcessorInfoBuffer[Index].Location.Core,
	    mMpInformationHobData->ProcessorInfoBuffer[Index].Location.Thread));
  }


  return Status;
}

EFI_STATUS
EFIAPI
ArmRegisterMmFoundationEntry(
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                  MmEntryPoint
  ) {
  // store the entry point in a global
  mMmEntryPoint = MmEntryPoint;
  return EFI_SUCCESS;
}
