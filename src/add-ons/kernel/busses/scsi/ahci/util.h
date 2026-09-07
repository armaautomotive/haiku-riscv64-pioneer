/*
 * Copyright 2004-2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __UTIL_H
#define __UTIL_H

#include <KernelExport.h>

#ifdef __cplusplus
extern "C" {
#endif

area_id alloc_mem(void **virt, phys_addr_t *phy, size_t size, uint32 protection,
			const char *name);
area_id map_mem(void **virt, phys_addr_t phy, size_t size, uint32 protection,
			const char *name);

status_t sg_memcpy(const physical_entry *sgTable, int sgCount, const void *data, size_t dataSize);
status_t sg_memcpy_from(void *data, size_t dataSize,
			const physical_entry *sgTable, int sgCount);

void swap_words(void *data, size_t size);

#if defined(__riscv)
// Only use for dedicated DMA allocations, with CPU/device ownership serialized.
void ahci_dma_sync(phys_addr_t physical, size_t size, bool publish);
#endif

#ifdef __cplusplus
}
#endif

#endif
