/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H
#define _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H

#include <arch/vm_translation_map.h>


//gVirtFromPhysOffset = virtAdr - physAdr;
extern ssize_t gVirtFromPhysOffset;


// Removes a page-table entry inherited from the boot loader without applying
// VM page or mapping accounting. This is used for kernel stack guard pages,
// which must be genuinely unmapped even when their virtual address overlaps a
// loader-created mapping that is not represented by a VMArea.
void arch_vm_translation_map_clear_untracked_page(VMTranslationMap* map,
	addr_t virtualAddress);


static inline void*
VirtFromPhys(phys_addr_t physAdr)
{
	return (void*)(physAdr + gVirtFromPhysOffset);
}


static inline phys_addr_t
PhysFromVirt(void* virtAdr)
{
	return (phys_addr_t)virtAdr - gVirtFromPhysOffset;
}


#endif /* _KERNEL_ARCH_RISCV64_VM_TRANSLATION_MAP_H */
