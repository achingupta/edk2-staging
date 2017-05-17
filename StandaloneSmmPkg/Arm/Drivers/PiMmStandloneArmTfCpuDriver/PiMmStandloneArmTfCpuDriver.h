/** @file
  Private header with declarations and definitions specific to the MM Standalone
  CPU driver

  Copyright (c) 2017, ARM Limited. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ARM_TF_CPU_DRIVER_H_
#define _ARM_TF_CPU_DRIVER_H_

#include <Protocol/MmConfiguration.h>
#include <Protocol/MmHandlerStateNotification.h>
#include <Protocol/SmmCpu.h>
#include <Guid/MpInformation.h>

//
// Common declarations and definitions
//
#define EVENT_ID_MM_COMMUNICATE_SMC	0x10

//
// CPU driver initialisation specific declarations
//
extern EFI_SMM_SYSTEM_TABLE2 *mSmst;

//
// CPU State Save protocol specific declarations
//
extern EFI_SMM_CPU_PROTOCOL mMmCpuState;

EFI_STATUS
EFIAPI
MmReadSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  OUT VOID                        *Buffer
  );

EFI_STATUS
EFIAPI
MmWriteSaveState (
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  IN CONST VOID                   *Buffer
  );

//
// MM Handler State Notification protocol specific declarations
//
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
typedef struct {
  UINT16   HandlerCount;
} EVENT_ID_INFO;

extern EVENT_ID_INFO EventIdInfo[];
extern EFI_MM_HANDLER_STATE_NOTIFICATION_PROTOCOL *mMmHandlerStateNotification;

EFI_STATUS
EFIAPI
MmiHandlerRegisterNotifier (
  IN UINTN EventId
  );

EFI_STATUS
EFIAPI
MmiHandlerUnregisterNotifier (
  IN UINTN EventId
  );

EFI_STATUS
EFIAPI
MmiHandlerStateNotifier (
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN CONST EFI_GUID                 *HandlerType   OPTIONAL,
  IN EFI_MM_HANDLER_STATE           HandlerState
  );

//
// MM event handling specific declarations
//
extern EFI_SMM_COMMUNICATE_HEADER    **PerCpuGuidedEventContext;
extern EFI_SMRAM_DESCRIPTOR          mNsCommBuffer;
extern MP_INFORMATION_HOB_DATA       *mMpInformationHobData;
extern EFI_MM_CONFIGURATION_PROTOCOL mMmConfig;

EFI_STATUS
PiMmStandloneArmTfCpuDriverEntry (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  );

EFI_STATUS
EFIAPI
PiMmCpuTpFwRootMmiHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

EFI_STATUS _PiMmStandloneArmTfCpuDriverEntry (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  );

#endif
