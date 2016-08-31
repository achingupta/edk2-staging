/** @file
  EFI MM Handler state notification Protocol

  This protocol is used to register a notification function that is invoked
  whenever a SMI handler is registered or unregistered by the MM core

  Copyright (c) 2016, ARM Limited. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MM_HANDLER_STATE_NOTIFICATION_H_
#define _MM_HANDLER_STATE_NOTIFICATION_H_

#include <Pi/PiSmmCis.h>

#define EFI_MM_HANDLER_STATE_NOTIFICATION_PROTOCOL_GUID \
  { \
    0x30c8340f, 0x4c30, 0x41d9, {0xbf, 0xae, 0x44, 0x4a, 0xcb, 0x2c, 0x1f, 0x76} \
  }

typedef struct _EFI_MM_HANDLER_STATE_NOTIFICATION_PROTOCOL EFI_MM_HANDLER_STATE_NOTIFICATION_PROTOCOL;

///
/// States of a MMI handler
///
typedef enum {
  HandlerRegistered,
  HandlerUnregistered,
} EFI_MM_HANDLER_STATE;

/**
  Function prototype for SMI handler state notifier.

  @param[in] Handler                   Handler service funtion pointer.
  @param[in] HandlerType               Points to the handler type or NULL for root SMI handlers.
  @param[in] HandlerState              Handler state i.e. registered or unregistered

  @retval EFI_SUCCESS                  Notification function was successfully invoked
  @retval EFI_INVALID_PARAMETER        Handler is NULL or HandlerType is unrecognized
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_HANDLER_STATE_NOTIFY_FN)(
  IN EFI_SMM_HANDLER_ENTRY_POINT2      Handler,
  IN CONST EFI_GUID                    *HandlerType   OPTIONAL,
  IN EFI_MM_HANDLER_STATE              HandlerState
  );

/**
  Registers a callback function that is invoked whenever a SMI handler is
  registered or unregistered.

  @param[in]       Notifier            Points to the notification function.
  @param[in, out]  Registration        Returns the registration record that has been successfully added.

  @retval EFI_SUCCESS                  Notification function registered successfully.
  @retval EFI_INVALID_PARAMETER        Registration is NULL.
  @retval EFI_OUT_OF_RESOURCES         Not enough memory resource to finish the request.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_HANDLER_STATE_NOTIFIER_REGISTER)(
  IN  EFI_SMM_HANDLER_STATE_NOTIFY_FN  Notifier,
  OUT VOID                             **Registration
  );

/**
  Unregisters a callback function that is invoked whenever a SMI handler is
  registered or unregistered.

  @param[in]  Registration             Registration record returned upon successfully registering the callback function

  @retval EFI_SUCCESS                  Notification function unregistered successfully.
  @retval EFI_INVALID_PARAMETER        Registration is NULL.
  @retval EFI_NOT_FOUND                Registration record not found
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_HANDLER_STATE_NOTIFIER_UNREGISTER)(
  IN VOID                             *Registration
  );

///
/// The EFI MM Handler notification Protocol is a mandatory protocol published
/// by the MM core to register a notifier that will be invoked upon
/// registration/unregistration of MM Handlers
///
struct _EFI_MM_HANDLER_STATE_NOTIFICATION_PROTOCOL {
  EFI_SMM_HANDLER_STATE_NOTIFIER_REGISTER    SmiHandlerStateNotifierRegister;
  EFI_SMM_HANDLER_STATE_NOTIFIER_UNREGISTER  SmiHandlerStateNotifierUnregister;
};

extern EFI_GUID gEfiMmHandlerStateNotificationProtocolGuid;

#endif
