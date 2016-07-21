/** @file
  EFI MM Configuration Protocol as defined in the PI 1.5 specification.

  This protocol is used to:
  1) register the MM Foundation entry point with the processor code. The entry
     point will be invoked by the MM processor entry code.

  Copyright (c) 2016, ARM Limited. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MM_CONFIGURATION_H_
#define _MM_CONFIGURATION_H_

#include <Pi/PiSmmCis.h>

#define EFI_MM_CONFIGURATION_PROTOCOL_GUID \
  { \
    0xc109319, 0xc149, 0x450e, {0xa3, 0xe3, 0xb9, 0xba, 0xdd, 0x9d, 0xc3, 0xa4 } \
  }

typedef struct _EFI_MM_CONFIGURATION_PROTOCOL EFI_MM_CONFIGURATION_PROTOCOL;

/**
  Register the MM Foundation entry point.

  This function registers the MM Foundation entry point with the
  processor code. This entry point will be invoked by the MM
  Processor entry code.

  @param[in] This                The EFI_MM_CONFIGURATION_PROTOCOL instance.
  @param[in] MmEntryPoint        MM Foundation entry point.

  @retval EFI_SUCCESS            Success to register SMM Entry Point.
  @retval EFI_INVALID_PARAMETER  SmmEntryPoint is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_MM_REGISTER_MM_FOUNDATION_ENTRY)(
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  /// TODO: Change to EFI_MM_ENTRY_POINT once the rest of the code changes
  IN EFI_SMM_ENTRY_POINT                   MmEntryPoint
  );

///
/// The EFI MM Configuration Protocol is a mandatory protocol published by a MM
/// CPU driver to register the MM Foundation entry point with the MM entry
/// vector code.
///
struct _EFI_MM_CONFIGURATION_PROTOCOL {
	EFI_MM_REGISTER_MM_FOUNDATION_ENTRY     RegisterMmFoundationEntry;
};

extern EFI_GUID gEfiMmConfigurationProtocolGuid;

#endif
