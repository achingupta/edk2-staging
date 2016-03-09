/** @file
  EFI SMM MemoryMap Protocol.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _SMM_MEMORY_MAP_H_
#define _SMM_MEMORY_MAP_H_

#define EFI_SMM_MEMORY_MAP_PROTOCOL_GUID \
  { \
    0x46c5132c, 0x8e88, 0x4865, { 0x8f, 0x2e, 0x14, 0xd2, 0xc5, 0x6, 0xf2, 0x5c } \
  }
  
typedef struct _EFI_SMM_MEMORY_MAP_PROTOCOL  EFI_SMM_MEMORY_MAP_PROTOCOL;

///
/// EFI SMM MemoryMap Protocol provides server to get SMM memory map.
///
/// This protocol allows SMM drivers to get memory map of SMRAM.
/// All SMRAM MUST be returned. Non SMRAM MUST not be returned.
///
struct _EFI_SMM_MEMORY_MAP_PROTOCOL {
  EFI_GET_MEMORY_MAP GetMemoryMap;
};

extern EFI_GUID gEfiSmmMemoryMapProtocolGuid;

#endif

