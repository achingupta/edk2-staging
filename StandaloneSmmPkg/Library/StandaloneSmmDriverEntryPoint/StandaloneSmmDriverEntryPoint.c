/** @file
  Entry point to a Standalone SMM driver.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/



#include <PiSmm.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

VOID
EFIAPI
ProcessLibraryConstructorList (
  IN EFI_HANDLE               ImageHandle,
  IN IN EFI_SMM_SYSTEM_TABLE2 *SmmSystemTable
  );

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE               ImageHandle,
  IN IN EFI_SMM_SYSTEM_TABLE2 *SmmSystemTable
  );

VOID
EFIAPI
ProcessLibraryDestructorList (
  IN EFI_HANDLE               ImageHandle,
  IN IN EFI_SMM_SYSTEM_TABLE2 *SmmSystemTable
  );

/**
  The entry point of PE/COFF Image for a DXE Driver, DXE Runtime Driver, DXE SMM 
  Driver, or UEFI Driver. 

  This function is the entry point for a DXE Driver, DXE Runtime Driver, DXE SMM Driver,
  or UEFI Driver.  This function must call ProcessLibraryConstructorList() and
  ProcessModuleEntryPointList(). If the return status from ProcessModuleEntryPointList()
  is an error status, then ProcessLibraryDestructorList() must be called. The return 
  value from ProcessModuleEntryPointList() is returned. If _gDriverUnloadImageCount 
  is greater than zero, then an unload handler must be registered for this image 
  and the unload handler must invoke ProcessModuleUnloadList().
  If _gUefiDriverRevision is not zero and SystemTable->Hdr.Revision is less than 
  _gUefiDriverRevison, then return EFI_INCOMPATIBLE_VERSION.


  @param  ImageHandle  The image handle of the DXE Driver, DXE Runtime Driver, 
                       DXE SMM Driver, or UEFI Driver.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The DXE Driver, DXE Runtime Driver, DXE SMM 
                                     Driver, or UEFI Driver exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than 
                                    SystemTable->Hdr.Revision.
  @retval  Other                     Return value from ProcessModuleEntryPointList().

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_HANDLE               ImageHandle,
  IN IN EFI_SMM_SYSTEM_TABLE2 *SmmSystemTable
  )
{
  EFI_STATUS                 Status;

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (ImageHandle, SmmSystemTable);

  //
  // Call the driver entry point
  //
  Status = ProcessModuleEntryPointList (ImageHandle, SmmSystemTable);

  //
  // If all of the drivers returned errors, then invoke all of the library destructors
  //
  if (EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (ImageHandle, SmmSystemTable);
  }

  //
  // Return the cummalative return status code from all of the driver entry points
  //
  return Status;
}

