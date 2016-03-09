/** @file
  Standalone SMM.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _STANDALONE_SMM_H_
#define _STANDALONE_SMM_H_

#include <PiSmm.h>

typedef
EFI_STATUS
(EFIAPI *SMM_IMAGE_ENTRY_POINT) (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SMM_SYSTEM_TABLE2 *SmmSystemTable
  );

typedef
EFI_STATUS
(EFIAPI *STANDALONE_SMM_FOUNDATION_ENTRY_POINT) (
  IN VOID  *HobStart
  );

#endif
