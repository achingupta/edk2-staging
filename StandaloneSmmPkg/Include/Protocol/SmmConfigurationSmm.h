/** @file
  EFI SMM Configuration SMM Protocol.

  This protocol is used register the SMM Foundation entry point with the processor code.
  The entry point will be invoked by the SMM processor entry code.
  
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _SMM_CONFIGURATION_SMM_H_
#define _SMM_CONFIGURATION_SMM_H_

#include <Protocol/SmmConfiguration.h>

#define EFI_SMM_CONFIGURATION_SMM_PROTOCOL_GUID \
  { \
    0x4f81defe, 0x3221, 0x4682, { 0xbf, 0xfc, 0xe7, 0xd2, 0x1b, 0x4a, 0xa3, 0x17 }  \
  }

///
/// The SMM Configuration Protocol is a mandatory protocol published by a Standalone SMM CPU driver.
///
/// The RegisterSmmEntry() function allows the Standalone SMM Foundation driver to register the SMM 
/// Foundation entry point with the SMM entry vector code.
///
/// EFI_SMM_CONFIGURATION_SMM_PROTOCOL has same interface as SMM_CONFIGURATION_PROTOCOL.
/// The only difference is that EFI_SMM_CONFIGURATION_SMM_PROTOCOL is installed in SMM protocol database,
/// while SMM_CONFIGURATION_PROTOCOL is installed in DXE protocol database.
///

extern EFI_GUID gEfiSmmConfigurationSmmProtocolGuid;

#endif

