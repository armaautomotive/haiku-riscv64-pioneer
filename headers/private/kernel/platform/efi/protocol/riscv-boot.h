/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#pragma once

#include <efi/types.h>


#define EFI_RISCV_BOOT_PROTOCOL_GUID \
	{0xccd15fec, 0x6f73, 0x4eec, {0x83, 0x95, 0x3e, 0x69, 0xe4, 0xb9, 0x40, 0xbf}}

#define EFI_RISCV_BOOT_PROTOCOL_REVISION 0x00010000


typedef struct efi_riscv_boot_protocol efi_riscv_boot_protocol;

struct efi_riscv_boot_protocol {
	uint64_t Revision;
	efi_status (*GetBootHartId)(efi_riscv_boot_protocol* self,
		size_t* bootHartId) EFIAPI;
};
