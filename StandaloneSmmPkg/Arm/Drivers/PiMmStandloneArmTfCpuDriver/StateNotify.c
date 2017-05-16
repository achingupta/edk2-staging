/** @file

  Copyright (c) 2017, ARM Limited. All rights reserved.

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
#include <Protocol/DebugSupport.h> // for EFI_SYSTEM_CONTEXT
#include <Guid/ZeroGuid.h>
#include <Guid/SmmUefiInfo.h>
#include <IndustryStandard/ArmStdSmc.h>
#include "PiMmStandloneArmTfCpuDriver.h"

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

// Pointer to handler notification protocol interface
EFI_MM_HANDLER_STATE_NOTIFICATION_PROTOCOL *mMmHandlerStateNotification;

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
MmiHandlerRegisterNotifier (
  IN UINTN EventId
  ) {
  EFI_STATUS Status;
  ARM_SVC_ARGS RegisterEventSvcArgs = {0};

  // Check if the event has already been registered with EL3 else do so
  if (EventIdInfo[EventId].HandlerCount == 0) {

    // Prepare arguments to register and enable this event at EL3 and
    // check if the GUID corresponds to an event that can be registered
    RegisterEventSvcArgs.Arg0 = ARM_SMC_ID_MM_EVENT_REGISTER_AARCH64;
    RegisterEventSvcArgs.Arg1 = EventId;
    RegisterEventSvcArgs.Arg2 = (UINTN) _PiMmStandloneArmTfCpuDriverEntry;

    ArmCallSvc(&RegisterEventSvcArgs);
    Status = RegisterEventSvcArgs.Arg0;
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_INFO, "ArmMmiHandlerRegisterNotifier- Unable to register EventId %d, Status %d\n", EventId, Status));
      return EFI_INVALID_PARAMETER;
    }
  }

  EventIdInfo[EventId].HandlerCount++;
  return Status;
}

EFI_STATUS
EFIAPI
MmiHandlerUnregisterNotifier (
  IN UINTN EventId
  ) {
  EFI_STATUS Status;
  ARM_SVC_ARGS UnRegisterEventSvcArgs = {0};

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
      DEBUG ((EFI_D_INFO, "MmiHandlerUnregisterNotifier- Unable to unregister EventId %d, Status %d\n", EventId, Status));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MmiHandlerStateNotifier (
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN CONST EFI_GUID                 *HandlerType   OPTIONAL,
  IN EFI_MM_HANDLER_STATE           HandlerState
  ) {
  EFI_STATUS   Status;
  UINTN        EventId;

  // Find the event id corresponding to this GUID
  Status = ValidateAndFindEventIdFromGuid(HandlerType, &EventId);
  if (Status != EFI_SUCCESS) {
    DEBUG ((EFI_D_INFO, "MmiHandlerStateNotifier - Unknown GUID %g, Handler - 0x%x\n, State - %d, Status - %d", HandlerType, Handler, HandlerState, Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "MmiHandlerStateNotifier - GUID %g, Handler - 0x%x\n, State - %d", HandlerType, Handler, HandlerState));

  if (HandlerState == HandlerRegistered) {
    return MmiHandlerRegisterNotifier(EventId);
  }

  if (HandlerState == HandlerUnregistered) {
    return MmiHandlerUnregisterNotifier(EventId);
  }

  return EFI_INVALID_PARAMETER;
}
