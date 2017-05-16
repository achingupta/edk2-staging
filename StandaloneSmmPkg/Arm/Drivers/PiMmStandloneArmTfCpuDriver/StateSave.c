/** @file

  Copyright (c) 2016 HP Development Company, L.P.
  Copyright (c) 2016-2017, ARM Limited. All rights reserved.

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

#include "PiMmStandloneArmTfCpuDriver.h"

EFI_SMM_CPU_PROTOCOL mMmCpuState = {
  MmReadSaveState,
  MmWriteSaveState
};

EFI_STATUS
EFIAPI
MmReadSaveState(
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  OUT VOID                        *Buffer
  ) {
  // todo: implement
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
MmWriteSaveState(
  IN CONST EFI_SMM_CPU_PROTOCOL   *This,
  IN UINTN                        Width,
  IN EFI_SMM_SAVE_STATE_REGISTER  Register,
  IN UINTN                        CpuIndex,
  IN CONST VOID                   *Buffer
  ) {
  // todo: implement
  return EFI_UNSUPPORTED;
}
