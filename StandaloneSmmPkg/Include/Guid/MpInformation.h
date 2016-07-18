/** @file
  EFI MP information protocol provides a lightweight MP_SERVICES_PROTOCOL.

  MP information protocol only provides static information of MP processor.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _MP_INFORMATION_H_
#define _MP_INFORMATION_H_

#include <Protocol/MpService.h>
#include <PiPei.h>
#include <Ppi/SecPlatformInformation.h>

#define MP_INFORMATION_GUID \
  { \
    0xba33f15d, 0x4000, 0x45c1, {0x8e, 0x88, 0xf9, 0x16, 0x92, 0xd4, 0x57, 0xe3}  \
  }

#pragma pack(1)
typedef struct {
  UINT64                     NumberOfProcessors;
  UINT64                     NumberOfEnabledProcessors;
  EFI_PROCESSOR_INFORMATION  ProcessorInfoBuffer[];
} MP_INFORMATION_HOB_DATA;
#pragma pack()

extern EFI_GUID gMpInformationHobGuid;

#endif
