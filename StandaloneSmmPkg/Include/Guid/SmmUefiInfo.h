/** @file
  GUIDs for SMM Event.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __SMM_UEFI_INFO_H__
#define __SMM_UEFI_INFO_H__

#define SMM_UEFI_INFO_GUID \
  { 0xa37721e4, 0x8c0b, 0x4bca, { 0xb5, 0xe8, 0xe9, 0x2, 0xa0, 0x25, 0x51, 0x4e }}

extern EFI_GUID gSmmUefiInfoGuid;

#pragma pack(1)
typedef struct {
  EFI_PHYSICAL_ADDRESS      EfiSystemTable;
} EFI_SMM_COMMUNICATE_UEFI_INFO_DATA;

typedef struct {
  EFI_GUID                              HeaderGuid;
  UINTN                                 MessageLength;
  EFI_SMM_COMMUNICATE_UEFI_INFO_DATA    Data;
} EFI_SMM_COMMUNICATE_UEFI_INFO;
#pragma pack()

#endif