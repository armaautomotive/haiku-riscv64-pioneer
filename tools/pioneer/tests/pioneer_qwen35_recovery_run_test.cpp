/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026112100010121)
#ifdef TEST_LIVE_TAKEOVER
#ifndef TEST_GRAPH_RECOVERY
#error Live takeover recovery test requires the shared-role and graph owners
#endif
#define PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS 1
#endif
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
#include <algorithm>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadState.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using status_t=int;
constexpr int B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_BUSY=-3,B_DEV_NOT_READY=-4,B_NO_MEMORY=-5;
static unsigned failStage=0,abortCount=0,completeCount=0,contextDeleted=0,cookieDeleted=0;
static bool failPayloadRead=false,failPayloadRestore=false,failFirmwareQueue=false,failDbi=false;
static bool ticketPending=false,invalidTicket=false,guardV2Missing=false;
static unsigned validateCalls=0,invalidateOnValidate=0;
static bool resetTimeout=false,resumeResetTimeout=false;
static unsigned resetResumeWaits=0;
#ifdef TEST_GRAPH_RECOVERY
static bool failGraphRelease=false;
static bool failSharedRoleRelease=false;
static bool graphReleased=false;
static unsigned graphReleases=0,sharedRoleReleases=0;
struct FakeGraphOwner { bool retained=false; };
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
struct pioneer_driver_recovery_guard_v2_info {
	module_info info;uint32 version=2;
	status_t (*begin)(uint64,uint64*);status_t (*complete)(uint64,const PioneerDriverRecoveryProof*,uint64*);
	status_t (*abort)(uint64);status_t (*validate)(uint64);
};
static status_t Begin(uint64 g,uint64* t) {
	if (g!=3 || ticketPending)return B_BAD_VALUE;
	ticketPending=true;*t=9;return B_OK;
}
static status_t Complete(uint64 t,const PioneerDriverRecoveryProof* p,uint64* g) {
	completeCount++;assert(t==9 && ticketPending && !invalidTicket && p->Complete()
		&& p->reset_generation==t && p->firmware_generation==t);
	if (!Step(PIONEER_RECOVERY_STAGE_GUARD_COMPLETE)) return B_DEV_NOT_READY;
	ticketPending=false;*g=4;return B_OK;
}
static status_t Abort(uint64 t) { assert(t==9);abortCount++;ticketPending=false;return B_OK; }
static status_t Validate(uint64 t) {
	if (++validateCalls==invalidateOnValidate) invalidTicket=true;
	return t==9 && ticketPending && !invalidTicket?B_OK:B_DEV_NOT_READY;
}
static pioneer_driver_recovery_guard_info recoveryGuard{{},1,Begin,Complete,Abort};
static pioneer_driver_recovery_guard_v2_info recoveryGuardV2{{},2,Begin,Complete,Abort,Validate};
#define PIONEER_DRIVER_RECOVERY_GUARD_MODULE "fake/guard"
#define PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE "fake/guard/v2"
static status_t get_module(const char* name,module_info** out) {
	if (guardV2Missing && !strcmp(name,PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE)) return B_DEV_NOT_READY;
	*out=!strcmp(name,PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE)?&recoveryGuardV2.info:&recoveryGuard.info;
	return B_OK;
}
static void put_module(const char*) {}
struct ColdLatch { bool value=true;operator bool()const{return value;}
	ColdLatch& operator=(bool v){value=v;return *this;}
	void ClearAfterVerifiedRecovery(){value=false;} };
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
enum : uint32 { BLACKHOLE_ASIC_RESET_COMPLETE=5,BLACKHOLE_ASIC_RESET_TIMEOUT=6 };
struct BlackholeAsicResetResult {
	uint32 stage=0,polls=0,command_before=0,marker_after=0,trigger_target=0,trigger_control=0;
};
struct FakeProfile {};
struct PioneerSessionIO {
	bool postResetDisposeComplete=false;
#ifdef TEST_LIVE_TAKEOVER
	void* qwen35BorrowedRoleScope=nullptr;
#endif
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
	r.stage=7;
	if (!Step(PIONEER_RECOVERY_STAGE_ASIC_RESET)) return false;
	r.trigger_target=1;r.trigger_control=0x11;
	r.stage=resetTimeout?BLACKHOLE_ASIC_RESET_TIMEOUT:BLACKHOLE_ASIC_RESET_COMPLETE;
	return !resetTimeout;
}
[[maybe_unused]] static bool BlackholeAsicResetResumable(const BlackholeAsicResetResult& r) {
	return (r.stage==BLACKHOLE_ASIC_RESET_TIMEOUT || r.stage==BLACKHOLE_ASIC_RESET_COMPLETE)
		&& r.trigger_target==1 && r.trigger_control==0x11;
}
template<class IO> static bool BlackholeAsicResetResumeWait(IO&,unsigned,BlackholeAsicResetResult& r) {
	++resetResumeWaits;
	if (!BlackholeAsicResetResumable(r)) return false;
	if (r.stage==BLACKHOLE_ASIC_RESET_COMPLETE) return true;
	if (resumeResetTimeout) return false;
	r.stage=BLACKHOLE_ASIC_RESET_COMPLETE;return true;
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
#ifdef TEST_LIVE_TAKEOVER
struct FakeDataflowService { bool ready=true,faulted=true;bool StoppedResetReady()const{return ready;}
	bool Faulted()const{return faulted;} };
#endif
struct PioneerStoreContext {
	PioneerPostResetMapping* recoveryMapping=nullptr;bool recoveryAttempted=false,dispatchOwnerRetained=true;
	PioneerDriverRecoveryProof recoveryFinishProof{};uint64 recoveryFinishTicket=0;bool recoveryFinishPending=false;
	PioneerPciePayloadSnapshot recoveryPayload{};bool recoveryFinishNeedsAudit=false;
	BlackholeAsicResetResult recoveryResetResult{};uint64 recoveryResetTicket=0;bool recoveryResetPending=false;
#ifdef TEST_GRAPH_RECOVERY
	FakeGraphOwner graphOwner;FakeGraphOwner* qwen35GraphOwner=&graphOwner;
	FakeSharedRoleOwner sharedRoleOwner;FakeSharedRoleOwner* qwen35SharedRoleBinding=&sharedRoleOwner;
#endif
	PioneerRecoveryDbiSnapshot recoveryDbi;
#ifdef TEST_LIVE_TAKEOVER
	bool bankStorageOnly=true,qwen35LiveFaultRecoveryRequested=false;
	FakeDataflowService service;FakeDataflowService* qwen35DataflowService=&service;
#endif
	struct {bool config_attempted=true,map_attempted=true;} transaction;
	FakeProfile profile;FakePciResult pci;PioneerSessionIO io;
	FakePciResult& Pci(){return pci;}
	~PioneerStoreContext(){contextDeleted++;delete recoveryMapping;}
};
#ifdef TEST_GRAPH_RECOVERY
static bool PioneerQ35ReleaseSharedRoleAfterReset(FakeSharedRoleOwner*& owner,
	const PioneerDriverRecoveryProof& proof,uint64 ticket) {
	if (!owner) return true;
	assert(owner && proof.ReadyToRetireOneOwner() && proof.reset_generation==ticket
		&& proof.firmware_generation==ticket && ticket==9);
	assert(!trace.empty() && trace.back()==PIONEER_RECOVERY_STAGE_OWNER_RELEASE);
	sharedRoleReleases++;
	if (failSharedRoleRelease) return false;
	owner=nullptr;return true;
}
static bool PioneerQ35GraphOwnerReleaseAfterReset(FakeGraphOwner* owner,
	const PioneerDriverRecoveryProof& proof,uint64 ticket) {
	if (graphReleased) return true;
	assert(owner && proof.ReadyToRetireOneOwner() && proof.reset_generation==ticket
		&& proof.firmware_generation==ticket && ticket==9);
	graphReleases++;if (failGraphRelease) return false;
	graphReleased=true;return true;
}
#endif
struct PioneerProbeCookie {uint32 magic=0x50313030;int team=17;PioneerStoreContext* context=nullptr;bool finalFreeRetained=false;
	~PioneerProbeCookie(){cookieDeleted++;}};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
#ifdef TEST_LIVE_TAKEOVER
static PioneerProbeCookie* sQwen35ExclusiveCookie=nullptr;
static bool PioneerQ35GraphOwnerRetained(const FakeGraphOwner* owner) {
	return owner && owner->retained;
}
struct tt_resident_session_result {};
static unsigned retainCalls=0;
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result&) {
	++retainCalls;c.dispatchOwnerRetained=true;sColdRecoveryRequired=true;
}
#endif
struct FakeCard {int bus=0,device=0,function=0;} sCard;[[maybe_unused]] static int sCookie;
#define TT_PCI_PROBE_DEVICE "fake"
struct FakePci {status_t unreserve_device(int,int,int,const char*,void*) {
	return Step(PIONEER_RECOVERY_STAGE_UNRESERVE)?B_OK:B_DEV_NOT_READY;
}} pci;[[maybe_unused]] static FakePci* sPCI=&pci;
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
	ticketPending=invalidTicket=guardV2Missing=false;
	validateCalls=invalidateOnValidate=0;
	resetTimeout=resumeResetTimeout=false;resetResumeWaits=0;
#ifdef TEST_GRAPH_RECOVERY
	failGraphRelease=failSharedRoleRelease=graphReleased=false;graphReleases=sharedRoleReleases=0;
#endif
	sColdRecoveryRequired.value=true;sReloadQuiesced=false;sBankStorageOnly=true;sReloadGeneration=3;sOpenCookies=1;
	#ifdef TEST_LIVE_TAKEOVER
	sQwen35ExclusiveCookie=nullptr;
	retainCalls=0;
	#endif
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
	Setup();guardV2Missing=true;auto unavailable=Request();
	assert(Qwen35RecoveryRun(&inspector,&unavailable,sizeof(unavailable))==B_OK
		&& unavailable.operation_status==B_DEV_NOT_READY && trace.empty() && sRuntimeOwner
		&& !sRuntimeOwner->context->recoveryAttempted && sColdRecoveryRequired
		&& !ticketPending && !abortCount && !completeCount);
	Cleanup();
	Setup();resetTimeout=true;auto r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_ASIC_RESET && r.detail==BLACKHOLE_ASIC_RESET_TIMEOUT
		&& sRuntimeOwner && sRuntimeOwner->context->recoveryResetPending
		&& sRuntimeOwner->context->recoveryMapping && ticketPending && !abortCount && sColdRecoveryRequired);
	resumeResetTimeout=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_ASIC_RESET && resetResumeWaits==1
		&& sRuntimeOwner && ticketPending && !abortCount);
	resumeResetTimeout=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && resetResumeWaits==2
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	Setup();resetTimeout=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY);
	invalidTicket=true;const auto resetTrace=trace;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& trace==resetTrace && !resetResumeWaits && sRuntimeOwner && sColdRecoveryRequired && ticketPending);
	Cleanup();
	Setup();r=Request();assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK
		&& r.operation_status==B_OK && r.stage==PIONEER_RECOVERY_STAGE_COMPLETE && r.new_generation==4
		&& !sColdRecoveryRequired && !sRuntimeOwner && contextDeleted==1 && cookieDeleted==1 && !abortCount && completeCount==1);
#ifdef TEST_GRAPH_RECOVERY
	assert(graphReleases==1 && sharedRoleReleases==1);
	Setup();failSharedRoleRelease=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_OWNER_RELEASE && sharedRoleReleases==1 && !graphReleases
		&& sRuntimeOwner && sRuntimeOwner->context->qwen35SharedRoleBinding
		&& sColdRecoveryRequired && !abortCount && !completeCount && ticketPending);
	failSharedRoleRelease=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && completeCount==1 && !abortCount
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_FLEET)==2
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_HOST_DMA)==2);
	Setup();failGraphRelease=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_OWNER_RELEASE && graphReleases==1 && sharedRoleReleases==1
		&& sRuntimeOwner && sColdRecoveryRequired && !abortCount && !completeCount && ticketPending);
	failGraphRelease=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && graphReleases==2 && sharedRoleReleases==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_FLEET)==2);
	Setup();failSharedRoleRelease=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY);
	failSharedRoleRelease=false;failStage=PIONEER_RECOVERY_STAGE_FLEET;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_FLEET && sRuntimeOwner && sColdRecoveryRequired
		&& sRuntimeOwner->context->recoveryMapping && !sRuntimeOwner->context->recoveryFinishPending
		&& !ticketPending && abortCount==1 && !completeCount
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1
		&& !std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_UNRESERVE));
	Cleanup();
	Setup();failSharedRoleRelease=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY);
	invalidTicket=true;failSharedRoleRelease=false;const auto traceBefore=trace;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& trace==traceBefore && sRuntimeOwner && sColdRecoveryRequired && ticketPending
		&& sharedRoleReleases==1 && !graphReleases && !completeCount);
	Cleanup();
	Setup();invalidateOnValidate=3;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& sRuntimeOwner && sColdRecoveryRequired && ticketPending && sharedRoleReleases==1
		&& !sRuntimeOwner->context->qwen35SharedRoleBinding && !graphReleases && !completeCount);
	const auto faultTrace=trace;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& trace==faultTrace && sRuntimeOwner && ticketPending && !graphReleases);
	Cleanup();
	Setup();failSharedRoleRelease=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY);
	sRuntimeOwner->context->recoveryFinishProof.firmware_generation=10;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_BAD_VALUE
		&& sRuntimeOwner && sColdRecoveryRequired && !ticketPending && abortCount==1
		&& sharedRoleReleases==1 && !graphReleases && !completeCount);
	Cleanup();
#endif
	for(unsigned stage=1;stage<=12;stage++) {
		Setup();failStage=stage;r=Request();
		assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status!=B_OK && r.stage==stage);
		assert(sColdRecoveryRequired && sReloadGeneration==3 && abortCount==1);
		if(stage<12) assert(sRuntimeOwner && !contextDeleted && !cookieDeleted && !completeCount);
		else assert(sRuntimeOwner && !contextDeleted && !cookieDeleted && completeCount==1);
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
	Cleanup();
#ifdef TEST_LIVE_TAKEOVER
	Setup();auto* fault=sRuntimeOwner;fault->finalFreeRetained=false;sQwen35ExclusiveCookie=fault;
	fault->context->service.ready=false;fault->context->dispatchOwnerRetained=false;
	sColdRecoveryRequired=false;r=Request();r.expected_generation=2;
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_BAD_VALUE && !retainCalls);
	r=Request();r.expected_tag++;
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_BAD_VALUE && !retainCalls);
	activeTeam=18;r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_NOT_ALLOWED && !retainCalls);
	activeTeam=17;
	sOpenCookies=2;r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && !retainCalls);
	sOpenCookies=1;fault->context->service.faulted=false;r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && !retainCalls);
	fault->context->service.faulted=true;fault->context->io.qwen35BorrowedRoleScope=reinterpret_cast<void*>(1);r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && !retainCalls);
	fault->context->io.qwen35BorrowedRoleScope=nullptr;sQwen35ExclusiveCookie=nullptr;r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && !retainCalls);
	sQwen35ExclusiveCookie=fault;resetTimeout=true;r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& retainCalls==1 && fault->context->qwen35LiveFaultRecoveryRequested
		&& fault->context->recoveryResetPending && ticketPending && sColdRecoveryRequired);
	resetTimeout=false;r=Request();
	assert(Qwen35RecoveryRun(fault,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !fault->context && !sColdRecoveryRequired && retainCalls==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	sQwen35ExclusiveCookie=nullptr;delete fault;
	// A retained graph is a fault even if its service has not changed phase.
	Setup();fault=sRuntimeOwner;fault->finalFreeRetained=false;sQwen35ExclusiveCookie=fault;
	fault->context->service.ready=true;fault->context->service.faulted=false;
	fault->context->graphOwner.retained=true;fault->context->dispatchOwnerRetained=false;
	sColdRecoveryRequired=false;r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& retainCalls==1 && !sRuntimeOwner && !fault->context && !sColdRecoveryRequired);
	sQwen35ExclusiveCookie=nullptr;delete fault;
	// The cold latch blocks ordinary close even if service setup did not
	// complete. A valid exclusive owner can still request the guarded reset.
	Setup();fault=sRuntimeOwner;fault->finalFreeRetained=false;sQwen35ExclusiveCookie=fault;
	fault->context->qwen35DataflowService=nullptr;fault->context->dispatchOwnerRetained=false;
	r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& retainCalls==1 && !sRuntimeOwner && !fault->context && !sColdRecoveryRequired);
	sQwen35ExclusiveCookie=nullptr;delete fault;
	Setup();fault=sRuntimeOwner;fault->finalFreeRetained=false;sQwen35ExclusiveCookie=fault;
	fault->context->service.ready=false;fault->context->dispatchOwnerRetained=false;
	sColdRecoveryRequired=false;failSharedRoleRelease=true;r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& retainCalls==1 && sRuntimeOwner==fault && ticketPending);
	failSharedRoleRelease=false;r=Request();
	assert(Qwen35RecoveryRun(fault,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !fault->context && retainCalls==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	sQwen35ExclusiveCookie=nullptr;delete fault;
	Setup();auto* live=sRuntimeOwner;live->finalFreeRetained=false;sQwen35ExclusiveCookie=live;
	live->context->io.qwen35BorrowedRoleScope=reinterpret_cast<void*>(1);r=Request();
	assert(Qwen35RecoveryRun(live,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && trace.empty());
	live->context->io.qwen35BorrowedRoleScope=nullptr;live->context->service.ready=false;r=Request();
	assert(Qwen35RecoveryRun(live,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && trace.empty());
	live->context->service.ready=true;sQwen35ExclusiveCookie=nullptr;r=Request();
	assert(Qwen35RecoveryRun(live,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && trace.empty());
	sQwen35ExclusiveCookie=live;r=Request();
	assert(Qwen35RecoveryRun(live,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& r.stage==PIONEER_RECOVERY_STAGE_COMPLETE && !sRuntimeOwner && !sColdRecoveryRequired
		&& contextDeleted==1 && !cookieDeleted && live->magic==0x50313030 && !live->context
		&& sQwen35ExclusiveCookie==live && sOpenCookies==1);
	sQwen35ExclusiveCookie=nullptr;delete live;
	Setup();live=sRuntimeOwner;live->finalFreeRetained=false;sQwen35ExclusiveCookie=live;
	failSharedRoleRelease=true;r=Request();
	assert(Qwen35RecoveryRun(live,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& sRuntimeOwner==live && live->context && !cookieDeleted && ticketPending && sColdRecoveryRequired);
	failSharedRoleRelease=false;r=Request();
	assert(Qwen35RecoveryRun(live,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !live->context && !cookieDeleted && !sColdRecoveryRequired
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	sQwen35ExclusiveCookie=nullptr;delete live;
	Setup();live=sRuntimeOwner;live->finalFreeRetained=false;sQwen35ExclusiveCookie=live;resetTimeout=true;r=Request();
	assert(Qwen35RecoveryRun(live,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& live->context->recoveryResetPending && ticketPending);
	// If the live fd is closed mid-wait, a separate inspector may resume the
	// same reset ticket and retire the retained cookie without a second reset.
	live->finalFreeRetained=true;sQwen35ExclusiveCookie=nullptr;resetTimeout=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && cookieDeleted==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
#endif
	puts("PASS actual recovery operation: ordered success, every injected stage failure stays cold, owner/FD/generation admission; SIMULATED HARDWARE ONLY");
#endif
}
