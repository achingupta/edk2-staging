/** @file
  EFI MM Guided Event Management Protocol

  This protocol is used to:
  1) register Guided handlers for MMIs with the processor code.

  Copyright (c) 2016, ARM Limited. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MM_GUIDED_EVENT_MANAGEMENT_H_
#define _MM_GUIDED_EVENT_MANAGEMENT_H_

#include <Pi/PiSmmCis.h>

#define EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL_GUID \
  { \
    0x30c8340f, 0x4c30, 0x41d9, {0xbf, 0xae, 0x44, 0x4a, 0xcb, 0x2c, 0x1f, 0x76} \
  }

typedef struct _EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL;

/**
  Registers a Guided MMI handler to execute within MM.

  @param  This           The EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL instance.
  @param  Handler        Handler service funtion pointer.
  @param  HandlerType    Points to the handler type or NULL for root SMI handlers.
  @param  DispatchHandle On return, contains a unique handle which can be used to later unregister the handler function.

  @retval EFI_SUCCESS           Handler register success.
  @retval EFI_INVALID_PARAMETER Handler, HandlerType or DispatchHandle is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MM_GUIDED_EVENT_HANDLER_REGISTER)(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  EFI_SMM_HANDLER_ENTRY_POINT2                   Handler,
  IN  CONST EFI_GUID                                *HandlerType  OPTIONAL,
  OUT EFI_HANDLE                                    *DispatchHandle
  );

/**
  Registers a Guided MMI handler to execute within MM.

  @param  This            The EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL instance.
  @param  DispatchHandle  The handle that was specified when the handler was registered.

  @retval EFI_SUCCESS           Handler function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER DispatchHandle does not refer to a valid handle.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MM_GUIDED_EVENT_HANDLER_UNREGISTER)(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  CONST EFI_GUID                                *HandlerType,
  IN  EFI_HANDLE                                    *DispatchHandle
  );

/**
  Returns context information associated with the last guided event received by the MM entry point on the executing cpu

  @param  This            The EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL instance.
  @param  CommBuffer      Pointer to communication buffer containing event context information
  @param  CommBufferSize  Size of communication buffer containing event context information

  @retval EFI_SUCCESS           Communication buffer and size were successfully returned
  @retval EFI_INVALID_PARAMETER *This does not refer to a valid handle.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_MM_GUIDED_EVENT_GET_CONTEXT)(
  IN  CONST EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL *This,
  IN  UINTN                                         CpuNumber,
  OUT  VOID                                         **CommBuffer,
  OUT  UINTN                                         *CommBufferSize
  );

///
/// The EFI MM Guided Event Management Protocol is a mandatory protocol published by a MM
/// CPU driver to register handlers for events that can be identified using a GUID.
///
struct _EFI_MM_GUIDED_EVENT_MANAGEMENT_PROTOCOL {
  EFI_MM_GUIDED_EVENT_HANDLER_REGISTER          GuidedEventHandlerRegister;
  EFI_MM_GUIDED_EVENT_HANDLER_UNREGISTER        GuidedEventHandlerUnregister;
  EFI_MM_GUIDED_EVENT_GET_CONTEXT               GuidedEventGetContext;
};

extern EFI_GUID gEfiMmGuidedEventManagementProtocolGuid;

#endif
