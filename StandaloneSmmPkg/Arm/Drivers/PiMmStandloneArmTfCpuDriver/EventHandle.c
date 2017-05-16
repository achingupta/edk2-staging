/** @file

  Copyright (c) 2016 HP Development Company, L.P.
  Copyright (c) 2016-2017, ARM Limited. All rights reserved.

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
#include <Library/ArmSvcLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>

#include <Protocol/DebugSupport.h> // for EFI_SYSTEM_CONTEXT

#include <Guid/ZeroGuid.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/SmmUefiInfo.h>

#include <IndustryStandard/ArmStdSmc.h>

#include "PiMmStandloneArmTfCpuDriver.h"

EFI_STATUS
EFIAPI
MmFoundationEntryRegister(
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                  MmEntryPoint
  );

//
// On ARM platforms every event is expected to have a GUID associated with
// it. It will be used by the MM Entry point to find the handler for the
// event. It will either be populated in a EFI_SMM_COMMUNICATE_HEADER by the
// caller of the event (e.g. MM_COMMUNICATE SMC) or by the CPU driver
// (e.g. during an asynchronous event). In either case, this context is
// maintained in an array which has an entry for each CPU. The pointer to this
// array is held in PerCpuGuidedEventContext. Memory is allocated once the
// number of CPUs in the system are made known through the
// MP_INFORMATION_HOB_DATA.
//
EFI_SMM_COMMUNICATE_HEADER **PerCpuGuidedEventContext = NULL;

//
// When an event is received by the CPU driver, it could correspond to a unique
// GUID (e.g. interrupt events) or to multiple GUIDs (e.g. MM_COMMUNICATE
// SMC). A table is used by the CPU driver to find the GUID corresponding to the
// event id in case there is a 1:1 mapping between the two. If an event id has
// multiple GUIDs associated with it then such an entry will not be found in
// this table.
//
// TODO: Currently NULL since there are no asynchronous events
static EFI_GUID *EventIdToGuidLookupTable = NULL;

// Descriptor with whereabouts of memory used for communication with the normal world
EFI_SMRAM_DESCRIPTOR  mNsCommBuffer;

MP_INFORMATION_HOB_DATA *mMpInformationHobData;

EFI_MM_CONFIGURATION_PROTOCOL mMmConfig = {
  MmFoundationEntryRegister
};

static EFI_SMM_ENTRY_POINT     mMmEntryPoint = NULL;

EFI_STATUS
PiMmStandloneArmTfCpuDriverEntry (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  )
{
  EFI_SMM_COMMUNICATE_HEADER *GuidedEventContext = NULL;
  EFI_SMM_ENTRY_CONTEXT       MmEntryPointContext = {0};
  EFI_STATUS                  Status;
  BOOLEAN                     UniqueGuidedEvent = FALSE;
  UINTN                       NsCommBufferSize;

  DEBUG ((EFI_D_INFO, "Received event - 0x%x on cpu %d\n", EventId, CpuNumber));

  // Check whether any handler for this event has been registered
  if (!EventIdInfo[EventId].HandlerCount)
    return EFI_INVALID_PARAMETER;

  // Check if this event corresponds to a single Guided handler or has multiple
  // handlers registered.
  // TODO: Remove assumptions that:
  //       1. events with a 1:1 mapping to a GUID do not have arguments that
  //          need to be copied into this translation regime.
  //       2. events with a 1:N mapping with GUIDs always have a buffer address
  //          and size in X1 and X2
  if (EventIdToGuidLookupTable)
    UniqueGuidedEvent = !CompareGuid(&EventIdToGuidLookupTable[EventId], &gZeroGuid);

  if (UniqueGuidedEvent) {
    // Found a GUID, allocate memory to populate a communication buffer
    // with the GUID in it
    Status = mSmst->SmmAllocatePool(EfiRuntimeServicesData, sizeof(EFI_SMM_COMMUNICATE_HEADER), (VOID **) &GuidedEventContext);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_INFO, "Mem alloc failed - 0x%x\n", EventId));
      return Status;
    }

    // Copy the GUID
    CopyGuid(&GuidedEventContext->HeaderGuid, &EventIdToGuidLookupTable[EventId]);

    // Message Length is 0 'cause of the assumption mentioned above
    GuidedEventContext->MessageLength = 0;
  } else {
    // TODO: Perform parameter validation of NsCommBufferAddr

    // This event id is the parent of multiple GUIDed handlers. Retrieve
    // the specific GUID from the communication buffer passed by the
    // caller.

    if (NsCommBufferAddr && (NsCommBufferAddr < mNsCommBuffer.PhysicalStart))
      return EFI_INVALID_PARAMETER;

    // Find out the size of the buffer passed
    NsCommBufferSize = ((EFI_SMM_COMMUNICATE_HEADER *) NsCommBufferAddr)->MessageLength;

    // Alternative approach in case EL3 has preallocated the non-secure
    // buffer. MM Foundation is told about the buffer through the Hoblist
    // and is responsible for performing the bounds check.
    if (NsCommBufferAddr + NsCommBufferSize >=
      mNsCommBuffer.PhysicalStart + mNsCommBuffer.PhysicalSize)
        return EFI_INVALID_PARAMETER;


    // Now that the secure world can see the normal world buffer, allocate
    // memory to copy the communication buffer to the secure world.
    Status = mSmst->SmmAllocatePool(EfiRuntimeServicesData, NsCommBufferSize, (VOID **) &GuidedEventContext);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_INFO, "Mem alloc failed - 0x%x\n", EventId));
      // TODO: Unmap secure memory before exiting to the normal world
      return Status;
    }

    // X1 contains the VA of the normal world memory accessible from
    // S-EL0
    CopyMem(GuidedEventContext, (CONST VOID *) NsCommBufferAddr, NsCommBufferSize);
  }

  // Stash the pointer to the allocated Event Context for this CPU
  PerCpuGuidedEventContext[CpuNumber] = GuidedEventContext;

  // TODO: Populate entire entry point context with valid information
  MmEntryPointContext.CurrentlyExecutingCpu = CpuNumber;
  MmEntryPointContext.NumberOfCpus = mMpInformationHobData->NumberOfProcessors;

  // Populate the MM system table with MP and state information
  mSmst->CurrentlyExecutingCpu = CpuNumber;
  mSmst->NumberOfCpus = mMpInformationHobData->NumberOfProcessors;
  mSmst->CpuSaveStateSize = 0;
  mSmst->CpuSaveState = NULL;

  mMmEntryPoint(&MmEntryPointContext);

  // Free the memory allocation done earlier and reset the per-cpu context
  // TODO: Check for the return status of the FreePool API
  ASSERT (GuidedEventContext);
  mSmst->SmmFreePool((VOID *) GuidedEventContext);
  PerCpuGuidedEventContext[CpuNumber] = NULL;

  return Status;
}

EFI_STATUS
EFIAPI
MmFoundationEntryRegister(
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                  MmEntryPoint
  ) {
  // store the entry point in a global
  mMmEntryPoint = MmEntryPoint;
  return EFI_SUCCESS;
}

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
PiMmCpuTpFwRootMmiHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS Status;
  UINTN      CpuNumber;

  ASSERT (Context == NULL);
  ASSERT (CommBuffer == NULL);
  ASSERT (CommBufferSize == NULL);

  CpuNumber = mSmst->CurrentlyExecutingCpu;
  if (!PerCpuGuidedEventContext[CpuNumber])
    return EFI_NOT_FOUND;

  DEBUG ((EFI_D_INFO, "CommBuffer - 0x%x, CommBufferSize - 0x%x\n",
          PerCpuGuidedEventContext[CpuNumber],
	  PerCpuGuidedEventContext[CpuNumber]->MessageLength));

  Status = mSmst->SmiManage(&PerCpuGuidedEventContext[CpuNumber]->HeaderGuid,
                     NULL,
                     PerCpuGuidedEventContext[CpuNumber]->Data,
                     &PerCpuGuidedEventContext[CpuNumber]->MessageLength);

  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_WARN, "Unable to manage Guided Event - %d\n", Status));
  }

  return Status;
}
