/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026112100010121)
#ifdef TEST_GRAPH_RECOVERY
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#endif
#ifndef TEST_RECOVERY_DISABLED
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadState.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using status_t=int;
constexpr int B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_BUSY=-3,B_DEV_NOT_READY=-4,B_NO_MEMORY=-5;
static unsigned failStage=0,abortCount=0,completeCount=0,contextDeleted=0,cookieDeleted=0;
static bool failPayloadRead=false,failPayloadRestore=false,failFirmwareQueue=false,failDbi=false;
#ifdef TEST_GRAPH_RECOVERY
static bool failGraphRelease=false;
static bool failSharedRoleRelease=false;
static unsigned graphReleases=0,sharedRoleReleases=0;
struct FakeGraphOwner {};
struct FakeSharedRoleOwner {};
#endif
static std::vector<unsigned> trace;
static bool Step(unsigned stage) { trace.push_back(stage);return failStage!=stage; }
struct module_info {};
struct pioneer_driver_recovery_guard_info {
	module_info info;uint32 version=1;
	status_t (*begin)(uint64,uint64*);status_t (*complete)(uint64,const PioneerDriverRecoveryProof*,uint64*);
	status_t (*abort)(uint64);
};
static status_t Begin(uint64 g,uint64* t) { if (g!=3)return B_BAD_VALUE;*t=9;return B_OK; }
static status_t Complete(uint64 t,const PioneerDriverRecoveryProof* p,uint64* g) {
	completeCount++;assert(t==9 && p->Complete());if (!Step(PIONEER_RECOVERY_STAGE_GUARD_COMPLETE)) return B_DEV_NOT_READY;
	*g=4;return B_OK;
}
static status_t Abort(uint64 t) { assert(t==9);abortCount++;return B_OK; }
static pioneer_driver_recovery_guard_info recoveryGuard{{},1,Begin,Complete,Abort};
#define PIONEER_DRIVER_RECOVERY_GUARD_MODULE "fake/guard"
static status_t get_module(const char*,module_info** out) { *out=&recoveryGuard.info;return B_OK; }
static void put_module(const char*) {}
struct ColdLatch { bool value=true;operator bool()const{return value;}void ClearAfterVerifiedRecovery(){value=false;} };
static ColdLatch sColdRecoveryRequired;
static bool sReloadQuiesced=false,sBankStorageOnly=true;
static int guard=1;static int* sReloadGuard=&guard;static uint64 sReloadGeneration=3;static uint32 sOpenCookies=1;
struct tt_arc_scratch_result {uint32 state=0;};
struct tt_arc_queue_result {uint32 state=0;};
struct PioneerPostResetAuditResult {uint32 failure=0,noc=0,core=0;};
struct PioneerPostResetPciResult {uint32 stage=0;};
struct PioneerPciePayloadSnapshot {uint32 capability=0x70,version=2,supported=2,control=0x20;bool valid=true;};
constexpr uint32 PIONEER_RECOVERY_DBI_MPS_MASK=0xe0;
struct PioneerRecoveryDbiSnapshot {uint32 mps=0x20;bool valid=true;};
struct BlackholeAsicResetResult {uint32 stage=0;};
struct FakeProfile {};
struct PioneerSessionIO {
	bool postResetDisposeComplete=false;
	uint32 CardRead(uint16,uint8){return 0;} uint32 RootRead(uint16,uint8){return 0;}
	void CardWrite(uint16,uint8,uint32){}
	status_t DisposeAfterVerifiedReset(const PioneerDriverRecoveryProof& p) {
		assert(p.ReadyToRetireOneOwner());
		if (!Step(PIONEER_RECOVERY_STAGE_OWNER_RELEASE)) return B_DEV_NOT_READY;
		postResetDisposeComplete=true;return B_OK;
	}
};
static bool PioneerPreResetDisableMaster(PioneerSessionIO&) { return Step(PIONEER_RECOVERY_STAGE_DISABLE_MASTER); }
static bool PioneerPciePayloadRead(PioneerSessionIO&,PioneerPciePayloadSnapshot&) {return !failPayloadRead;}
static bool PioneerPciePayloadRestore(PioneerSessionIO&,const PioneerPciePayloadSnapshot&) {return !failPayloadRestore;}
template<class IO> static bool BlackholeAsicReset(IO&,unsigned,BlackholeAsicResetResult& r) {
	r.stage=7;return Step(PIONEER_RECOVERY_STAGE_ASIC_RESET);
}
static bool PioneerPostResetPciRestore(PioneerSessionIO&,PioneerPostResetPciResult& r) {
	r.stage=4;return Step(PIONEER_RECOVERY_STAGE_PCI_RESTORE);
}
struct FakePciResult {uint32 version=1,commands_before[2]={},bridge_before[3]={0x0001fff1,0,0},bars_before[6]={12,0,12,0,12,0};};
static bool PioneerPostResetPciFinish(PioneerSessionIO&,const FakePciResult&,unsigned& stage) {
	stage=2;return Step(PIONEER_RECOVERY_STAGE_PCI_IDLE);
}
struct PioneerPostResetMapping {
	explicit PioneerPostResetMapping(FakeProfile&) {}
	bool Open(){return Step(PIONEER_RECOVERY_STAGE_MAPPING);}
	bool ProbeFirmware(unsigned,tt_arc_scratch_result& r){r.state=2;return Step(PIONEER_RECOVERY_STAGE_FIRMWARE);}
	bool RestoreInternalDbi(const PioneerRecoveryDbiSnapshot& s,uint32 control) {
		assert(s.valid && s.mps==(control&PIONEER_RECOVERY_DBI_MPS_MASK));return !failDbi;
	}
	bool InitializeFirmwareQueue(const tt_arc_scratch_result&,tt_arc_queue_result& r){r.state=3;return !failFirmwareQueue;}
	bool Audit(PioneerPostResetAuditResult& r){r.failure=2;r.core=7;return Step(PIONEER_RECOVERY_STAGE_FLEET);}
	bool AuditHostDma(){return Step(PIONEER_RECOVERY_STAGE_HOST_DMA);}
	bool Close(){return Step(PIONEER_RECOVERY_STAGE_CLOSE_MAPPING);}
};
struct PioneerStoreContext {
	PioneerPostResetMapping* recoveryMapping=nullptr;bool recoveryAttempted=false,dispatchOwnerRetained=true;
#ifdef TEST_GRAPH_RECOVERY
	FakeGraphOwner graphOwner;FakeGraphOwner* qwen35GraphOwner=&graphOwner;
	FakeSharedRoleOwner sharedRoleOwner;FakeSharedRoleOwner* qwen35SharedRoleBinding=&sharedRoleOwner;
#endif
	PioneerRecoveryDbiSnapshot recoveryDbi;
	struct {bool config_attempted=true,map_attempted=true;} transaction;
	FakeProfile profile;FakePciResult pci;PioneerSessionIO io;
	FakePciResult& Pci(){return pci;}
	~PioneerStoreContext(){contextDeleted++;delete recoveryMapping;}
};
#ifdef TEST_GRAPH_RECOVERY
static bool PioneerQ35ReleaseSharedRoleAfterReset(FakeSharedRoleOwner*& owner,
	const PioneerDriverRecoveryProof& proof,uint64 ticket) {
	assert(owner && proof.ReadyToRetireOneOwner() && proof.reset_generation==ticket
		&& proof.firmware_generation==ticket && ticket==9);
	assert(!trace.empty() && trace.back()==PIONEER_RECOVERY_STAGE_OWNER_RELEASE);
	sharedRoleReleases++;
	if (failSharedRoleRelease) return false;
	owner=nullptr;return true;
}
static bool PioneerQ35GraphOwnerReleaseAfterReset(FakeGraphOwner* owner,
	const PioneerDriverRecoveryProof& proof,uint64 ticket) {
	assert(owner && proof.ReadyToRetireOneOwner() && proof.reset_generation==ticket
		&& proof.firmware_generation==ticket && ticket==9);
	graphReleases++;return !failGraphRelease;
}
#endif
struct PioneerProbeCookie {uint32 magic=0x50313030;int team=17;PioneerStoreContext* context=nullptr;bool finalFreeRetained=false;
	~PioneerProbeCookie(){cookieDeleted++;}};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
struct FakeCard {int bus=0,device=0,function=0;} sCard;static int sCookie;
#define TT_PCI_PROBE_DEVICE "fake"
struct FakePci {status_t unreserve_device(int,int,int,const char*,void*) {
	return Step(PIONEER_RECOVERY_STAGE_UNRESERVE)?B_OK:B_DEV_NOT_READY;
}} pci,*sPCI=&pci;
static int snooze(int){return B_OK;}
struct thread_info {int team;};static int activeTeam=17,sLock=1,held=0;
static int find_thread(void*){return 1;}static int get_thread_info(int,thread_info* t){t->team=activeTeam;return B_OK;}
static status_t acquire_sem(int){assert(!held);held=1;return B_OK;}static void release_sem(int){assert(held);held=0;}
static status_t user_memcpy(void* dst,const void* src,size_t n){memcpy(dst,src,n);return B_OK;}
#include "pioneer_recovery_run.inc"
static tt_pioneer_recovery_run Request(){tt_pioneer_recovery_run r={};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=3;return r;}
static void Setup() {
	failStage=abortCount=completeCount=contextDeleted=cookieDeleted=0;trace.clear();
	failPayloadRead=failPayloadRestore=failFirmwareQueue=failDbi=false;
#ifdef TEST_GRAPH_RECOVERY
	failGraphRelease=failSharedRoleRelease=false;graphReleases=sharedRoleReleases=0;
#endif
	sColdRecoveryRequired.value=true;sReloadQuiesced=false;sBankStorageOnly=true;sReloadGeneration=3;sOpenCookies=1;
	sRuntimeOwner=new PioneerProbeCookie;sRuntimeOwner->context=new PioneerStoreContext;sRuntimeOwner->finalFreeRetained=true;
}
static void Cleanup(){if(sRuntimeOwner){delete sRuntimeOwner->context;delete sRuntimeOwner;sRuntimeOwner=nullptr;}}
int main(){
	PioneerProbeCookie inspector;
#ifdef TEST_RECOVERY_DISABLED
	Setup();auto r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_NOT_ALLOWED
		&& trace.empty() && sRuntimeOwner && sColdRecoveryRequired && !abortCount && !completeCount);
	Cleanup();puts("PASS default recovery gate: no guard, PCI or owner actions; SIMULATED HARDWARE ONLY");
#else
	Setup();auto r=Request();assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK
		&& r.operation_status==B_OK && r.stage==PIONEER_RECOVERY_STAGE_COMPLETE && r.new_generation==4
		&& !sColdRecoveryRequired && !sRuntimeOwner && contextDeleted==1 && cookieDeleted==1 && !abortCount && completeCount==1);
#ifdef TEST_GRAPH_RECOVERY
	assert(graphReleases==1 && sharedRoleReleases==1);
	Setup();failSharedRoleRelease=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_OWNER_RELEASE && sharedRoleReleases==1 && !graphReleases
		&& sRuntimeOwner && sRuntimeOwner->context->qwen35SharedRoleBinding
		&& sColdRecoveryRequired && abortCount==1 && !completeCount);
	Cleanup();
	Setup();failGraphRelease=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_OWNER_RELEASE && graphReleases==1 && sharedRoleReleases==1
		&& sRuntimeOwner && sColdRecoveryRequired && abortCount==1 && !completeCount);
	Cleanup();
#endif
	for(unsigned stage=1;stage<=12;stage++) {
		Setup();failStage=stage;r=Request();
		assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status!=B_OK && r.stage==stage);
		assert(sColdRecoveryRequired && sReloadGeneration==3 && abortCount==1);
		if(stage<12) assert(sRuntimeOwner && !contextDeleted && !cookieDeleted && !completeCount);
		else assert(!sRuntimeOwner && contextDeleted==1 && cookieDeleted==1 && completeCount==1);
		Cleanup();
	}
	Setup();failPayloadRead=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_DISABLE_MASTER && trace.size()==1
		&& sRuntimeOwner && sColdRecoveryRequired && abortCount==1);
	Cleanup();Setup();failPayloadRestore=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_PCI_RESTORE && trace.size()==3
		&& sRuntimeOwner && sColdRecoveryRequired && abortCount==1);
	Cleanup();Setup();failFirmwareQueue=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_FIRMWARE && r.detail==3
		&& sRuntimeOwner && sColdRecoveryRequired && abortCount==1);
	Cleanup();Setup();failDbi=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_FIRMWARE && sRuntimeOwner && sColdRecoveryRequired && abortCount==1);
	Cleanup();Setup();sRuntimeOwner->context->recoveryDbi.valid=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_NOT_ALLOWED
		&& trace.empty() && sRuntimeOwner && sColdRecoveryRequired && !abortCount);
	Cleanup();Setup();sRuntimeOwner->context->recoveryDbi.mps=0x40;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_DISABLE_MASTER && trace.size()==1
		&& sRuntimeOwner && sColdRecoveryRequired && abortCount==1);
	Cleanup();
	Setup();sRuntimeOwner->finalFreeRetained=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && trace.empty());
	Cleanup();Setup();sOpenCookies=2;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && trace.empty());
	Cleanup();Setup();r=Request();r.expected_generation=2;
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_BAD_VALUE && trace.empty());
	Cleanup();puts("PASS actual recovery operation: ordered success, every injected stage failure stays cold, owner/FD/generation admission; SIMULATED HARDWARE ONLY");
#endif
}
