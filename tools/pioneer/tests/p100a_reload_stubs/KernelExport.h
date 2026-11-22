/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_TEST_KERNEL_STUB_H
#define PIONEER_TEST_KERNEL_STUB_H
#include "module.h"
#include <cassert>
static unsigned semCalls=0;
static sem_id create_sem(int count,const char*) { assert(count==1); return 1; }
static status_t acquire_sem(sem_id s) { assert(s>0); semCalls++; return B_OK; }
static status_t release_sem(sem_id s) { assert(s>0); semCalls++; return B_OK; }
static int32 atomic_get(int32* p) { return *p; }
static bool injectAtomicFault=false;
static int32 atomic_test_and_set(int32* p,int32 value,int32 expected) {
	if (injectAtomicFault) { injectAtomicFault=false; *p=(int32)((uint32(*p)&~1U)+2U)|1; }
	int32 before=*p; if (before==expected) *p=value; return before;
}
#endif
