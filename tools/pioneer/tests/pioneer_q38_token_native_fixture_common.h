/* SPDX-License-Identifier: MIT */
// Shared TEST OS/clock/unused-foreign-owner metadata,NOT a native grant.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeState.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include <memory>
#include <type_traits>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
using int64_t=std::int64_t;using status_t=int;using thread_id=int;
static constexpr int B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_DEV_NOT_READY=-3,B_BUSY=-4;
static constexpr unsigned TT_PIONEER_MMIO_RESTORE_FAILED=5; // simulated PCI-result endpoint enum
struct thread_info{int team=-1;};
static int currentThread=17,currentTeam=23,lockHeld=0,lockAcquires=0,lockReleases=0;
static bool failThreadInfo=false;static status_t lockStatus=B_OK;
static thread_id find_thread(const char*){return currentThread;}
static status_t get_thread_info(thread_id thread,thread_info* out){
 if(failThreadInfo || thread!=currentThread)return B_BAD_VALUE;
 out->team=currentTeam;return B_OK;
}
static int sLock=1;
static status_t acquire_sem(int lock){
 assert(lock==sLock);if(lockStatus!=B_OK)return lockStatus;if(lockHeld)return B_BUSY;
 lockHeld=1;lockAcquires++;return B_OK;
} // ONLY the OS semaphore syscall is simulated;the production lock is extracted verbatim
static status_t release_sem(int lock){assert(lock==sLock && lockHeld==1);lockHeld=0;lockReleases++;return B_OK;}
struct PioneerProbeCookie{uint32 magic=0x50313030;PioneerStoreContext* context=nullptr;int team=23;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64 sReloadGeneration=UINT64_C(0x100000007);
struct MockLease {
 bool saved=false,ready=false,pending=false,recovery_required=false,dirty=false,unsafe=false;
 bool running=false,awaiting=false,closed=false;
};
struct MockHost{bool allocated=true,ready=true,pending=false,quarantined=false,released=false,memory_type_set=true;};
struct MockForeignOwner:MockLease {
 bool Unsafe()const{return unsafe;}
 bool Closed()const{return false;}
 void Fault(){unsafe=true;}
 template<class... Args>bool MatchesNativeOwner(const Args&...)const{return false;}
}; // foreign owners NEVER succeed or provide initialization/producer authority
struct MockFleet {MockLease compute,store,noc;MockLease& Compute(){return compute;}MockLease& Store(){return store;}};
struct MockCohort {bool unsafe=false;struct Node{MockLease noc,image,store;MockLease& Store(){return store;}}worker[120];};
struct MockClock {
 struct Value{uint32 state=TT_ARC_CLOCK_BUSY_VERIFIED,busy_requested=1,busy_acknowledged=1,restored=0,busy_mhz=1400;}value;
 bool required=true,quarantined=false;
 bool Unsafe()const{return quarantined;}
 bool RequiresExplicitEnd()const{return required;}
 const Value& Snapshot()const{return value;}
 void Quarantine(){quarantined=true;}
};
