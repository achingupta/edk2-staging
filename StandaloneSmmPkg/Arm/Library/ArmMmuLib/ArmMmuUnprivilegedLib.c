/** @file
*  File managing the MMU for ARMv8 architecture in S-EL0
*
*  Copyright (c) 2017, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Uefi.h>
#include <Chipset/AArch64.h>
#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <Library/ArmSvcLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

RETURN_STATUS
RequestMemoryPermissionChange(
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length,
  IN  UINTN                     Permissions
  )
{
  ARM_SVC_ARGS		ChangeMemoryPermissionsSvcArgs = {0};

  ChangeMemoryPermissionsSvcArgs.Arg0 = ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES_AARCH64;
  ChangeMemoryPermissionsSvcArgs.Arg1 = BaseAddress;
  ChangeMemoryPermissionsSvcArgs.Arg2 = (Length >= EFI_PAGE_SIZE) ?	\
	  				 Length >> EFI_PAGE_SHIFT : 1;
  ChangeMemoryPermissionsSvcArgs.Arg3 = Permissions;

  ArmCallSvc(&ChangeMemoryPermissionsSvcArgs);

  return ChangeMemoryPermissionsSvcArgs.Arg0;
}

RETURN_STATUS
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return RequestMemoryPermissionChange(BaseAddress,
				       Length,
				       SET_MEM_ATTR_MAKE_PERM_REQUEST( \
					       SET_MEM_ATTR_DATA_PERM_RO, \
					       SET_MEM_ATTR_CODE_PERM_XN));
}

RETURN_STATUS
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return RequestMemoryPermissionChange(BaseAddress,
				       Length,
				       SET_MEM_ATTR_MAKE_PERM_REQUEST( \
					       SET_MEM_ATTR_DATA_PERM_RO, \
					       SET_MEM_ATTR_CODE_PERM_X));
}

RETURN_STATUS
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return RequestMemoryPermissionChange(BaseAddress,
				       Length,
				       SET_MEM_ATTR_MAKE_PERM_REQUEST( \
					       SET_MEM_ATTR_DATA_PERM_RO, \
					       SET_MEM_ATTR_CODE_PERM_XN));
}

RETURN_STATUS
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  )
{
  return RequestMemoryPermissionChange(BaseAddress,
				       Length,
				       SET_MEM_ATTR_MAKE_PERM_REQUEST( \
					       SET_MEM_ATTR_DATA_PERM_RW, \
					       SET_MEM_ATTR_CODE_PERM_XN));
}

RETURN_STATUS
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                         **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize OPTIONAL
  )
{
  return RETURN_UNSUPPORTED;
}

RETURN_STATUS
EFIAPI
ArmMmuBaseLibConstructor (
  VOID
  )
{
  return RETURN_SUCCESS;
}
