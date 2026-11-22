/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q35_ALIGNED_ALLOCATION_H
#define PIONEER_Q35_ALIGNED_ALLOCATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <new>

// Haiku's kernel does not provide C++17 aligned new/delete. Preserve the
// malloc pointer immediately before the aligned placement-new address.
template<class T,class... Args> T* PioneerQ35NewAligned(Args&&... args)
{
	constexpr size_t alignment=alignof(T)>alignof(void*)?alignof(T):alignof(void*);
	constexpr size_t extra=alignment-1+sizeof(void*);
	static_assert((alignment&(alignment-1))==0,"alignment must be a power of two");
	static_assert(sizeof(T)<=SIZE_MAX-extra,"aligned allocation size must not overflow");
	void* raw=malloc(sizeof(T)+extra);
	if (!raw) return nullptr;
	const uintptr_t start=reinterpret_cast<uintptr_t>(raw)+sizeof(void*);
	const uintptr_t aligned=(start+alignment-1)&~uintptr_t(alignment-1);
	*reinterpret_cast<void**>(aligned-sizeof(void*))=raw;
	return ::new (reinterpret_cast<void*>(aligned)) T(static_cast<Args&&>(args)...);
}

template<class T> void PioneerQ35DeleteAligned(T* object)
{
	if (!object) return;
	void* raw=*reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(object)-sizeof(void*));
	object->~T();free(raw);
}
#endif
