/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define TT_PIONEER_DRIVER_RELOAD 1
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH 1
#define PIONEER_QWEN35_INPLACE_RECOVERY 1
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026112100010121)
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryProtocol.h"
using status_t=int;using uint32=uint32_t;using uint64=uint64_t;
constexpr int B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2;
struct PioneerStoreContext { bool dispatchOwnerRetained=false; };
struct PioneerProbeCookie { uint32 magic=0x50313030;int team=17;PioneerStoreContext* context=nullptr;bool finalFreeRetained=false; };
struct tt_resident_session_result {};
struct thread_info { int team; };
static int sLock=1,held=0,currentTeam=17,guard=1;
static int* sReloadGuard=&guard;
static uint64 sReloadGeneration=3;
static uint32 sOpenCookies=2;
static bool sColdRecoveryRequired=true,sReloadQuiesced=false;
static bool retainOnEnd=true;
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static int find_thread(void*) { return 1; }
static int get_thread_info(int,thread_info* t) { t->team=currentTeam;return B_OK; }
static int acquire_sem(int) { assert(!held);held=1;return B_OK; }
static void release_sem(int) { assert(held);held=0; }
static int user_memcpy(void* dst,const void* src,size_t n) { memcpy(dst,src,n);return B_OK; }
static void EndResidentSession(PioneerProbeCookie* c,tt_resident_session_result&,bool) {
	assert(held);
	if (retainOnEnd) c->context->dispatchOwnerRetained=true;
	else { c->context=nullptr;sRuntimeOwner=nullptr; }
}
#include "pioneer_cookie_release.inc"
#include "pioneer_probe_free.inc"
#include "pioneer_recovery_status.inc"
static tt_pioneer_recovery_status Request() {
	tt_pioneer_recovery_status r={};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;
	r.expected_generation=sReloadGeneration;return r;
}
int main() {
	PioneerStoreContext context;auto* owner=new PioneerProbeCookie;
	owner->context=&context;sRuntimeOwner=owner;
	PioneerProbeCookie inspector;
	auto r=Request();assert(Qwen35RecoveryStatus(&inspector,&r,sizeof(r))==B_OK
		&& r.cold && r.owner_present && !r.owner_final_free && !r.owner_retirement_eligible);
	assert(ProbeFree(owner)==B_OK && !held && sOpenCookies==1 && owner->finalFreeRetained
		&& owner->context==&context && context.dispatchOwnerRetained);
	r=Request();assert(Qwen35RecoveryStatus(&inspector,&r,sizeof(r))==B_OK
		&& r.owner_final_free && r.owner_retirement_eligible && r.open_count==1);
	sOpenCookies=2;r=Request();assert(Qwen35RecoveryStatus(&inspector,&r,sizeof(r))==B_OK
		&& !r.owner_retirement_eligible);sOpenCookies=1;
	r=Request();r.expected_generation++;assert(Qwen35RecoveryStatus(&inspector,&r,sizeof(r))==B_BAD_VALUE);
	r=Request();r.reserved[0]=1;assert(Qwen35RecoveryStatus(&inspector,&r,sizeof(r))==B_BAD_VALUE);
	r=Request();currentTeam++;assert(Qwen35RecoveryStatus(&inspector,&r,sizeof(r))==B_NOT_ALLOWED);
	currentTeam--;sReloadQuiesced=true;r=Request();assert(Qwen35RecoveryStatus(&inspector,&r,sizeof(r))==B_OK
		&& !r.owner_retirement_eligible);sReloadQuiesced=false;
	delete owner;
	retainOnEnd=false;sOpenCookies=1;auto* clean=new PioneerProbeCookie;
	clean->context=&context;sRuntimeOwner=clean;
	assert(ProbeFree(clean)==B_OK && !held && !sOpenCookies && !sRuntimeOwner);
	puts("PASS actual final-free and read-only recovery owner status: live owner, retained owner, clean close, extra FD, stale generation, team and quiesce; NO PCI/MMIO");
}
