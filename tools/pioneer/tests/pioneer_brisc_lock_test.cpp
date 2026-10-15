/* SPDX-License-Identifier: MIT */
#include <cassert>
using status_t = int;
static constexpr status_t B_OK = 0, B_DEV_NOT_READY = -5, B_BUSY=-6;
static int sLock = 123, acquired = 0, released = 0, acquisitionStatus = B_OK;
static bool sColdRecoveryRequired;
static const void* sRuntimeOwner=nullptr;
static status_t acquire_sem(int sem) { assert(sem == sLock); acquired++; return acquisitionStatus; }
static void release_sem(int sem) { assert(sem == sLock); released++; }
#include "pioneer_lock.inc"
int main()
{
	assert(AcquireProbeLock() == B_OK && acquired == 1 && released == 0);
	int owner=0,foreign=0; sRuntimeOwner=&owner;
	assert(AcquireProbeLock()==B_BUSY && acquired==2 && released==1);
	assert(AcquireProbeLock(&foreign)==B_BUSY && acquired==3 && released==2);
	assert(AcquireProbeLock(&owner)==B_OK && acquired==4 && released==2);
	sColdRecoveryRequired = true;
	assert(AcquireProbeLock() == B_DEV_NOT_READY && acquired == 5 && released == 3);
	assert(AcquireProbeLock(&owner)==B_DEV_NOT_READY && acquired==6 && released==4);
	acquisitionStatus = -10;
	assert(AcquireProbeLock() == -10 && acquired == 7 && released == 4);
}
