/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef __NVME_ARCH_H__
#define __NVME_ARCH_H__


#include <OS.h>
#include <arch_vm.h>
#include <arch_atomic.h>


#define NVME_ARCH_64 __HAIKU_ARCH_64_BIT
#define NVME_MMIO_64BIT NVME_ARCH_64
#define PAGE_SIZE B_PAGE_SIZE


#ifndef asm
#define asm __asm__
#endif


#define nvme_wmb() memory_write_barrier()
#if defined(__riscv)
// NVMe completions and payloads both live in ordinary non-cacheable RAM on
// SG2042. The RISC-V memory_read_barrier() orders device-input reads, not a
// completion-queue RAM load followed by a payload RAM load, so use a full
// I/O-and-memory fence when consuming DMA completed by the controller.
#define nvme_dma_rmb() memory_full_barrier()
#else
#define nvme_dma_rmb() memory_read_barrier()
#endif


typedef uint8 __u8;
typedef uint32 __u32;
typedef uint64 __u64;


static inline __u32
nvme_mmio_read_4(const volatile __u32 *addr)
{
	return *addr;
}


static inline void
nvme_mmio_write_4(volatile __u32 *addr, __u32 val)
{
	*addr = val;
}


static inline __u64
nvme_mmio_read_8(volatile __u64 *addr)
{
#ifdef NVME_MMIO_64BIT
	return *addr;
#else
	volatile __u32 *addr32 = (volatile __u32 *)addr;
	__u64 val;

	/*
	 * Read lower 4 bytes before upper 4 bytes.
	 * This particular order is required by I/OAT.
	 * If the other order is required, use a pair of
	 * _nvme_mmio_read_4() calls.
	 */
	val = addr32[0];
	val |= (__u64)addr32[1] << 32;

	return val;
#endif
}


static inline void
nvme_mmio_write_8(volatile __u64 *addr, __u64 val)
{

#ifdef NVME_MMIO_64BIT
	*addr = val;
#else
	volatile __u32 *addr32 = (volatile __u32 *)addr;

	addr32[0] = (__u32)val;
	addr32[1] = (__u32)(val >> 32);
#endif
}

#endif /* __NVME_ARCH_H__ */
