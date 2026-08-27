/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DRIVER_H
#define DRIVER_H


#include <KernelExport.h>
#include <PCI.h>


#include <kernel/lock.h>


#include "radeon_hd.h"
#include "radeon_hd_private.h"


// PCI Communications

#define read8(address)   		(*((volatile uint8*)(address)))
#define read16(address)  		(*((volatile uint16*)(address)))
#define read32(address) 		(*((volatile uint32*)(address)))
#define write8(address, data)  	(*((volatile uint8*)(address)) = (data))
#define write16(address, data) 	(*((volatile uint16*)(address)) = (data))
#define write32(address, data) 	(*((volatile uint32*)(address)) = (data))


extern char* gDeviceNames[];
extern radeon_info* gDeviceInfo[];
extern pci_module_info* gPCI;
extern mutex gLock;


static inline uint32
get_pci_config(pci_info* info, uint8 offset, uint8 size)
{
	return gPCI->read_pci_config(info->bus, info->device, info->function,
		offset, size);
}


static inline void
set_pci_config(pci_info* info, uint8 offset, uint8 size, uint32 value)
{
	gPCI->write_pci_config(info->bus, info->device, info->function, offset,
		size, value);
}


static inline phys_addr_t
get_pci_rom_address(pci_info* info)
{
	// pci_info::rom_base predates 64-bit host physical addresses and can
	// truncate a translated PCI ROM BAR.  Translate the live PCI bus address
	// here instead (for example, SG2042 maps 0x50020000 to 0x4050020000).
	uint32 address = get_pci_config(info, PCI_rom_base, 4)
		& PCI_rom_address_mask;
	return gPCI->ram_address(address);
}


#endif  /* DRIVER_H */
