/** @file
  EFI SMM Communication Protocol as defined in the PI 1.2 specification.

  This protocol provides a means of communicating between drivers outside of SMM and SMI
  handlers inside of SMM.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_COMMUNICATION_H_
#define _SMM_COMMUNICATION_H_

//
// Include the EFI_MM_COMMUNICATION_PROTOCOL header for actual definitions. This
// header is used to maintain backwards compatibility.
//
#include <Protocol/MmCommunication.h>

#define EFI_SMM_COMMUNICATION_PROTOCOL_GUID EFI_MM_COMMUNICATION_PROTOCOL_GUID

typedef EFI_MM_COMMUNICATION_PROTOCOL       EFI_SMM_COMMUNICATION_PROTOCOL;
typedef EFI_MM_COMMUNICATE                  EFI_SMM_COMMUNICATE2;

extern EFI_GUID gEfiSmmCommunicationProtocolGuid;
#endif
