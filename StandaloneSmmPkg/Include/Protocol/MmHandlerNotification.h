/** @file
  EFI MM Handler notification Protocol

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

#ifndef _MM_HANDLER_NOTIFICATION_H_
#define _MM_HANDLER_NOTIFICATION_H_

#include <Pi/PiSmmCis.h>

#define EFI_MM_HANDLER_NOTIFICATION_PROTOCOL_GUID \
  { \
    0x30c8340f, 0x4c30, 0x41d9, {0xbf, 0xae, 0x44, 0x4a, 0xcb, 0x2c, 0x1f, 0x76} \
  }

typedef struct _EFI_MM_HANDLER_NOTIFICATION_PROTOCOL EFI_MM_HANDLER_NOTIFICATION_PROTOCOL;

///
/// The EFI MM Handler notification Protocol is a mandatory protocol published
/// by the MM core to register notifiers that will be invoked upon
/// registration/unregistration of MM Handlers
///
struct _EFI_MM_HANDLER_NOTIFICATION_PROTOCOL {
  EFI_SMM_HANDLER_REGISTER_NOTIFIER_REGISTER    SmiHandlerRegisterNotifierRegister;
  EFI_SMM_HANDLER_UNREGISTER_NOTIFIER_REGISTER  SmiHandlerUnregisterNotifierRegister;
};

extern EFI_GUID gEfiMmHandlerNotificationProtocolGuid;

#endif
