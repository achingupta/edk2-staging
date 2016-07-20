/** @file

  Copyright (c) 2016, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiLib.h>
#include <Library/CpuLib.h>
#include <Library/DefaultExceptionHandlerLib.h>
#include <Library/DebugLib.h>

#include <Guid/DebugImageInfoTable.h>

#include <Protocol/Cpu.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/DebugSupportPeriodicCallback.h>
#include <Protocol/VirtualUncachedPages.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/MmCommunication.h>

#include <IndustryStandard/ArmStdSmc.h>

#include <Pi/PiDxeCis.h>

/**
  Communicates with a registered handler.

  This function provides an interface to send and receive messages to the
  Standalone MM environment on behalf of UEFI services.  This function is part
  of the MM Communication Protocol that may be called in physical mode prior to
  SetVirtualAddressMap() and in virtual mode after SetVirtualAddressMap().

  @param[in]      This                The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBuffer          A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize            The size of the data buffer being passed in.On exit, the size of data
                                      being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation. If this error is
                                      returned, the MessageLength field in the CommBuffer header or the integer
                                      pointed by CommSize are updated to reflect the maximum payload size the
                                      implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter or CommSize parameter, if not omitted,
                                      are in address range that cannot be accessed by the MM environment
**/
EFI_STATUS
EFIAPI
MmCommunicationCommunicate (
  IN CONST EFI_MM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                             *CommBuffer,
  IN OUT UINTN                            *CommSize    OPTIONAL
  );

//
// Address of the pre-allocated buffer for communication with the secure
// world.
//
VOID *mNsBufferAddress = NULL;

//
// Maximum size of a non-secure buffer that can be tolerated by the secure
// world.
//
UINTN mNsBufferMaxSize = 0;

//
// Handle to install the MM Communication Protocol
//
EFI_HANDLE mMmCommunicateHandle = NULL;

//
// MM Communication Protocol instance
//
EFI_MM_COMMUNICATION_PROTOCOL  mMmCommunication = {
  MmCommunicationCommunicate
};

/**
  Communicates with a registered handler.

  This function provides an interface to send and receive messages to the
  Standalone MM environment on behalf of UEFI services.  This function is part
  of the MM Communication Protocol that may be called in physical mode prior to
  SetVirtualAddressMap() and in virtual mode after SetVirtualAddressMap().

  @param[in]      This                The EFI_MM_COMMUNICATION_PROTOCOL instance.
  @param[in, out] CommBuffer          A pointer to the buffer to convey into SMRAM.
  @param[in, out] CommSize            The size of the data buffer being passed in.On exit, the size of data
                                      being returned. Zero if the handler does not wish to reply with any data.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer is too large for the MM implementation. If this error is
                                      returned, the MessageLength field in the CommBuffer header or the integer
                                      pointed by CommSize are updated to reflect the maximum payload size the
                                      implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter or CommSize parameter, if not omitted,
                                      are in address range that cannot be accessed by the MM environment
**/
EFI_STATUS
EFIAPI
MmCommunicationCommunicate (
  IN CONST EFI_MM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                             *CommBuffer,
  IN OUT UINTN                            *CommSize
  )
{
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader = CommBuffer;
  ARM_SMC_ARGS                CommunicateSmcArgs = {0};
  EFI_STATUS                  Status = EFI_SUCCESS;
  UINTN                      *BufferSizeAddress, BufferSize;
//  volatile UINTN  Index;

//  for (Index = 0; Index == 0;);

  DEBUG ((EFI_D_INFO, "MmCommunicationCommunicate Communicate Enter - 0x%x\n", CommBuffer));

  //
  // TODO: Make communication MP safe in this driver. At the moment the callers
  // are expected to do this on its behalf.
  //

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // If the length of the CommBuffer is 0 then return the expected length.
  if (CommSize) {
    BufferSizeAddress = CommSize;
    BufferSize = *CommSize;

    //
    // If an intermediate buffer is being used then allow space to copy
    // the CommSize parameter as well.
    //
    if (mNsBufferAddress) {
      BufferSize += sizeof(UINTN);
    }
  } else {
    BufferSizeAddress = &CommunicateHeader->MessageLength;
    BufferSize = CommunicateHeader->MessageLength;
  }

  //
  // If the buffer size if 0 or greater than what can be tolerated by the MM
  // environment then return the expected size.
  //
  if (!*BufferSizeAddress || (BufferSize > mNsBufferMaxSize)) {
    if (mNsBufferAddress && CommSize) {
      *BufferSizeAddress = mNsBufferMaxSize - sizeof(UINTN);
    } else {
      *BufferSizeAddress = mNsBufferMaxSize;
    }
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // CommSize must hold HeaderGuid and MessageLength
  //
  if (CommSize && *CommSize < OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_LOAD | DEBUG_INFO, "0x%x - 0x%x - 0x%x \n", mNsBufferAddress, CommBuffer, *BufferSizeAddress));

  if (mNsBufferAddress) {
    CopyMem(mNsBufferAddress, CommBuffer, *BufferSizeAddress);
    CommunicateSmcArgs.Arg1 = (UINTN) mNsBufferAddress;

    // Copy the CommSize if used
    if (CommSize) {
      BufferSizeAddress = mNsBufferAddress + *BufferSizeAddress;
      CopyMem(BufferSizeAddress, CommSize, sizeof(UINTN));
    }
  } else {
    CommunicateSmcArgs.Arg1 = (UINTN) CommBuffer;
    CommunicateSmcArgs.Arg2 = (UINTN) CommSize;
  }

  // Call the Standalone MM environment. The CommSize parameter is omitted
  CommunicateSmcArgs.Arg0 = ARM_SMC_ID_MM_COMMUNICATE_AARCH64;
  ArmCallSmc(&CommunicateSmcArgs);

  Status = CommunicateSmcArgs.Arg0;
  switch (Status) {
  case EFI_SUCCESS:
    break;

  case ARM_SMC_MM_RET_NOT_SUPPORTED:
  case ARM_SMC_MM_RET_INVALID_PARAMS:
    Status = EFI_INVALID_PARAMETER;
    break;

  case ARM_SMC_MM_RET_DENIED:
    Status = EFI_ACCESS_DENIED;
    break;

  case ARM_SMC_MM_RET_NO_MEMORY:
    // TODO: Unexpected error since the CommSize was checked for zero length
    // prior to issuing the SMC
  default:
          ASSERT (0);
  }

  return Status;
}

/**
  The Entry Point for MM Communication

  This function installs the MM communication protocol interface and finds out
  what type of buffer management will be required prior to invoking the
  communication SMC.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MmCommunicationInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_MM_COMMUNICATE_HEADER  CommunicateHeader;
  ARM_SMC_ARGS               MmSmcArgs = {0};
  EFI_STATUS                 Status;

  // Install the communication protocol
  Status = gBS->InstallProtocolInterface (&mMmCommunicateHandle,
                                          &gEfiMmCommunicationProtocolGuid,
                                          EFI_NATIVE_INTERFACE,
                                          &mMmCommunication);
  if (Status != EFI_SUCCESS)
    return Status;

  //
  // Query the secure world to obtain the extents of a pre-allocated buffer for
  // communication. In its absence, the caller provided buffer will be passed as
  // is to the secure world. Upon a successful return the values would be
  // populated as follows:
  //
  // X0 = ARM_SMC_MM_RET_SUCCESS
  // X1 = Address of buffer aligned to the 4K boundary
  // X2 = Size of the buffer
  //
  // TODO: This approach currently assumes that the extents of this buffer were
  //       populated in the virtual memory map and mapped into the translation
  //       tables during the PEI phase. In the absence of a simple mechanism for
  //       the PEI phase to convey the buffer details to this driver, it is
  //       simpler to ask the secure world directly since the secure and
  //       non-secure views of this buffer should be the same.
  //
  //       There would be no dependence on the PEI phase if the GCD
  //       AddMemorySpace() service was able to dynamically map a buffer in the
  //       translation tables instead of just the UEFI memory map. It would then
  //       be a simple matter of obtaining the buffer extents from the secure
  //       world and mapping it in.
  //
  MmSmcArgs.Arg0 = ARM_SMC_ID_MM_GET_NS_BUFFER_AARCH64;
  ArmCallSmc(&MmSmcArgs);
  if (MmSmcArgs.Arg0 == ARM_SMC_MM_RET_SUCCESS) {
    mNsBufferAddress = (UINTN *) MmSmcArgs.Arg1;
    mNsBufferMaxSize = MmSmcArgs.Arg2;

    // TODO: Add support to map this buffer here as described previously

    ZeroMem(mNsBufferAddress, mNsBufferMaxSize);
    goto exit;
  }

  //
  // If the ARM_SMC_ID_MM_GET_NS_BUFFER_AARCH64 did not work then rely on PCD
  // values to obtain this information. if the buffer address is 0 then it is
  // assumed that the caller of this protocol will provide a reference to the
  // communication buffer.
  //
  mNsBufferAddress = (VOID *) PcdGet64(PcdMmBufferBase);
  mNsBufferMaxSize = PcdGet64(PcdMmBufferSize);

  if (mNsBufferMaxSize) {
    goto exit;
  }

  //
  // If the maximum buffer size has still not been obtained then issue the
  // communicate SMC with a zero buffer size. Secure world should respond with
  // the maximum buffer size that it can tolerate.
  //
  // TODO: The secure world implementation does not support this approach
  //       currently.
  //
  ZeroMem (&MmSmcArgs, sizeof (ARM_SMC_ARGS));
  ZeroMem (&CommunicateHeader, sizeof (EFI_MM_COMMUNICATE_HEADER));
  MmSmcArgs.Arg0 = ARM_SMC_ID_MM_COMMUNICATE_AARCH64;
  MmSmcArgs.Arg1 = (UINTN) &CommunicateHeader;
  ArmCallSmc(&MmSmcArgs);

  // Read the maximum size from the communicate header
  ASSERT (MmSmcArgs.Arg0 == ARM_SMC_MM_RET_NO_MEMORY);
  mNsBufferMaxSize = CommunicateHeader.MessageLength;

exit:
  DEBUG ((DEBUG_INFO, "MmCommunicateInitialize: mNsBufferAddress - 0x%x, mNsBufferMaxSize - 0x%x\n", mNsBufferAddress, mNsBufferMaxSize));
  return Status;
}
