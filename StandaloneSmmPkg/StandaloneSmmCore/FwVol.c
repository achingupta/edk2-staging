/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "StandaloneSmmCore.h"
#include <Library/FvLib.h>

EFI_STATUS
SmmAddToDriverList (
  IN EFI_HANDLE   FvHandle,
  IN VOID         *Pe32Data,
  IN UINTN        Pe32DataSize,
  IN VOID         *Depex,
  IN UINTN        DepexSize,
  IN EFI_GUID     *DriverName
  );

BOOLEAN
FvHasBeenProcessed (
  IN EFI_HANDLE  FvHandle
  );

VOID
FvIsBeingProcesssed (
  IN EFI_HANDLE  FvHandle
  );

EFI_STATUS
SmmCoreFfsFindSmmDriver (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  )
/*++

Routine Description:
  Given the pointer to the Firmware Volume Header find the
  SMM driver and return it's PE32 image.

Arguments:
  FwVolHeader - Pointer to memory mapped FV
 
Returns:  
  other       - Failure

--*/
{
  EFI_STATUS          Status;
  EFI_STATUS          DepexStatus;
  EFI_FFS_FILE_HEADER *FileHeader;
  EFI_FV_FILETYPE     FileType;
  VOID                *Pe32Data;
  UINTN               Pe32DataSize;
  VOID                *Depex;
  UINTN               DepexSize;
  
  if (FvHasBeenProcessed (FwVolHeader)) {
    return EFI_SUCCESS;
  }

  FvIsBeingProcesssed (FwVolHeader);

  FileType  = EFI_FV_FILETYPE_SMM;
  FileHeader  = NULL;
  do {
    Status = FfsFindNextFile (FileType, FwVolHeader, &FileHeader);
    if (!EFI_ERROR (Status)) {
      Status = FfsFindSectionData (EFI_SECTION_PE32, FileHeader, &Pe32Data, &Pe32DataSize);
      DepexStatus = FfsFindSectionData (EFI_SECTION_SMM_DEPEX, FileHeader, &Depex, &DepexSize);
      if (!EFI_ERROR(DepexStatus)) {
        SmmAddToDriverList (FwVolHeader, Pe32Data, Pe32DataSize, Depex, DepexSize, &FileHeader->Name);
      }
    }
  } while (!EFI_ERROR (Status));

  return Status;
}
