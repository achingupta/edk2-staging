/** @file
  SMM IPL that load the SMM Core into SMRAM

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available 
  under the terms and conditions of the BSD License which accompanies this 
  distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PiPei.h>
#include <Pi/PiDxeCis.h>
#include <PiSmm.h>
#include <StandaloneSmm.h>

EFI_STATUS
LoadSmmCore (
  IN EFI_PHYSICAL_ADDRESS  Entry,
  IN VOID                  *Context1
  )
{
  STANDALONE_SMM_FOUNDATION_ENTRY_POINT         EntryPoint;

  EntryPoint = (STANDALONE_SMM_FOUNDATION_ENTRY_POINT)(UINTN)Entry;
  return EntryPoint (Context1);
}
