/** @file
  Rewrite the HEST tables GHES records, allocating 64K per HEST for the Error
  Status Address. On a real system this would be baked in the platform memory map.

  Some of this code poached from AppPkg/Applications/Sockets/WebServer/ACPI.c:
  Copyright (c) 2011-2012, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Guid/Acpi.h>
#include <IndustryStandard/Acpi60.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#pragma pack(1)

typedef struct {
	UINT32 Signature;
	UINT32 Length;
	UINT8 Revision;
	UINT8 Checksum;
	UINT8 OemId[6];
	UINT8 OemTableId[8];
	UINT32 OemRevision;
	UINT32 CreatorId;
	UINT32 CreatorRevision;
	UINT32 Entry[1];
} ACPI_RSDT;

/**
  Locate the RSDT table

  @return  Table address or NULL if not found

**/
CONST ACPI_RSDT *LocateRsdt (VOID)
{
	CONST EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp10b;
	CONST EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp30;
	CONST ACPI_RSDT * pRsdt;
	EFI_STATUS Status;

	//
	//  Use for/break instead of goto
	//
	pRsdt = NULL;
	for ( ; ; ) {
	  	//
		//  Locate the RSDT
		//
		Status = EfiGetSystemConfigurationTable ( &gEfiAcpiTableGuid, (VOID **)&pRsdp30 );
		if ( !EFI_ERROR ( Status )) {
			pRsdt = (ACPI_RSDT *)(UINTN)pRsdp30->RsdtAddress;
		} else {
			Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, (VOID **)&pRsdp10b );
			if ( EFI_ERROR ( Status ))
				break;

			pRsdt = (ACPI_RSDT *)(UINTN)pRsdp10b->RsdtAddress;
		}

		break;
	}

	//
	//  The entry was not found
	//
	return pRsdt;
}



/**
  Locate the specified table

  @param [in] Signature     Table signature

  @return  Table address or NULL if not found

**/
CONST VOID *LocateTable (IN UINT32 Signature)
{
	CONST UINT32 * pEnd;
	CONST UINT32 * pEntry;
	CONST EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp10b;
	CONST EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER * pRsdp30;
	CONST ACPI_RSDT * pRsdt;
	CONST UINT32 * pSignature;
	EFI_STATUS Status;

	//
	//  Use for/break instead of goto
	//
	for ( ; ; ) {
		//
		//  Locate the RSDT
		//
		Status = EfiGetSystemConfigurationTable ( &gEfiAcpiTableGuid, (VOID **)&pRsdp30 );
		if ( !EFI_ERROR ( Status )) {
			pRsdt = (ACPI_RSDT *)(UINTN)pRsdp30->RsdtAddress;
		} else {
			Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, (VOID **)&pRsdp10b );
			if ( EFI_ERROR ( Status ))
				break;

			pRsdt = (ACPI_RSDT *)(UINTN)pRsdp10b->RsdtAddress;
		}

		//
		//  Walk the list of entries
		//
		pEntry = &pRsdt->Entry[ 0 ];
		pEnd = &pEntry[(( pRsdt->Length - sizeof ( *pRsdt )) >> 2 ) + 1 ];
		while ( pEnd > pEntry ) {
			//
			//  The entry is actually a 32-bit physical table address
			//  The first entry in the table is the 32-bit table signature
			//
			pSignature = (UINT32 *)(UINTN)*pEntry;
			if ( *pSignature == Signature )
				return (CONST VOID *)(UINTN)*pEntry;

			//
			//  Set the next entry
			//
			pEntry++;
		}

		break;
	}

	//
	//  The entry was not found
	//
	return NULL;
}

EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	int i;
	EFI_STATUS Status;
	EFI_PHYSICAL_ADDRESS alloc;
	char hest_sig[4] = {'H', 'E', 'S', 'T'};
	CONST EFI_ACPI_6_0_HARDWARE_ERROR_SOURCE_TABLE_HEADER *hest;
	EFI_ACPI_6_0_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE *ghes;

	Print (L"HACK! Mangling the HEST table\r\n");

	hest = LocateTable (*(UINT32 *)hest_sig);
	if (!hest) {
		Print (L"Failed to find the HEST table\r\n");
		return EFI_SUCCESS;
	}

	ghes = (EFI_ACPI_6_0_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE *)(hest + 1);
	for (i = 0; i < hest->ErrorSourceCount; i++) {
		if (ghes->Type != EFI_ACPI_6_0_GENERIC_HARDWARE_ERROR) {
			Print (L"Stopped at non-ghes entry %u\r\n", i);
			return EFI_SUCCESS;
		}

		if (ghes->ErrorStatusAddress.AddressSpaceId == EFI_ACPI_6_0_SYSTEM_MEMORY &&
		    ghes->ErrorStatusAddress.Address == 0) {
			Status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiACPIMemoryNVS, EFI_SIZE_TO_PAGES (SIZE_64KB), &alloc);
			if (!EFI_ERROR (Status))
				ghes->ErrorStatusAddress.Address = alloc;
			else
				Print (L"Failed to allocate memory for GHES entry %u\r\n", i);
		}

		ghes += 1;
	}

	return EFI_SUCCESS;
}
