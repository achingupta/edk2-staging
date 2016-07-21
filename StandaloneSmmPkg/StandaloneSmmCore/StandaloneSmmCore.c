/** @file
  SMM Core Main Entry Point

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available 
  under the terms and conditions of the BSD License which accompanies this 
  distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "StandaloneSmmCore.h"

EFI_STATUS
SmmCoreFfsFindSmmDriver (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader
  );

EFI_STATUS
SmmDispatcher (
  VOID
  );

//
// Physical pointer to private structure shared between SMM IPL and the SMM Core
//
SMM_CORE_PRIVATE_DATA  *gSmmCorePrivate;

//
// SMM Core global variable for SMM System Table.  Only accessed as a physical structure in SMRAM.
//
EFI_SMM_SYSTEM_TABLE2  gSmmCoreSmst = {
  {
    SMM_SMST_SIGNATURE,
    EFI_SMM_SYSTEM_TABLE2_REVISION,
    sizeof (gSmmCoreSmst.Hdr)
  },
  NULL,                          // SmmFirmwareVendor
  0,                             // SmmFirmwareRevision
  SmmInstallConfigurationTable,
  {
    {
      (EFI_SMM_CPU_IO2) SmmEfiNotAvailableYetArg5,       // SmmMemRead
      (EFI_SMM_CPU_IO2) SmmEfiNotAvailableYetArg5        // SmmMemWrite
    },
    {
      (EFI_SMM_CPU_IO2) SmmEfiNotAvailableYetArg5,       // SmmIoRead
      (EFI_SMM_CPU_IO2) SmmEfiNotAvailableYetArg5        // SmmIoWrite
    }
  },
  SmmAllocatePool,
  SmmFreePool,
  SmmAllocatePages,
  SmmFreePages,
  NULL,                          // SmmStartupThisAp
  0,                             // CurrentlyExecutingCpu
  0,                             // NumberOfCpus
  NULL,                          // CpuSaveStateSize
  NULL,                          // CpuSaveState
  0,                             // NumberOfTableEntries
  NULL,                          // SmmConfigurationTable
  SmmInstallProtocolInterface,
  SmmUninstallProtocolInterface,
  SmmHandleProtocol,
  SmmRegisterProtocolNotify,
  SmmLocateHandle,
  SmmLocateProtocol,
  SmiManage,
  SmiHandlerRegister,
  SmiHandlerUnRegister
};

//
// Flag to determine if the platform has performed a legacy boot.
// If this flag is TRUE, then the runtime code and runtime data associated with the 
// SMM IPL are converted to free memory, so the SMM COre must guarantee that is
// does not touch of the code/data associated with the SMM IPL if this flag is TRUE.
//
BOOLEAN  mInLegacyBoot = FALSE;

//
// Table of SMI Handlers that are registered by the SMM Core when it is initialized
//
SMM_CORE_SMI_HANDLERS  mSmmCoreSmiHandlers[] = {
  { SmmUefiInfoHandler,       &gSmmUefiInfoGuid,                  NULL, TRUE  },
  { SmmFvDispatchHandler,     &gSmmFvDispatchGuid,                NULL, TRUE  },
  { SmmDriverDispatchHandler, &gEfiEventDxeDispatchGuid,          NULL, TRUE  },
  { SmmReadyToLockHandler,    &gEfiDxeSmmReadyToLockProtocolGuid, NULL, TRUE  }, 
  { SmmEndOfDxeHandler,       &gEfiEndOfDxeEventGroupGuid,        NULL, FALSE },
  { SmmLegacyBootHandler,     &gEfiEventLegacyBootGuid,           NULL, FALSE },
  { SmmExitBootServiceHandler,&gEfiEventExitBootServicesGuid,     NULL, FALSE },
  { SmmReadyToBootHandler,    &gEfiEventReadyToBootGuid,          NULL, FALSE },
  { NULL,                     NULL,                               NULL, FALSE },
};

EFI_SYSTEM_TABLE                *mEfiSystemTable;

UINTN                           mSmramRangeCount;
EFI_SMRAM_DESCRIPTOR            *mSmramRanges;

/**
  Place holder function until all the SMM System Table Service are available.

  Note: This function is only used by SMRAM invocation.  It is never used by DXE invocation.

  @param  Arg1                   Undefined
  @param  Arg2                   Undefined
  @param  Arg3                   Undefined
  @param  Arg4                   Undefined
  @param  Arg5                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
SmmEfiNotAvailableYetArg5 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4,
  UINTN Arg5
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.
  //
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Software SMI handler that is called when a Legacy Boot event is signalled.  The SMM
  Core uses this signal to know that a Legacy Boot has been performed and that 
  gSmmCorePrivate that is shared between the UEFI and SMM execution environments can
  not be accessed from SMM anymore since that structure is considered free memory by
  a legacy OS.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmLegacyBootHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_HANDLE  SmmHandle;
  EFI_STATUS  Status = EFI_SUCCESS;

  if (!mInLegacyBoot) {
    SmmHandle = NULL;
    Status = SmmInstallProtocolInterface (
               &SmmHandle,
               &gEfiEventLegacyBootGuid,
               EFI_NATIVE_INTERFACE,
               NULL
               );
  }
  mInLegacyBoot = TRUE;
  return Status;
}

/**
  Software SMI handler that is called when a ExitBoot Service event is signalled.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmExitBootServiceHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_HANDLE  SmmHandle;
  EFI_STATUS  Status = EFI_SUCCESS;
  STATIC BOOLEAN mInExitBootServices = FALSE;

  if (!mInExitBootServices) {
    SmmHandle = NULL;
    Status = SmmInstallProtocolInterface (
               &SmmHandle,
               &gEfiEventExitBootServicesGuid,
               EFI_NATIVE_INTERFACE,
               NULL
               );
  }
  mInExitBootServices = TRUE;
  return Status;
}

/**
  Software SMI handler that is called when a ExitBoot Service event is signalled.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmReadyToBootHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_HANDLE  SmmHandle;
  EFI_STATUS  Status = EFI_SUCCESS;
  STATIC BOOLEAN mInReadyToBoot = FALSE;

  if (!mInReadyToBoot) {
    SmmHandle = NULL;
    Status = SmmInstallProtocolInterface (
               &SmmHandle,
               &gEfiEventReadyToBootGuid,
               EFI_NATIVE_INTERFACE,
               NULL
               );
  }
  mInReadyToBoot = TRUE;
  return Status;
}

/**
  Software SMI handler that is called when the DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signalled.  This function unregisters the 
  Software SMIs that are nor required after SMRAM is locked and installs the 
  SMM Ready To Lock Protocol so SMM Drivers are informed that SMRAM is about 
  to be locked.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmReadyToLockHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_HANDLE  SmmHandle;

  DEBUG ((EFI_D_INFO, "SmmReadyToLockHandler\n"));

  //
  // Unregister SMI Handlers that are no required after the SMM driver dispatch is stopped
  //
  for (Index = 0; mSmmCoreSmiHandlers[Index].HandlerType != NULL; Index++) {
    if (mSmmCoreSmiHandlers[Index].UnRegister) {
      SmiHandlerUnRegister (mSmmCoreSmiHandlers[Index].DispatchHandle);
    }
  }

  //
  // Install SMM Ready to lock protocol
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEfiSmmReadyToLockProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );

  //
  // Display any drivers that were not dispatched because dependency expression
  // evaluated to false if this is a debug build
  //
  DEBUG_CODE_BEGIN ();
    SmmDisplayDiscoveredNotDispatched ();
  DEBUG_CODE_END ();

  return Status;
}

/**
  Software SMI handler that is called when the EndOfDxe event is signalled.
  This function installs the SMM EndOfDxe Protocol so SMM Drivers are informed that
  platform code will invoke 3rd part code.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmEndOfDxeHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  SmmHandle;

  DEBUG ((EFI_D_INFO, "SmmEndOfDxeHandler\n"));
  //
  // Install SMM EndOfDxe protocol
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEfiSmmEndOfDxeProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );
  return Status;
}

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmUefiInfoHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  )
{
  EFI_SMM_COMMUNICATE_UEFI_INFO_DATA    *CommunicationData;
  EFI_HANDLE                            SmmHandle;
  EFI_STATUS                            Status = EFI_SUCCESS;

  DEBUG ((EFI_D_INFO, "SmmUefiInfoHandler\n"));

  CommunicationData = CommBuffer;
  
  DEBUG ((EFI_D_INFO, "  SystemTable - 0x%016lx\n", CommunicationData->EfiSystemTable));
  
  if (mEfiSystemTable == NULL) {
    mEfiSystemTable = (EFI_SYSTEM_TABLE *)(UINTN)CommunicationData->EfiSystemTable;
    SmmHandle = NULL;
    Status = SmmInstallProtocolInterface (
               &SmmHandle,
               &gSmmUefiInfoGuid,
               EFI_NATIVE_INTERFACE,
               mEfiSystemTable
               );
  }

  return Status;
}

/**
  The main entry point to SMM Foundation.

  Note: This function is only used by SMRAM invocation.  It is never used by DXE invocation.

  @param  SmmEntryContext           Processor information and functionality
                                    needed by SMM Foundation.

**/
VOID
EFIAPI
SmmEntryPoint (
  IN CONST EFI_SMM_ENTRY_CONTEXT  *SmmEntryContext
)
{
  EFI_STATUS                  Status;
  EFI_SMM_COMMUNICATE_HEADER  *CommunicateHeader;
  BOOLEAN                     InLegacyBoot;

  //DEBUG ((EFI_D_INFO, "SmmEntryPoint ...\n"));

  //
  // Update SMST using the context
  //
  CopyMem (&gSmmCoreSmst.SmmStartupThisAp, SmmEntryContext, sizeof (EFI_SMM_ENTRY_CONTEXT));

  //
  // Do not use private data structure
  //

  //
  // If a legacy boot has occured, then make sure gSmmCorePrivate is not accessed
  //
  InLegacyBoot = mInLegacyBoot;
  if (!InLegacyBoot) {
    //
    // TBD: Mark the InSmm flag as TRUE
    //
    gSmmCorePrivate->InSmm = TRUE;

    //
    // Check to see if this is a Synchronous SMI sent through the SMM Communication 
    // Protocol or an Asynchronous SMI
    //
    if (gSmmCorePrivate->CommunicationBuffer != 0) {
      //
      // Synchronous SMI for SMM Core or request from Communicate protocol
      //
      if (!SmmIsBufferOutsideSmmValid ((UINTN)gSmmCorePrivate->CommunicationBuffer, gSmmCorePrivate->BufferSize)) {
        //
        // If CommunicationBuffer is not in valid address scope, return EFI_INVALID_PARAMETER
        //
        gSmmCorePrivate->CommunicationBuffer = 0;
        gSmmCorePrivate->ReturnStatus = EFI_INVALID_PARAMETER;
      } else {
        CommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)(UINTN)gSmmCorePrivate->CommunicationBuffer;
        gSmmCorePrivate->BufferSize -= OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
        //DEBUG ((EFI_D_INFO, "CommunicateHeader->HeaderGuid - %g\n", &CommunicateHeader->HeaderGuid));
        Status = SmiManage (
                   &CommunicateHeader->HeaderGuid, 
                   NULL, 
                   CommunicateHeader->Data, 
                   (UINTN *)&gSmmCorePrivate->BufferSize
                   );
        //
        // Update CommunicationBuffer, BufferSize and ReturnStatus
        // Communicate service finished, reset the pointer to CommBuffer to NULL
        //
        gSmmCorePrivate->BufferSize += OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
        gSmmCorePrivate->CommunicationBuffer = 0;
        gSmmCorePrivate->ReturnStatus = (Status == EFI_SUCCESS) ? EFI_SUCCESS : EFI_NOT_FOUND;
      }
    }
  }

  //
  // Process Asynchronous SMI sources
  //
  SmiManage (NULL, NULL, NULL, NULL);
  
  //
  // TBD: Do not use private data structure ?
  //

  //
  // If a legacy boot has occured, then make sure gSmmCorePrivate is not accessed
  //
  if (!InLegacyBoot) {
    //
    // Clear the InSmm flag as we are going to leave SMM
    //
    gSmmCorePrivate->InSmm = FALSE;
  }
  
  //DEBUG ((EFI_D_INFO, "SmmEntryPoint Done\n"));
}

EFI_STATUS
EFIAPI
SmmConfigurationSmmNotify (
  IN CONST EFI_GUID *Protocol,
  IN VOID           *Interface,
  IN EFI_HANDLE     Handle
  )
{
  EFI_STATUS                      Status;
  EFI_SMM_CONFIGURATION_PROTOCOL  *SmmConfiguration;

  DEBUG ((EFI_D_INFO, "SmmConfigurationSmmNotify(%g) - %x\n", Protocol, Interface));
  
  SmmConfiguration = Interface;

  //
  // Register the SMM Entry Point provided by the SMM Core with the SMM COnfiguration protocol
  //
  Status = SmmConfiguration->RegisterSmmEntry (SmmConfiguration, (EFI_SMM_ENTRY_POINT)(UINTN)gSmmCorePrivate->SmmEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Set flag to indicate that the SMM Entry Point has been registered which 
  // means that SMIs are now fully operational.
  //
  gSmmCorePrivate->SmmEntryPointRegistered = TRUE;

  //
  // Print debug message showing SMM Core entry point address.
  //
  DEBUG ((DEBUG_INFO, "SMM Core registered SMM Entry Point address %p\n", (VOID *)(UINTN)gSmmCorePrivate->SmmEntryPoint));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MmConfigurationMmNotify (
  IN CONST EFI_GUID *Protocol,
  IN VOID           *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS                      Status;
  EFI_MM_CONFIGURATION_PROTOCOL  *MmConfiguration;

  DEBUG ((EFI_D_INFO, "MmConfigurationMmNotify(%g) - %x\n", Protocol, Interface));

  MmConfiguration = Interface;

  //
  // Register the MM Entry Point provided by the MM Core with the MM COnfiguration protocol
  //
  Status = MmConfiguration->RegisterMmFoundationEntry (MmConfiguration, (EFI_SMM_ENTRY_POINT)(UINTN)gSmmCorePrivate->SmmEntryPoint);
  ASSERT_EFI_ERROR (Status);

  //
  // Set flag to indicate that the MM Entry Point has been registered which
  // means that MMIs are now fully operational.
  //
  gSmmCorePrivate->SmmEntryPointRegistered = TRUE;

  //
  // Print debug message showing MM Core entry point address.
  //
  DEBUG ((DEBUG_INFO, "MM Core registered MM Entry Point address %p\n", (VOID *)(UINTN)gSmmCorePrivate->SmmEntryPoint));
  return EFI_SUCCESS;
}

UINTN
GetHobListSize (
  IN VOID *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (HobStart != NULL);
   
  Hob.Raw = (UINT8 *) HobStart;
  while (!END_OF_HOB_LIST (Hob)) {
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  //
  // Need plus END_OF_HOB_LIST
  //
  return (UINTN)Hob.Raw - (UINTN)HobStart + sizeof(EFI_HOB_GENERIC_HEADER);
}

/**
  The Entry Point for SMM Core

  Install DXE Protocols and reload SMM Core into SMRAM and register SMM Core 
  EntryPoint on the SMI vector.

  Note: This function is called for both DXE invocation and SMRAM invocation.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmMainStandalone (
  IN VOID  *HobStart
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  VOID                            *SmmHobStart;
  UINTN                           HobSize;
  VOID                            *Registration;
  EFI_HOB_GUID_TYPE               *GuidHob;
  SMM_CORE_DATA_HOB_DATA          *DataInHob;
  EFI_HOB_GUID_TYPE               *SmramRangesHob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *SmramRangesHobData;
  EFI_SMRAM_DESCRIPTOR            *SmramRanges;
  UINT32                          SmramRangeCount;
  EFI_HOB_FIRMWARE_VOLUME         *BfvHob;

  ProcessLibraryConstructorList (HobStart, &gSmmCoreSmst);

  DEBUG ((EFI_D_INFO, "SmmMain - 0x%x\n", HobStart));
  
  //
  // Determine if the caller has passed a reference to a SMM_CORE_PRIVATE_DATA
  // structure in the Hoblist. This choice will govern how boot information is
  // extracted later.
  //
  GuidHob = GetNextGuidHob (&gSmmCoreDataHobGuid, HobStart);
  if (GuidHob == NULL) {
    //
    // Allocate and zero memory for a SMM_CORE_PRIVATE_DATA table and then
    // initialise it
    //
    gSmmCorePrivate = (SMM_CORE_PRIVATE_DATA *) AllocateRuntimePages(EFI_SIZE_TO_PAGES(sizeof(SMM_CORE_PRIVATE_DATA)));
    SetMem ((VOID *)(UINTN)gSmmCorePrivate, sizeof(SMM_CORE_PRIVATE_DATA), 0);
    gSmmCorePrivate->Signature = SMM_CORE_PRIVATE_DATA_SIGNATURE;
    gSmmCorePrivate->SmmEntryPointRegistered = FALSE;
    gSmmCorePrivate->InSmm = FALSE;
    gSmmCorePrivate->ReturnStatus = EFI_SUCCESS;

    //
    // Extract the SMRAM ranges from the SMRAM descriptor HOB
    //
    SmramRangesHob = GetNextGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid, HobStart);
    if (SmramRangesHob == NULL)
      return EFI_UNSUPPORTED;

    SmramRangesHobData = GET_GUID_HOB_DATA (SmramRangesHob);
    ASSERT (SmramRangesHobData != NULL);
    SmramRanges = SmramRangesHobData->Descriptor;
    SmramRangeCount = SmramRangesHobData->NumberOfSmmReservedRegions;
    ASSERT (SmramRanges);
    ASSERT (SmramRangeCount);

    //
    // Copy the SMRAM ranges into SMM_CORE_PRIVATE_DATA table just in case any
    // code relies on them being present there
    //
    gSmmCorePrivate->SmramRangeCount = SmramRangeCount;
    gSmmCorePrivate->SmramRanges = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePool (SmramRangeCount * sizeof(EFI_SMRAM_DESCRIPTOR));
    ASSERT (gSmmCorePrivate->SmramRanges != 0);
    CopyMem ((VOID *)(UINTN)gSmmCorePrivate->SmramRanges,
             SmramRanges,
             SmramRangeCount * sizeof(EFI_SMRAM_DESCRIPTOR));
  } else {
    DataInHob       = GET_GUID_HOB_DATA (GuidHob);
    gSmmCorePrivate = (SMM_CORE_PRIVATE_DATA *)(UINTN)DataInHob->Address;
    SmramRanges     = (EFI_SMRAM_DESCRIPTOR *)(UINTN)gSmmCorePrivate->SmramRanges;
    SmramRangeCount = gSmmCorePrivate->SmramRangeCount;
  }

  //
  // Print the SMRAM ranges passed by the caller
  //
  DEBUG ((EFI_D_INFO, "SmramRangeCount - 0x%x\n", SmramRangeCount));
  for (Index = 0; Index < SmramRangeCount; Index++) {
          DEBUG ((EFI_D_INFO, "SmramRanges[%d]: 0x%016lx - 0x%lx\n", Index,
                  SmramRanges[Index].CpuStart,
                  SmramRanges[Index].PhysicalSize));
  }

  //
  // Copy the SMRAM ranges into private SMRAM
  //
  mSmramRangeCount = SmramRangeCount;
  DEBUG ((EFI_D_INFO, "mSmramRangeCount - 0x%x\n", mSmramRangeCount));
  mSmramRanges = AllocatePool (mSmramRangeCount * sizeof (EFI_SMRAM_DESCRIPTOR));
  DEBUG ((EFI_D_INFO, "mSmramRanges - 0x%x\n", mSmramRanges));
  ASSERT (mSmramRanges != NULL);
  CopyMem (mSmramRanges, (VOID *)(UINTN)SmramRanges, mSmramRangeCount * sizeof (EFI_SMRAM_DESCRIPTOR));

  //
  // Get Boot Firmware Volume address from the BFV Hob
  //
  BfvHob = GetFirstHob (EFI_HOB_TYPE_FV);
  if (BfvHob != NULL) {
    DEBUG ((EFI_D_INFO, "BFV address - 0x%x\n", BfvHob->BaseAddress));
    DEBUG ((EFI_D_INFO, "BFV size    - 0x%x\n", BfvHob->Length));
    gSmmCorePrivate->StandaloneBfvAddress = BfvHob->BaseAddress;
  }

  gSmmCorePrivate->Smst          = (EFI_PHYSICAL_ADDRESS)(UINTN)&gSmmCoreSmst;
  gSmmCorePrivate->SmmEntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)SmmEntryPoint;
  
  //
  // No need to initialize memory service.
  // It is done in constructor of StandaloneSmmCoreMemoryAllocationLib(),
  // so that the library linked with StandaloneSmmCore can use AllocatePool() in constuctor.
  //

  DEBUG ((EFI_D_INFO, "SmmInstallConfigurationTable For HobList\n"));
  //
  // Install HobList
  //
  HobSize = GetHobListSize (HobStart);
  DEBUG ((EFI_D_INFO, "HobSize - 0x%x\n", HobSize));
  SmmHobStart = AllocatePool (HobSize);
  DEBUG ((EFI_D_INFO, "SmmHobStart - 0x%x\n", SmmHobStart));
  ASSERT (SmmHobStart != NULL);
  CopyMem (SmmHobStart, HobStart, HobSize);
  Status = SmmInstallConfigurationTable (&gSmmCoreSmst, &gEfiHobListGuid, SmmHobStart, HobSize);
  ASSERT_EFI_ERROR (Status);
  //
  // Register all SMI Handlers required by the SMM Core
  //
  for (Index = 0; mSmmCoreSmiHandlers[Index].HandlerType != NULL; Index++) {
    Status = SmiHandlerRegister (
               mSmmCoreSmiHandlers[Index].Handler,
               mSmmCoreSmiHandlers[Index].HandlerType,
               &mSmmCoreSmiHandlers[Index].DispatchHandle
               );
    ASSERT_EFI_ERROR (Status);
  }
  

  DEBUG ((EFI_D_INFO, "SmmRegisterProtocolNotify - SmmConfigurationSmmProtocol\n"));
  Status = SmmRegisterProtocolNotify (
             &gEfiSmmConfigurationSmmProtocolGuid,
             SmmConfigurationSmmNotify,
             &Registration
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Register notification for EFI_MM_CONFIGURATION_PROTOCOL registration and
  // use it to register the MM Foundation entrypoint
  //
  DEBUG ((EFI_D_INFO, "MmRegisterProtocolNotify - MmConfigurationMmProtocol\n"));
  Status = SmmRegisterProtocolNotify (
             &gEfiMmConfigurationProtocolGuid,
             MmConfigurationMmNotify,
             &Registration
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Dispatch standalone BFV
  //
  DEBUG ((EFI_D_INFO, "Smm Dispatch StandaloneBfvAddress - 0x%08x\n", gSmmCorePrivate->StandaloneBfvAddress));
  if (gSmmCorePrivate->StandaloneBfvAddress != 0) {
    SmmCoreFfsFindSmmDriver ((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)gSmmCorePrivate->StandaloneBfvAddress);
    SmmDispatcher ();
  }
  
  DEBUG ((EFI_D_INFO, "SmmMain Done!\n"));

  return EFI_SUCCESS;
}
