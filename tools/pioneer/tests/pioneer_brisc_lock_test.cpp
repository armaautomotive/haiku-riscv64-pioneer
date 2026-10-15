/* SPDX-License-Identifier: MIT */
#include <cassert>
using status_t = int;
static constexpr status_t B_OK = 0, B_DEV_NOT_READY = -5;
static int sLock = 123, acquired = 0, released = 0, acquisitionStatus = B_OK;
static bool sColdRecoveryRequired;
static status_t acquire_sem(int sem) { assert(sem == sLock); acquired++; return acquisitionStatus; }
static void release_sem(int sem) { assert(sem == sLock); released++; }
#include "pioneer_lock.inc"
int main()
{
	assert(AcquireProbeLock() == B_OK && acquired == 1 && released == 0);
	sColdRecoveryRequired = true;
	assert(AcquireProbeLock() == B_DEV_NOT_READY && acquired == 2 && released == 1);
	acquisitionStatus = -10;
	assert(AcquireProbeLock() == -10 && acquired == 3 && released == 1);
}
