/** @file
  EFI SMM Control PPI definition.

  This PPI is used to initiate SMI/PMI activations. This protocol could be published by either:
  - A processor driver to abstract the SMI/PMI IPI
  - The driver that abstracts the ASIC that is supporting the APM port, such as the ICH in an
  Intel chipset
  Because of the possibility of performing SMI or PMI IPI transactions, the ability to generate this
  event from a platform chipset agent is an optional capability for both IA-32 and Itanium-based
  systems.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _SMM_CONTROL2_PPI_H_
#define _SMM_CONTROL2_PPI_H_

#define EFI_PEI_SMM_CONTROL2_PPI_GUID \
  { 0x11c8b147, 0x3f92, 0x4651, { 0xae, 0x6b, 0x76, 0xe5, 0x75, 0x45, 0xe1, 0x9d }}

typedef struct _EFI_PEI_SMM_CONTROL2_PPI  EFI_PEI_SMM_CONTROL2_PPI;
typedef UINTN  EFI_PEI_SMM_PERIOD;

/**
  Invokes SMI activation from either the preboot or runtime environment.

  @param  This                  The EFI_PEI_SMM_CONTROL2_PPI instance.
  @param  CommandPort           The value written to the command port.
  @param  DataPort              The value written to the data port.
  @param  Periodic              An optional mechanism to periodically repeat activation.
  @param  ActivationInterval    An optional parameter to repeat at this period one
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The timing is unsupported.
  @retval EFI_INVALID_PARAMETER The activation period is unsupported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMM_ACTIVATE2) (
  IN EFI_PEI_SMM_CONTROL2_PPI                        *This,
  IN OUT UINT8                                       *CommandPort OPTIONAL,
  IN OUT UINT8                                       *DataPort OPTIONAL,
  IN BOOLEAN                                         Periodic OPTIONAL,
  IN UINTN                                           ActivationInterval OPTIONAL
  );

/**
  Clears any system state that was created in response to the Active call.

  @param  This                  The EFI_PEI_SMM_CONTROL2_PPI instance.
  @param  Periodic              Optional parameter to repeat at this period one 
                                time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS           The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR      The source could not be cleared.
  @retval EFI_INVALID_PARAMETER The service did not support the Periodic input argument.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SMM_DEACTIVATE2) (
  IN EFI_PEI_SMM_CONTROL2_PPI              *This,
  IN BOOLEAN                               Periodic OPTIONAL
  );

///
///  PEI SMM Control PPI is used to initiate SMI/PMI activations. This protocol could be published by either:
///  - A processor driver to abstract the SMI/PMI IPI
///  - The driver that abstracts the ASIC that is supporting the APM port, such as the ICH in an
///  Intel chipset
/// 
struct _EFI_PEI_SMM_CONTROL2_PPI {
  EFI_PEI_SMM_ACTIVATE2    Trigger;
  EFI_PEI_SMM_DEACTIVATE2  Clear;
  EFI_PEI_SMM_PERIOD       MinimumTriggerPeriod;
};

extern EFI_GUID gEfiPeiSmmControl2PpiGuid;

#endif
