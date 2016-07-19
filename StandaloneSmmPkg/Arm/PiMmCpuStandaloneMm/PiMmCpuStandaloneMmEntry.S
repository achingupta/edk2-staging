//
//  Copyright (c) 2016, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

#include <Base.h>
#include <AutoGen.h>
#include <IndustryStandard/ArmStdSmc.h>

.text
.align 3

GCC_ASM_IMPORT(PiMmCpuStandaloneMmEntryPoint)
GCC_ASM_EXPORT(_PiMmCpuStandaloneMmEntryPoint)

// Stub entry point to ensure that the stacks are completely unwound before
// signalling completion of event handling
ASM_PFX(_PiMmCpuStandaloneMmEntryPoint):
  bl    PiMmCpuStandaloneMmEntryPoint
  mov   x1, x0
  mov	x0, #(ARM_SMC_ID_MM_EVENT_COMPLETE_AARCH64 & 0xffff)
  movk	x0, #((ARM_SMC_ID_MM_EVENT_COMPLETE_AARCH64 >> 16) & 0xffff), lsl #16
  svc	#0
LoopForever:
  b	LoopForever
