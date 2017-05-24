/** @file
  Entry point to the Standalone MM Foundation when initialised during the SEC
  phase on ARM platforms

Copyright (c) 2017, ARM Ltd. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MODULE_ENTRY_POINT_H__
#define __MODULE_ENTRY_POINT_H__

#include <Library/PeCoffLib.h>
#include <Library/FvLib.h>

#define CPU_INFO_FLAG_PRIMARY_CPU  0x00000001

typedef
EFI_STATUS
(*PI_MM_CPU_TP_FW_ENTRYPOINT) (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  );

typedef struct {
  UINT8  Type;       /* type of the structure */
  UINT8  Version;    /* version of this structure */
  UINT16 Size;      /* size of this structure in bytes */
  UINT32 Attr;      /* attributes: unused bits SBZ */
} EFI_PARAM_HEADER;

typedef struct {
  UINT64 Mpidr;
  UINT32 LinearId;
  UINT32 Flags;
} EFI_SECURE_PARTITION_CPU_INFO;

typedef struct {
  EFI_PARAM_HEADER              Header;
  UINT64                        SpMemBase;
  UINT64                        SpMemLimit;
  UINT64                        SpImageBase;
  UINT64                        SpStackBase;
  UINT64                        SpHeapBase;
  UINT64                        SpNsCommBufBase;
  UINT64                        SpSharedBufBase;
  UINT32                        SpImageSize;
  UINT32                        SpPcpuStackSize;
  UINT32                        SpHeapSize;
  UINT32                        SpNsCommBufSize;
  UINT32                        SpPcpuSharedBufSize;
  UINT32                        NumSpMemRegions;
  UINT32                        NumCpus;
  EFI_SECURE_PARTITION_CPU_INFO *CpuInfo;
} EFI_SECURE_PARTITION_BOOT_INFO;

typedef struct {
  EFI_PARAM_HEADER               h;
  UINT64                         SpStackBase;
  UINT64                         SpSharedBufBase;
  UINT32                         SpPcpuStackSize;
  UINT32                         SpPcpuSharedBufSize;
  EFI_SECURE_PARTITION_CPU_INFO  CpuInfo;
} EFI_SECURE_PARTITION_WARM_BOOT_INFO;


typedef
EFI_STATUS
(*PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT) (
  IN UINTN EventId,
  IN UINTN CpuNumber,
  IN UINTN NsCommBufferAddr
  );

typedef struct {
  PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT *ArmTfCpuDriverEpPtr;
} ARM_TF_CPU_DRIVER_EP_DESCRIPTOR;

typedef RETURN_STATUS (*REGION_PERMISSION_UPDATE_FUNC) (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  );

/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function sets the correct permissions of
  sections in the Standalone MM Core module to be able to access RO and RW data
  and make further progress in the boot process.

  @param  ImageContext           Pointer to PE/COFF image context
  @param  SectionHeaderOffset    Offset of PE/COFF image section header
  @param  NumberOfSections       Number of Sections
  @param  TextUpdater            Function to change code permissions
  @param  ReadOnlyUpdater        Function to change RO permissions
  @param  ReadWriteUpdater       Function to change RW permissions

**/
EFI_STATUS
EFIAPI
UpdateMmFoundationPeCoffPermissions (
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT      *ImageContext,
  IN  UINT32                                  SectionHeaderOffset,
  IN  CONST  UINTN                            NumberOfSections,
  IN  REGION_PERMISSION_UPDATE_FUNC           TextUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC           ReadOnlyUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC           ReadWriteUpdater
  );


/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function locates the section information of
  the Standalone MM Core module to be able to change permissions of the
  individual sections later in the boot process.

  @param  TeData                 Pointer to PE/COFF image data
  @param  ImageContext           Pointer to PE/COFF image context
  @param  SectionHeaderOffset    Offset of PE/COFF image section header
  @param  NumberOfSections       Number of Sections

**/
EFI_STATUS
EFIAPI
GetStandaloneSmmCorePeCoffSections (
  IN        VOID                            *TeData,
  IN  OUT   PE_COFF_LOADER_IMAGE_CONTEXT    *ImageContext,
  IN  OUT   UINT32                          *SectionHeaderOffset;
  IN  OUT   UINTN                           *NumberOfSections;
  );


/**
  Privileged firmware assigns RO & Executable attributes to all memory occupied
  by the Boot Firmware Volume. This function locates the Standalone MM Core
  module PE/COFF image in the BFV and returns this information.

  @param  BfvAddress             Base Address of Boot Firmware Volume
  @param  TeData                 Pointer to address for allocating memory for
                                 PE/COFF image data
  @param  TeDataSize             Pointer to size of PE/COFF image data

**/
EFI_STATUS
EFIAPI
LocateStandaloneSmmCorePeCoffData (
  IN        EFI_FIRMWARE_VOLUME_HEADER      *BfvAddress,
  IN  OUT   VOID                            **TeData;
  IN  OUT   UINTN                           *TeDataSize;
  );


/**
  Use the boot information passed by privileged firmware to populate a HOB list
  suitable for consumption by the MM Core and drivers.

  @param  CpuDriverEntryPoint    Address of MM CPU driver entrypoint
  @param  PayloadBootInfo        Boot information passed by privileged firmware

**/
VOID *
EFIAPI
CreateHobListFromBootInfo (
  IN  OUT  PI_MM_ARM_TF_CPU_DRIVER_ENTRYPOINT *CpuDriverEntryPoint,
  IN       EFI_SECURE_PARTITION_BOOT_INFO     *PayloadBootInfo
  );


/**
  The entry point of Standalone MM Foundation.

  @param  HobStart  Pointer to the beginning of the HOB List.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN VOID    *SharedBufAddress,
  IN UINT64  SharedBufSize,
  IN UINT64  cookie1,
  IN UINT64  cookie2
  );


/**
  Autogenerated function that calls the library constructors for all of the module's dependent libraries.

  This function must be called by _ModuleEntryPoint().
  This function calls the set of library constructors for the set of library instances
  that a module depends on.  This includes library instances that a module depends on
  directly and library instances that a module depends on indirectly through other
  libraries. This function is autogenerated by build tools and those build tools are
  responsible for collecting the set of library instances, determine which ones have
  constructors, and calling the library constructors in the proper order based upon
  each of the library instances own dependencies.

  @param  ImageHandle  The image handle of the DXE Core.
  @param  SystemTable  A pointer to the EFI System Table.

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SMM_SYSTEM_TABLE2  *SmmSystemTable
  );


/**
  Autogenerated function that calls a set of module entry points.

  This function must be called by _ModuleEntryPoint().
  This function calls the set of module entry points.
  This function is autogenerated by build tools and those build tools are responsible
  for collecting the module entry points and calling them in a specified order.

  @param  HobStart  Pointer to the beginning of the HOB List passed in from the PEI Phase.

**/
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN VOID  *HobStart
  );

#endif
