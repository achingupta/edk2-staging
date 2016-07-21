/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011, ARM Limited. All rights reserved.
  Copyright (c) 2016 HP Development Company, L.P.
  Copyright (c) 2016, ARM Limited. All rights reserved.

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
#include <Library/BaseMemoryLib.h>

#include <Protocol/MmConfiguration.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/DebugSupport.h> // for EFI_SYSTEM_CONTEXT

EFI_STATUS
EFIAPI
ArmRegisterMmFoundationEntry (
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                  MmEntryPoint
  );

//
// Private copy of the MM system table for future use
//
EFI_SMM_SYSTEM_TABLE2 *mSmst = NULL;

//
// Globals used to initialize the protocol
//
EFI_HANDLE            mMmCpuHandle = NULL;

EFI_MM_CONFIGURATION_PROTOCOL mMmConfig = {
  ArmRegisterMmFoundationEntry
};

EFI_SMM_ENTRY_POINT     mMmEntryPoint = NULL;

EFI_STATUS
PiMmCpuStandaloneMmInitialize (
  IN EFI_HANDLE         ImageHandle,  // not actual imagehandle
  IN EFI_SMM_SYSTEM_TABLE2   *SystemTable  // not actual systemtable
  )
{
  EFI_STATUS                       Status;

  ASSERT (SystemTable != NULL);
  mSmst = SystemTable;

  // publish the SMM config protocol so the SMM core can register its entry point
  Status = mSmst->SmmInstallProtocolInterface(&mMmCpuHandle,
    &gEfiMmConfigurationProtocolGuid, EFI_NATIVE_INTERFACE, &mMmConfig);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return Status;
}

EFI_STATUS
EFIAPI
ArmRegisterMmFoundationEntry(
  IN CONST EFI_MM_CONFIGURATION_PROTOCOL  *This,
  IN EFI_SMM_ENTRY_POINT                  MmEntryPoint
  ) {
  // store the entry point in a global
  mMmEntryPoint = MmEntryPoint;
  return EFI_SUCCESS;
}
