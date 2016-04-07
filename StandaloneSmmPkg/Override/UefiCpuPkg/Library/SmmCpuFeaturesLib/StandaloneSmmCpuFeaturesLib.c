/** @file
The CPU specific programming for PiSmmCpuDxeSmm module.

Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiSmm.h>
#include <Library/SmmCpuFeaturesLib.h>

EFI_STATUS
InternalSmmCpuFeaturesLibConstructor(
  VOID
  );

/**
  The constructor function

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
StandaloneSmmCpuFeaturesLibConstructor (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SMM_SYSTEM_TABLE2  *SmmSystemTable
  )
{
  return InternalSmmCpuFeaturesLibConstructor ();
}

