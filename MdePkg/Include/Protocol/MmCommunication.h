/** @file
  EFI MM Communication Protocol as defined in the PI 1.5 specification.

  This protocol provides a means of communicating between drivers outside of MM and MMI
  handlers inside of MM.

  Copyright (c) 2016, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MM_COMMUNICATION_H_
#define _MM_COMMUNICATION_H_

//
// Need include this header file for EFI_MM_COMMUNICATE_HEADER data structure.
//
#include <Uefi/UefiAcpiDataTable.h>

#define EFI_MM_COMMUNICATION_PROTOCOL_GUID \
  { \
    0xc68ed8e2, 0x9dc6, 0x4cbd, { 0x9d, 0x94, 0xdb, 0x65, 0xac, 0xc5, 0xc3, 0x32 } \
  }

typedef struct _EFI_MM_COMMUNICATION_PROTOCOL  EFI_MM_COMMUNICATION_PROTOCOL;

/**
  Communicates with a registered handler.

  This function provides an interface to send and receive messages to the
  MM environment on behalf of UEFI services.  This function is part
  of the MM Communication Protocol that may be called in physical mode prior to
  SetVirtualAddressMap() and in virtual mode after SetVirtualAddressMap().

  @param[in]      This                The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBuffer          A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize            The size of the data buffer being passed in.On exit, the size of data
                                      being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation. If this error is
                                      returned, the MessageLength field in the CommBuffer header or the integer
                                      pointed by CommSize are updated to reflect the maximum payload size the
                                      implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter or CommSize parameter, if not omitted,
                                      are in address range that cannot be accessed by the MM environment
**/
typedef
EFI_STATUS
(EFIAPI *EFI_MM_COMMUNICATE)(
  IN CONST EFI_MM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                              *CommBuffer,
  IN OUT UINTN                             *CommSize    OPTIONAL
  );

///
/// EFI MM Communication Protocol provides runtime services for communicating
/// between DXE drivers and a registered SMI handler.
///
struct _EFI_MM_COMMUNICATION_PROTOCOL {
  EFI_MM_COMMUNICATE  Communicate;
};

extern EFI_GUID gEfiMmCommunicationProtocolGuid;

#endif
