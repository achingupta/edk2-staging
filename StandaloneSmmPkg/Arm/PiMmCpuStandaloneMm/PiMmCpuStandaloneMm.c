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
#include <Library/ArmSvcLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>

#include <Protocol/MmConfiguration.h>
#include <Protocol/MmGuidedEventManagement.h>
#include <Protocol/MmHandlerNotification.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/DebugSupport.h> // for EFI_SYSTEM_CONTEXT

#include <Guid/ZeroGuid.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/MpInformation.h>
#include <Guid/SmmUefiInfo.h>

#include <IndustryStandard/ArmStdSmc.h>

EFI_STATUS
EFIAPI
ArmMmiHandlerRegisterNotifier (
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN CONST EFI_GUID                 *HandlerType   OPTIONAL
  );

EFI_STATUS
EFIAPI
ArmMmiHandlerUnregisterNotifier (
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN CONST EFI_GUID                 *HandlerType   OPTIONAL
  );

EFI_STATUS
EFIAPI
ArmMmiHandlerRegister(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  EFI_SMM_HANDLER_ENTRY_POINT2                   Handler,
  IN  CONST EFI_GUID                                *HandlerType  OPTIONAL,
  OUT EFI_HANDLE                                    *DispatchHandle
  );

EFI_STATUS
EFIAPI
ArmMmiHandlerUnRegister(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  CONST EFI_GUID                                *HandlerType  OPTIONAL,
  IN  EFI_HANDLE                                    *DispatchHandle
  );

EFI_STATUS
EFIAPI
ArmMmiGetContext(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  UINTN                                         CpuNumber,
  OUT  VOID                                         **CommBuffer,
  OUT  UINTN                                        *CommBufferSize
  );
EFI_STATUS
EFIAPI
ArmRegisterMmFoundationEntry (
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                  MmEntryPoint
  );

EFI_STATUS
EFIAPI
ArmMmCpuReadSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  OUT VOID                        *Buffer
  );

EFI_STATUS
EFIAPI
ArmMmCpuWriteSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  IN CONST VOID                   *Buffer
  );

extern EFI_STATUS _PiMmCpuStandaloneMmEntryPoint (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  );

// TODO: Non standard flag defined by ARM TF to mark the SMRAM descriptor that
// contains the extents of the buffer to be used for communication with the
// normal world
#define EFI_SECURE_NS_MEM             0x00000080

//
// Many GUIDed events within the MM environment can correspond to a single Event
// ID within the ARM Trusted Firmware environment. EVENT_ID_INFO keeps track of
// the number of GUIDed event handlers registered for a particular ARM TF Event.
//
// TODO:
//      1. This infomation could be maintained in the platform component of MM.
//      2. Also, a flags field could be added to provide more information about
//         the event e.g. the parameters are populated in GP registers (assumed
//         currently), system registers or system memory.
//      3. Events could local to a CPU or global. More thought is needed around
//         this. Currently in the event ID, bits[3:0] contain the ID and bit[4]
//         when set indicates a global event.
//
#define EVENT_ID_MM_COMMUNICATE_SMC	0x10

typedef struct {
  UINT16   HandlerCount;
} EVENT_ID_INFO;

EVENT_ID_INFO EventIdInfo[] = {
  [EVENT_ID_MM_COMMUNICATE_SMC] = {0}
};

//
// GUID_TO_EVENT_ID_ENTRY is used to construct a lookup table. This is used at
// the time of registration of GUIDed handlers to lookup the corresponding
// event.
//
typedef struct {
  EFI_GUID Guid;
  UINTN    EventId;
} GUID_TO_EVENT_ID_ENTRY;


//
// Table that contains and entry for every GUID and its corresponding event id
// supported by this platform. There can be a many to one relationship between
// GUIDs and event ids.
//
const GUID_TO_EVENT_ID_ENTRY GuidToEventIdLookupTable[] = {
  {
    .Guid = SMM_UEFI_INFO_GUID,
    .EventId = EVENT_ID_MM_COMMUNICATE_SMC
  }
};

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
EFI_GUID *EventIdToGuidLookupTable = NULL;

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

EFI_SMM_CPU_PROTOCOL mMmCpuState = {
  ArmMmCpuReadSaveState,
  ArmMmCpuWriteSaveState
};

EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL mMmGuidedEventMgmt = {
  ArmMmiHandlerRegister,
  ArmMmiHandlerUnRegister,
  ArmMmiGetContext
};

EFI_SMM_ENTRY_POINT     mMmEntryPoint = NULL;

EFI_STATUS
PiMmCpuStandaloneMmEntryPoint (
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
  UINTN                            ArraySize;
  VOID                            *HobStart;

  ASSERT (SystemTable != NULL);
  mSmst = SystemTable;

  // publish the SMM config protocol so the SMM core can register its entry point
  Status = mSmst->SmmInstallProtocolInterface(&mMmCpuHandle,
    &gEfiMmConfigurationProtocolGuid, EFI_NATIVE_INTERFACE, &mMmConfig);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // publish the SMM CPU save state protocol
  Status = mSmst->SmmInstallProtocolInterface(&mMmCpuHandle,
    &gEfiSmmCpuProtocolGuid, EFI_NATIVE_INTERFACE, &mMmCpuState);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // publish the MM Guided event management protocol
  Status = mSmst->SmmInstallProtocolInterface(&mMmCpuHandle,
    &gEfiMmGuidedEventManagementProtocolGuid, EFI_NATIVE_INTERFACE, &mMmGuidedEventMgmt);
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

  //
  // Allocate memory for a table to hold pointers to a
  // EFI_SMM_COMMUNICATE_HEADER for each CPU
  //
  ArraySize = sizeof(EFI_SMM_COMMUNICATE_HEADER *) *
	  mMpInformationHobData->NumberOfEnabledProcessors;
  Status = mSmst->SmmAllocatePool(EfiRuntimeServicesData,
				  ArraySize,
				  (VOID **) &PerCpuGuidedEventContext);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_INFO, "PerCpuGuidedEventContext mem alloc failed - 0x%x\n", Status));
    return Status;
  }
  return Status;
}

EFI_STATUS
FindEventIdFromGuid(
  IN  CONST EFI_GUID                                *Guid,
  OUT UINTN                                         *EventId
  ) {
  UINTN Ctr;

  if (NULL == Guid || NULL == EventId)
    return EFI_INVALID_PARAMETER;

  if (NULL == GuidToEventIdLookupTable)
    return EFI_NOT_FOUND;

  for (Ctr = 0; Ctr < ARRAY_SIZE(GuidToEventIdLookupTable); Ctr++)
     if (CompareGuid(Guid, &GuidToEventIdLookupTable[Ctr].Guid)) {
       *EventId = GuidToEventIdLookupTable[Ctr].EventId;
       return EFI_SUCCESS;
     }
  return EFI_NOT_FOUND;
}

EFI_STATUS
ValidateAndFindEventIdFromGuid(
  IN  CONST EFI_GUID                                *Guid,
  OUT UINTN                                         *EventId
  ) {
  // Check if a GUID has been provided by the caller
  if (NULL == Guid)
    return EFI_INVALID_PARAMETER;

  // Check if a valid GUID has been provided by the caller
  if (CompareGuid(Guid, &gZeroGuid))
    return EFI_INVALID_PARAMETER;

  // Find the event id corresponding to this GUID
  return FindEventIdFromGuid(Guid, EventId);
}

EFI_STATUS
EFIAPI
ArmMmiHandlerRegisterNotifier (
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN CONST EFI_GUID                 *HandlerType   OPTIONAL
  ) {
  EFI_STATUS   Status;
  UINTN        EventId;
  ARM_SVC_ARGS RegisterEventSvcArgs = {0};

  // Find the event id corresponding to this GUID
  Status = ValidateAndFindEventIdFromGuid(HandlerType, &EventId);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_INFO, "ArmMmiHandlerRegisterNotifier - Unknown GUID %g, Handler - 0x%x\n, Status - %d", HandlerType, Handler, Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "ArmMmiHandlerRegisterNotifier - GUID %g, Handler - 0x%x, EventId %d\n", HandlerType, Handler, EventId));

  // Check if the event has already been registered with EL3 else do so
  if (EventIdInfo[EventId].HandlerCount == 0) {

    // Prepare arguments to register and enable this event at EL3 and
    // check if the GUID corresponds to an event that can be registered
    RegisterEventSvcArgs.Arg0 = ARM_SMC_ID_MM_EVENT_REGISTER_AARCH64;
    RegisterEventSvcArgs.Arg1 = EventId;
    RegisterEventSvcArgs.Arg2 = (UINTN) _PiMmCpuStandaloneMmEntryPoint;

    ArmCallSvc(&RegisterEventSvcArgs);
    Status = RegisterEventSvcArgs.Arg0;
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_INFO, "ArmMmiHandlerRegisterNotifier- Unable to register EventId %d, Status %d\n", EventId, Status));
      return Status;
    }
  }

  EventIdInfo[EventId].HandlerCount++;
  return Status;
}

EFI_STATUS
EFIAPI
ArmMmiHandlerUnregisterNotifier (
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN CONST EFI_GUID                 *HandlerType   OPTIONAL
  ) {
  EFI_STATUS Status;
  UINTN EventId;
  ARM_SVC_ARGS UnRegisterEventSvcArgs = {0};

  // Find the event id corresponding to this GUID
  Status = ValidateAndFindEventIdFromGuid(HandlerType, &EventId);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_INFO, "ArmMmiHandlerUnregisterNotifier - Unknown GUID %g, Handler - 0x%x\n, Status - %d", HandlerType, Handler, Status));
    return Status;
  }

  // Check if there is a handler registered for this Guided event. Decrement the
  // handler counter if so. Else, this is an attempt to unregister a handler
  // that was never registered.
  if (!EventIdInfo[EventId].HandlerCount)
    return EFI_INVALID_PARAMETER;

  EventIdInfo[EventId].HandlerCount--;

  // If the event has a 0 handler count then it has to be unregistered at EL3
  if (EventIdInfo[EventId].HandlerCount == 0) {
    UnRegisterEventSvcArgs.Arg0 = ARM_SMC_ID_MM_EVENT_UNREGISTER_AARCH64;
    UnRegisterEventSvcArgs.Arg1 = EventId;
    ArmCallSvc(&UnRegisterEventSvcArgs);
    Status = UnRegisterEventSvcArgs.Arg0;
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_INFO, "ArmMmiHandlerUnregisterNotifier- Unable to unregister EventId %d, Status %d\n", EventId, Status));
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
ArmMmiHandlerRegister(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  EFI_SMM_HANDLER_ENTRY_POINT2                   Handler,
  IN  CONST EFI_GUID                                *HandlerType,
  OUT EFI_HANDLE                                    *DispatchHandle
  ) {
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN EventId;
  ARM_SVC_ARGS RegisterEventSvcArgs = {0};

  // Find the event id corresponding to this GUID
  Status = ValidateAndFindEventIdFromGuid(HandlerType, &EventId);
  if (Status != EFI_SUCCESS)
    return Status;

  DEBUG ((EFI_D_INFO, "ArmMmiHandlerRegister - GUID %g, Handler - 0x%x, Event ID - %d\n",
	  HandlerType,
	  Handler,
	  EventId));

  //
  // Register the handler with the main MM dispatcher. Bail out in case of an
  // error
  //
  Status = mSmst->SmiHandlerRegister(Handler, HandlerType, DispatchHandle);
  if (Status != EFI_SUCCESS)
    return Status;

  // Check if the event has already been registered with EL3 else do so
  if (!EventIdInfo[EventId].HandlerCount) {

    // Prepare arguments to register and enable this event at EL3 and
    // check if the GUID corresponds to an event that can be registered
    RegisterEventSvcArgs.Arg0 = ARM_SMC_ID_MM_EVENT_REGISTER_AARCH64;
    RegisterEventSvcArgs.Arg1 = EventId;
    RegisterEventSvcArgs.Arg2 = (UINTN) _PiMmCpuStandaloneMmEntryPoint;

    ArmCallSvc(&RegisterEventSvcArgs);
    if (RegisterEventSvcArgs.Arg0 != EFI_SUCCESS) {
      Status = mSmst->SmiHandlerUnRegister(DispatchHandle);
      ASSERT (Status == EFI_SUCCESS);
      return EFI_INVALID_PARAMETER;
    }
  }

  EventIdInfo[EventId].HandlerCount++;
  return Status;
}

EFI_STATUS
EFIAPI
ArmMmiHandlerUnRegister(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  CONST EFI_GUID                                *HandlerType,
  IN  EFI_HANDLE                                    *DispatchHandle
  ) {
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN EventId;
  ARM_SVC_ARGS UnRegisterEventSvcArgs = {0};

  // Find the event id corresponding to this GUID
  Status = ValidateAndFindEventIdFromGuid(HandlerType, &EventId);
  if (Status != EFI_SUCCESS)
    return Status;

  // Check if there is a handler registered for this Guided event. Decrement the
  // handler counter if so. Else, this is an attempt to unregister a handler
  // that was never registered.
  if (!EventIdInfo[EventId].HandlerCount)
    return EFI_INVALID_PARAMETER;

  EventIdInfo[EventId].HandlerCount--;

  // If the event has a 0 handler count then it has to be unregistered at EL3
  if (!EventIdInfo[EventId].HandlerCount) {
    UnRegisterEventSvcArgs.Arg0 = ARM_SMC_ID_MM_EVENT_UNREGISTER_AARCH64;
    UnRegisterEventSvcArgs.Arg1 = EventId;
    ArmCallSvc(&UnRegisterEventSvcArgs);
    ASSERT (UnRegisterEventSvcArgs.Arg0 == EFI_SUCCESS);
  }

  //
  // UnRegister the handler with the main MM dispatcher. Bail out in case of an
  // error
  //
  return mSmst->SmiHandlerUnRegister(DispatchHandle);
}

EFI_STATUS
EFIAPI
ArmMmiGetContext(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  UINTN                                         CpuNumber,
  OUT  VOID                                         **CommBuffer,
  OUT  UINTN                                        *CommBufferSize
  ) {
  if (!CommBufferSize || !CommBuffer)
    return EFI_INVALID_PARAMETER;

  if (!PerCpuGuidedEventContext[CpuNumber])
    return EFI_NOT_FOUND;

  *CommBuffer = PerCpuGuidedEventContext[CpuNumber];
  *CommBufferSize = PerCpuGuidedEventContext[CpuNumber]->MessageLength;

  return EFI_SUCCESS;
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

EFI_STATUS
EFIAPI
ArmMmCpuReadSaveState(
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  OUT VOID                        *Buffer
  ) {
  // todo: implement
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
ArmMmCpuWriteSaveState(
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  IN CONST VOID                   *Buffer
  ) {
  // todo: implement
  return EFI_UNSUPPORTED;
}
