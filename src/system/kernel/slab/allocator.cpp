/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "slab_private.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>

#include <debug.h>
#include <heap.h>
#include <kernel.h> // for ROUNDUP
#include <malloc.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "ObjectCache.h"
#include "MemoryManager.h"


#if !USE_DEBUG_HEAPS_FOR_ALL_OBJECT_CACHES


#if DEBUG_HEAPS
#include "../debug/heaps.h"
#define SLAB_PUBLIC_NAME(NAME) slab_##NAME
#else
#define SLAB_PUBLIC_NAME(NAME) NAME
#endif


//#define TEST_ALL_CACHES_DURING_BOOT

static const size_t kBlockSizes[] = {
	16, 24, 32,
	48, 64, 80, 96, 112, 128,
	160, 192, 224, 256,
	320, 384, 448, 512,
	640, 768, 896, 1024,
	1280, 1536, 1792, 2048,
	2560, 3072, 3584, 4096,
	5120, 6144, 7168, 8192,
	10240, 12288, 14336, 16384,
};

static const size_t kNumBlockSizes = B_COUNT_OF(kBlockSizes);

static object_cache* sBlockCaches[kNumBlockSizes];

static addr_t sBootStrapMemory = 0;
static size_t sBootStrapMemorySize = 0;
static size_t sUsedBootStrapMemory = 0;

#if defined(__riscv)
struct BootstrapAllocationHeader {
	uint64 magic;
	size_t size;
};

static const uint64 kBootstrapAllocationMagic = 0x5256534c41424253ULL;
static const size_t kMaxBootstrapArenas = 64;

struct BootstrapArena {
	addr_t base;
	size_t size;
};

static BootstrapArena sBootstrapArenas[kMaxBootstrapArenas];
static size_t sBootstrapArenaCount = 0;


static bool
is_bootstrap_address(void* address)
{
	addr_t value = (addr_t)address;
	for (size_t i = 0; i < sBootstrapArenaCount; i++) {
		const BootstrapArena& arena = sBootstrapArenas[i];
		if (value >= arena.base + sizeof(BootstrapAllocationHeader)
			&& value < arena.base + arena.size) {
			return true;
		}
	}

	return false;
}


static bool
bootstrap_allocation_size(void* address, size_t& size)
{
	// Normal slab objects can start at a page boundary. Do not inspect the
	// preceding header until we know it belongs to a fully mapped bootstrap
	// arena, otherwise the probe itself can fault on the previous page.
	if (address == NULL || !is_bootstrap_address(address))
		return false;

	BootstrapAllocationHeader* header
		= (BootstrapAllocationHeader*)address - 1;
	if (header->magic != kBootstrapAllocationMagic)
		return false;

	size = header->size;
	return true;
}
#endif


RANGE_MARKER_FUNCTION_BEGIN(slab_allocator)


static int
size_to_index(size_t size)
{
	if (size <= 16)
		return 0;
	if (size <= 32)
		return 1 + (size - 16 - 1) / 8;
	if (size <= 128)
		return 3 + (size - 32 - 1) / 16;
	if (size <= 256)
		return 9 + (size - 128 - 1) / 32;
	if (size <= 512)
		return 13 + (size - 256 - 1) / 64;
	if (size <= 1024)
		return 17 + (size - 512 - 1) / 128;
	if (size <= 2048)
		return 21 + (size - 1024 - 1) / 256;
	if (size <= 4096)
		return 25 + (size - 2048 - 1) / 512;
	if (size <= 8192)
		return 29 + (size - 4096 - 1) / 1024;
	if (size <= 16384)
		return 33 + (size - 8192 - 1) / 2048;

	return -1;
}


static void*
block_alloc(size_t size, size_t alignment, uint32 flags)
{
#if defined(__riscv)
	// The Pioneer bootstrap is deliberately single-hart until the normal
	// allocator and scheduler paths are fully initialized. Avoid re-entering
	// object-cache locks from early VM setup. Cache-line-aligned scheduler
	// objects also reach this path before threads exist, so satisfy their
	// alignment directly from the never-freed bootstrap arena.
	if (gKernelStartup) {
		if (alignment <= kMinObjectAlignment)
			return block_alloc_early(size);

		ASSERT((alignment & (alignment - 1)) == 0);
		void* allocation = block_alloc_early(size + alignment - 1);
		if (allocation == NULL)
			return NULL;
		void* alignedAllocation = (void*)ROUNDUP((addr_t)allocation, alignment);
		BootstrapAllocationHeader* header
			= (BootstrapAllocationHeader*)alignedAllocation - 1;
		header->magic = kBootstrapAllocationMagic;
		header->size = size;
		return alignedAllocation;
	}
#endif

	if (alignment > kMinObjectAlignment) {
		// Make size >= alignment and a power of two. This is sufficient, since
		// all of our object caches with power of two sizes are aligned. We may
		// waste quite a bit of memory, but memalign() is very rarely used
		// in the kernel and always with power of two size == alignment anyway.
		ASSERT((alignment & (alignment - 1)) == 0);
		while (alignment < size)
			alignment <<= 1;
		size = alignment;

		// If we're not using an object cache, make sure that the memory
		// manager knows it has to align the allocation.
		if (size > kBlockSizes[kNumBlockSizes - 1])
			flags |= CACHE_ALIGN_ON_SIZE;
	}

	// allocate from the respective object cache, if any
	int index = size_to_index(size);
	if (index >= 0)
		return object_cache_alloc(sBlockCaches[index], flags);

	// the allocation is too large for our object caches -- ask the memory
	// manager
	void* block;
	if (MemoryManager::AllocateRaw(size, flags, block) != B_OK)
		return NULL;

	return block;
}


void*
block_alloc_early(size_t size)
{
#if !defined(__riscv)
	int index = size_to_index(size);
	if (index >= 0 && sBlockCaches[index] != NULL)
		return object_cache_alloc(sBlockCaches[index], CACHE_DURING_BOOT);
#endif
	// RISC-V reaches this path before object-cache locks are usable. Keep all
	// bootstrap allocations on the raw, single-threaded backing area instead.

	if (size > SLAB_CHUNK_SIZE_LARGE) {
		// This is a sufficiently large allocation -- just ask the memory
		// manager directly.
		void* block;
		if (MemoryManager::AllocateRaw(size, 0, block) != B_OK)
			return NULL;

		return block;
	}

	// A small allocation, but no object cache yet. Use the bootstrap memory.
	// This allocation must never be freed!
#if defined(__riscv)
	size_t neededSize = ROUNDUP(sizeof(BootstrapAllocationHeader) + size,
		sizeof(double));
#else
	size_t neededSize = ROUNDUP(size, sizeof(double));
#endif
	if (neededSize > SLAB_CHUNK_SIZE_LARGE)
		return NULL;

	if (sBootStrapMemorySize - sUsedBootStrapMemory < neededSize) {
		// We need more memory.
		void* block;
		if (MemoryManager::AllocateRaw(SLAB_CHUNK_SIZE_LARGE, 0, block) != B_OK)
			return NULL;
		if (sBootstrapArenaCount >= kMaxBootstrapArenas)
			return NULL;
		sBootstrapArenas[sBootstrapArenaCount++] = {
			(addr_t)block, SLAB_CHUNK_SIZE_LARGE};
		sBootStrapMemory = (addr_t)block;
		sBootStrapMemorySize = SLAB_CHUNK_SIZE_LARGE;
		sUsedBootStrapMemory = 0;
	}

	if (sUsedBootStrapMemory + neededSize > sBootStrapMemorySize)
		return NULL;
#if defined(__riscv)
	BootstrapAllocationHeader* header = (BootstrapAllocationHeader*)
		(sBootStrapMemory + sUsedBootStrapMemory);
	header->magic = kBootstrapAllocationMagic;
	header->size = size;
	void* block = header + 1;
#else
	void* block = (void*)(sBootStrapMemory + sUsedBootStrapMemory);
#endif
	sUsedBootStrapMemory += neededSize;

	return block;
}


static void
block_free(void* block, uint32 flags)
{
	if (block == NULL)
		return;

#if defined(__riscv)
	size_t bootstrapSize;
	if (bootstrap_allocation_size(block, bootstrapSize))
		return;
#endif

	ObjectCache* cache = MemoryManager::FreeRawOrReturnCache(block, flags);
	if (cache != NULL) {
		// a regular small allocation
		ASSERT(cache->object_size >= kBlockSizes[0]);
		ASSERT(cache->object_size <= kBlockSizes[kNumBlockSizes - 1]);
		ASSERT(cache == sBlockCaches[size_to_index(cache->object_size)]);
		object_cache_free(cache, block, flags);
	}
}


#if DEBUG_HEAPS
status_t
slab_heap_init(struct kernel_args*, addr_t, size_t)
#else
status_t
heap_init(struct kernel_args*)
#endif
{
	for (size_t index = 0; index < kNumBlockSizes; index++) {
		char name[32];
		snprintf(name, sizeof(name), "block allocator: %lu",
			kBlockSizes[index]);

		uint32 flags = CACHE_DURING_BOOT;
		size_t size = kBlockSizes[index];

		// align the power of two objects to their size
		size_t alignment = (size & (size - 1)) == 0 ? size : 0;

		// For the larger allocation sizes disable the object depot, so we don't
		// keep lot's of unused objects around.
		if (size > 2048)
			flags |= CACHE_NO_DEPOT;

		// Bind this early-bootstrap call directly: the loader has not yet
		// established the dynamic PLT state used by normal kernel calls.
		typedef object_cache* (*create_cache_func)(const char*, size_t, size_t,
			size_t, size_t, size_t, uint32, void*, object_cache_constructor,
			object_cache_destructor, object_cache_reclaimer);
		create_cache_func createCache;
		asm volatile("lla %0, create_object_cache_etc" : "=r"(createCache));
		sBlockCaches[index] = createCache(name, size, alignment, 0, 0, 0, flags,
			NULL, NULL, NULL, NULL);
		if (sBlockCaches[index] == NULL)
			panic("allocator: failed to init block cache");
	}
	return B_OK;
}


status_t
SLAB_PUBLIC_NAME(heap_init_post_sem)()
{
#ifdef TEST_ALL_CACHES_DURING_BOOT
	for (int index = 0; kBlockSizes[index] != 0; index++) {
		block_free(block_alloc(kBlockSizes[index] - sizeof(boundary_tag)), 0,
			0);
	}
#endif

	return B_OK;
}


// #pragma mark - public API


void *
SLAB_PUBLIC_NAME(memalign_etc)(size_t alignment, size_t size, uint32 flags)
{
	return block_alloc(size, alignment, flags & CACHE_ALLOC_FLAGS);
}


void
SLAB_PUBLIC_NAME(free_etc)(void *address, uint32 flags)
{
	if ((flags & CACHE_DONT_LOCK_KERNEL_SPACE) != 0) {
		deferred_free(address);
		return;
	}

	block_free(address, flags & CACHE_ALLOC_FLAGS);
}


void*
SLAB_PUBLIC_NAME(realloc_etc)(void* address, size_t newSize, uint32 flags)
{
	if (newSize == 0) {
		block_free(address, flags);
		return NULL;
	}

	if (address == NULL)
		return block_alloc(newSize, 0, flags);

	size_t oldSize;
	bool bootstrapAllocation = false;
#if defined(__riscv)
	bootstrapAllocation = bootstrap_allocation_size(address, oldSize);
#endif
	ObjectCache* cache = NULL;
	if (!bootstrapAllocation)
		cache = MemoryManager::GetAllocationInfo(address, oldSize);
	if (!bootstrapAllocation && cache == NULL && oldSize == 0) {
		panic("block_realloc(): allocation %p not known", address);
		return NULL;
	}

	if (oldSize == newSize)
		return address;

	void* newBlock = block_alloc(newSize, 0, flags);
	if (newBlock == NULL)
		return NULL;

	memcpy(newBlock, address, std::min(oldSize, newSize));

	if (!bootstrapAllocation)
		block_free(address, flags);

	return newBlock;
}


#if DEBUG_HEAPS


kernel_heap_implementation kernel_slab_heap = {
	"slab_heap",
	0, 0,

	slab_heap_init,
	NULL,
	slab_heap_init_post_sem,
	NULL,

	slab_memalign_etc,
	slab_realloc_etc,
	slab_free_etc,
};


#else


void*
malloc(size_t size)
{
	return block_alloc(size, 0, 0);
}


void
free(void* address)
{
	block_free(address, 0);
}


void*
realloc(void* address, size_t newSize)
{
	return realloc_etc(address, newSize, 0);
}


void*
memalign(size_t alignment, size_t size)
{
	return block_alloc(size, alignment, 0);
}


int
posix_memalign(void** _pointer, size_t alignment, size_t size)
{
	if ((alignment & (sizeof(void*) - 1)) != 0 || _pointer == NULL)
		return B_BAD_VALUE;
	*_pointer = block_alloc(size, alignment, 0);
	return 0;
}


status_t
heap_init_post_thread()
{
	return B_OK;
}


#endif


RANGE_MARKER_FUNCTION_END(slab_allocator)


#else	// USE_DEBUG_HEAPS_FOR_ALL_OBJECT_CACHES


void*
block_alloc_early(size_t size)
{
	panic("block allocator not enabled!");
	return NULL;
}


#endif	// USE_DEBUG_HEAPS_FOR_ALL_OBJECT_CACHES
