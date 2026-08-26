/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the NewOS License.
 */


#include "VMKernelArea.h"

#include <debug.h>
#include <heap.h>
#include <kernel.h>
#include <slab/Slab.h>
#include <vm/vm_priv.h>


VMKernelArea::VMKernelArea(VMAddressSpace* addressSpace, uint32 wiring,
	uint32 protection)
	:
	VMArea(addressSpace, wiring, protection),
	fBootstrapAllocated(false)
{
}


VMKernelArea::~VMKernelArea()
{
}


/*static*/ VMKernelArea*
VMKernelArea::Create(VMAddressSpace* addressSpace, const char* name,
	uint32 wiring, uint32 protection, ObjectCache* objectCache,
	uint32 allocationFlags)
{
#if defined(__riscv)
	bool* kernelStartup;
	asm volatile("lla %0, gKernelStartup" : "=r"(kernelStartup));
	VMKernelArea* area;
	if (*kernelStartup) {
		// The slab cache still enters its mutex/depot path here on the
		// 64-core Pioneer, although only the boot CPU is active.  These first
		// kernel areas live for the lifetime of the kernel, so allocate their
		// backing objects from the bootstrap arena.
		typedef void* (*block_alloc_early_func)(size_t);
		block_alloc_early_func directBlockAllocEarly;
		asm volatile("lla %0, _Z17block_alloc_earlym"
			: "=r"(directBlockAllocEarly));
		void* storage = directBlockAllocEarly(sizeof(VMKernelArea));
		area = static_cast<VMKernelArea*>(storage);
		if (area != NULL) {
			uint8* bytes = reinterpret_cast<uint8*>(area);
			for (size_t i = 0; i < sizeof(VMKernelArea); i++)
				bytes[i] = 0;
			area->SetBootstrapAllocated();
		}
	} else {
		area = new(objectCache, allocationFlags) VMKernelArea(
			addressSpace, wiring, protection);
	}
#else
	VMKernelArea* area = new(objectCache, allocationFlags) VMKernelArea(
		addressSpace, wiring, protection);
#endif
	if (area == NULL)
		return NULL;

#if defined(__riscv)
	status_t status;
	if (*kernelStartup) {
		typedef status_t (*area_init_bootstrap_func)(VMArea*, VMAddressSpace*,
			const char*, uint32, uint32, uint32);
		area_init_bootstrap_func directAreaInitBootstrap;
		asm volatile("lla %0, _ZN6VMArea13InitBootstrapEP14VMAddressSpacePKcjjj"
			: "=r"(directAreaInitBootstrap));
		status = directAreaInitBootstrap(area, addressSpace, name, wiring,
			protection, allocationFlags);
	} else
		status = area->Init(name, allocationFlags);
	if (status != B_OK) {
#else
	if (area->Init(name, allocationFlags) != B_OK) {
#endif
		if (area->IsBootstrapAllocated())
			area->~VMKernelArea();
		else
			object_cache_delete(objectCache, area);
		return NULL;
	}

	return area;
}
