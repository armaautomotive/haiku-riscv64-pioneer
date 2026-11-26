/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026112100010121)
#ifdef TEST_RETAINED_GATEWAY
#define PIONEER_QWEN35_RETAINED_GATEWAY_WITNESS 1
#define PIONEER_QWEN35_RETAINED_FINISH_TEST_ONLY 1
#endif
#ifdef TEST_LINK_RESET
#define PIONEER_QWEN35_LINK_RESET_WITNESS 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPcieLinkReset.h"
#endif
#ifdef TEST_FIRMWARE_RESET
#define PIONEER_QWEN35_FIRMWARE_RESET_WITNESS 1
#endif
#ifdef TEST_LIVE_TAKEOVER
#ifndef TEST_GRAPH_RECOVERY
#error Live takeover recovery test requires the shared-role and graph owners
#endif
#define PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS 1
#endif
#ifdef TEST_RECOVERY_QUALIFY
#ifndef TEST_LIVE_TAKEOVER
#error Recovery qualification test requires the live takeover owner
#endif
#define PIONEER_QWEN35_RECOVERY_QUALIFY 1
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
#include "../../../src/add-ons/kernel/drivers/misc/PioneerFirmwareResetTrace.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPcieRootAerSnapshot.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using status_t=int;
#ifdef TEST_LINK_RESET
using bigtime_t=int64_t;
static unsigned linkWrites=0;static bool failLinkAssert=false,failLinkRestore=false,fakeLinkReady=true;
#endif
constexpr int B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_BUSY=-3,B_DEV_NOT_READY=-4,B_NO_MEMORY=-5;
static unsigned failStage=0,abortCount=0,completeCount=0,contextDeleted=0,cookieDeleted=0;
static bool failPayloadRead=false,failPayloadRestore=false,failFirmwareQueue=false,failDbi=false;
static unsigned payloadReads=0,payloadDriftAt=0;
static bool firmwareNotReady=false;
static bool retryableMappingFailure=false;
static bool ticketPending=false,invalidTicket=false,guardV2Missing=false;
static unsigned validateCalls=0,invalidateOnValidate=0;
static bool resetTimeout=false,resumeResetTimeout=false;
static unsigned resetResumeWaits=0;
#ifdef TEST_FIRMWARE_RESET
static unsigned mappingCreates=0,preOpens=0,preProbes=0,firmwareStarts=0,firmwareResumes=0,firmwareRetires=0;
static bool failPreOpen=false,failPreProbe=false,failFirmwareStart=false,failFirmwareRetire=false;
#endif
#ifdef TEST_RECOVERY_QUALIFY
static uint32 fakeCardPciStatus=0,fakeRootPciStatus=0;
static bool fakeAerAvailable=true,fakeAerFailAfterPre=false;
static uint32 fakeAerPre=0,fakeAerPost=0x20;
static bool fakeResetIssued=false;
template<class IO> static bool BlackholeHostPcieStatus(IO&,unsigned peer,uint32& status) {
	status=peer?fakeRootPciStatus:fakeCardPciStatus;return status==0;
}
#endif
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
struct tt_arc_telemetry_result {};
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
#ifdef TEST_LINK_RESET
	uint32 linkControl=3;
#endif
#ifdef TEST_LIVE_TAKEOVER
	void* qwen35BorrowedRoleScope=nullptr;
#endif
	uint32 CardRead(uint16 offset,uint8){
#ifdef TEST_LINK_RESET
		if(offset==0)return 0xb1401e52;
		if(offset==0x2c)return 0x00431e52;
		if(offset==4)return 0x402;
#else
		(void)offset;
#endif
		return 0;
	}
	uint32 RootRead(uint16 offset,uint8){
#ifdef TEST_LINK_RESET
		if(offset==0)return 0x20421e30;
		if(offset==8)return 0x06040000;
		if(offset==0x18)return 0x003f0100;
		if(offset==4)return 0x402;
		if(offset==0x3e)return linkControl;
		if(offset==0x100)return 0x00010001;
		if(offset==0x104)return 0;
		if(offset==0x134)return 0x01000000;
#endif
#ifdef TEST_RECOVERY_QUALIFY
		if (fakeAerAvailable && !(fakeAerFailAfterPre && fakeResetIssued)) {
			if (offset==0) return 0x20421e30;
			if (offset==0x34) return 0x40;
			if (offset==0x40) return 0x0010;
			if (offset==0x42) return 0x0042;
			if (offset==0x4a) return fakeResetIssued?6:0;
			if (offset==0x52) return 0x1234;
			if (offset==0x100) return 0x00010001;
			if (offset==0x104) return fakeResetIssued?fakeAerPost:fakeAerPre;
			if (offset==0x134) return 0x01000000;
		}
#endif
		(void)offset;return 0;
	}
#ifdef TEST_LINK_RESET
	void RootWrite(uint16 offset,uint8 size,uint32 value){
		assert(offset==0x3e && size==2);++linkWrites;
		trace.push_back(value&0x40?9001:9002);
		linkControl=failLinkAssert && (value&0x40)?value&~0x40U:value;
	}
#endif
	void CardWrite(uint16 offset,uint8 size,uint32 value){
#ifdef TEST_RECOVERY_QUALIFY
		if (offset==0x930 && size==4 && value==0x11) fakeResetIssued=true;
#else
		(void)offset;(void)size;(void)value;
#endif
	}
	status_t DisposeAfterVerifiedReset(const PioneerDriverRecoveryProof& p) {
		assert(p.ReadyToRetireOneOwner());
		if (!Step(PIONEER_RECOVERY_STAGE_OWNER_RELEASE)) return B_DEV_NOT_READY;
		postResetDisposeComplete=true;return B_OK;
	}
};
static bool PioneerPreResetDisableMaster(PioneerSessionIO&) { return Step(PIONEER_RECOVERY_STAGE_DISABLE_MASTER); }
static bool PioneerPciePayloadRead(PioneerSessionIO&,PioneerPciePayloadSnapshot& payload) {
	if (failPayloadRead) return false;
	if (++payloadReads==payloadDriftAt) payload.control^=0x20;
	return true;
}
static bool PioneerPciePayloadRestore(PioneerSessionIO&,const PioneerPciePayloadSnapshot&) {return !failPayloadRestore;}
template<class IO> static bool BlackholeAsicReset(IO&,unsigned,BlackholeAsicResetResult& r) {
	r.stage=7;
	if (!Step(PIONEER_RECOVERY_STAGE_ASIC_RESET)) return false;
#ifdef TEST_RECOVERY_QUALIFY
	fakeResetIssued=true;
#endif
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
template<class IO> static bool PioneerPcieRootResetReady(IO&) { return true; }
#ifdef TEST_LINK_RESET
template<class IO> struct PioneerQ35PcieAuditAccess {IO& io;};
template<class IO> static bool PioneerQ35PcieLinkSample(IO&,unsigned,unsigned& speed,unsigned& width) {
	speed=4;width=16;return fakeLinkReady;
}
template<class IO,class Check> struct PioneerGuardedMasterDisableIO {
	IO& io;Check& check;
	PioneerGuardedMasterDisableIO(IO& owned,Check& authority):io(owned),check(authority) {}
	uint32 RootRead(uint16 offset,uint8 size) {return check()?io.RootRead(offset,size):UINT32_MAX;}
	bool Valid(){return check();}
};
template<class IO,class Check> static bool PioneerPostResetPciRestore(PioneerGuardedMasterDisableIO<IO,Check>&,PioneerPostResetPciResult& r) {
	r.stage=4;trace.push_back(9003);return !failLinkRestore;
}
template<class IO,class Check> static bool PioneerPciePayloadRestore(PioneerGuardedMasterDisableIO<IO,Check>&,const PioneerPciePayloadSnapshot&) {
	trace.push_back(9004);return true;
}
template<class IO,class Check> static bool PioneerPreResetDisableMaster(PioneerGuardedMasterDisableIO<IO,Check>&) {
	trace.push_back(9005);return true;
}
#endif
struct FakePciResult {uint32 version=1,commands_before[2]={},bridge_before[3]={0x0001fff1,0,0},bars_before[6]={12,0,12,0,12,0};};
static bool PioneerPostResetPciFinish(PioneerSessionIO&,const FakePciResult&,unsigned& stage) {
	stage=2;return Step(PIONEER_RECOVERY_STAGE_PCI_IDLE);
}
struct PioneerPostResetMapping {
	bool firmwareRetryable=false;
	uint32 firmwareDiagnostic=2,dbiDiagnostic=0x166084;
	tt_arc_telemetry_result firmwareTelemetry{};
#ifdef TEST_FIRMWARE_RESET
	bool isPre=false;
	explicit PioneerPostResetMapping(FakeProfile&):isPre(++mappingCreates==2) {}
#else
	explicit PioneerPostResetMapping(FakeProfile&) {}
#endif
	bool Open(){
#ifdef TEST_FIRMWARE_RESET
		if (isPre) {++preOpens;return !failPreOpen;}
#endif
		return Step(PIONEER_RECOVERY_STAGE_MAPPING);
	}
	bool PrepareOpenRetry(){return retryableMappingFailure;}
	bool ProbeFirmware(unsigned,tt_arc_scratch_result& r){
#ifdef TEST_FIRMWARE_RESET
		if (isPre) {++preProbes;r.state=1;return !failPreProbe;}
#endif
		r.state=2;firmwareRetryable=firmwareNotReady;
		return Step(PIONEER_RECOVERY_STAGE_FIRMWARE) && !firmwareNotReady;
	}
	bool FirmwareRetryable() const {return firmwareRetryable;}
	bool RestoreInternalDbi(const PioneerRecoveryDbiSnapshot& s,uint32 control) {
		assert(s.valid && s.mps==(control&PIONEER_RECOVERY_DBI_MPS_MASK));return !failDbi;
	}
	bool InitializeFirmwareQueue(const tt_arc_scratch_result&,tt_arc_queue_result& r){r.state=3;return !failFirmwareQueue;}
	bool Audit(PioneerPostResetAuditResult& r){r.failure=2;r.core=7;return Step(PIONEER_RECOVERY_STAGE_FLEET);}
	bool AuditHostDma(){return Step(PIONEER_RECOVERY_STAGE_HOST_DMA);}
	bool Close(){return Step(PIONEER_RECOVERY_STAGE_CLOSE_MAPPING);}
};
#ifdef TEST_FIRMWARE_RESET
template<class IO,class Guard> class PioneerFirmwareRecoveryOwner {
	IO& ownerIo;uint64 ticket;bool started=false,retired=false;
	PioneerFirmwareResetTrace callback=nullptr;void* callbackContext=nullptr;
	struct FakeResult {uint32 failedStage=3;tt_arc_queue_result probe{};} result;
public:
	PioneerFirmwareRecoveryOwner(PioneerPostResetMapping& m,IO& io,uint64 t):ownerIo(io),ticket(t) {assert(m.isPre);}
	bool SetTrace(PioneerFirmwareResetTrace next,void* context) {
		if (started || callback || !next || !context) return false;
		callback=next;callbackContext=context;return true;
	}
	bool Start(Guard* guard,uint64 t,const tt_arc_scratch_result&,const tt_arc_telemetry_result&) {
		assert(guard && t==ticket && !started);++firmwareStarts;
		if (!Step(PIONEER_RECOVERY_STAGE_ASIC_RESET) || failFirmwareStart) return false;
		#ifdef TEST_RECOVERY_QUALIFY
		fakeResetIssued=true;
		#endif
		started=true;return true;
	}
	bool Resume(Guard* guard,uint64 t,unsigned) {
		assert(guard && t==ticket && started && !retired);++firmwareResumes;
		if (callback) {
			callback(callbackContext,PioneerFirmwareResetBeforeFirstPoll);
			callback(callbackContext,PioneerFirmwareResetAfterFirstVendor);
			callback(callbackContext,PioneerFirmwareResetAfterFirstMarker);
			callback=nullptr;
		}
		return !resetTimeout && !resumeResetTimeout;
	}
	status_t Retire(Guard* guard,uint64 t,const PioneerDriverRecoveryProof& p) {
		assert(guard && t==ticket && started && !retired && !ownerIo.postResetDisposeComplete && p.ReadyToRetireOneOwner()
			&& p.reset_generation==t && p.firmware_generation==t);++firmwareRetires;
		if (failFirmwareRetire) return B_DEV_NOT_READY;
		retired=true;return B_OK;
	}
	bool Retired() const {return retired;}
	const FakeResult& Result() const {return result;}
};
#endif
#ifdef TEST_LIVE_TAKEOVER
struct FakeDataflowService { bool ready=true,faulted=true;bool StoppedResetReady()const{return ready;}
	bool Faulted()const{return faulted;} };
#endif
struct PioneerStoreContext {
#ifdef TEST_RECOVERY_QUALIFY
	bool qualificationTraceActive=false;
	bool qualificationResetTriggered=false,qualificationFirstDelay=false;
	bool qualificationFirstVendor=false,qualificationFirstMarker=false;
	uint32 qualificationTraceAttempted=0;
	tt_pioneer_pcie_trace_sample qualificationTrace[PIONEER_PCIE_TRACE_SAMPLES]={};
#endif
	PioneerPostResetMapping* recoveryMapping=nullptr;bool recoveryAttempted=false,dispatchOwnerRetained=true;
#ifdef TEST_FIRMWARE_RESET
	PioneerPostResetMapping* recoveryPreResetMapping=nullptr;
	PioneerFirmwareRecoveryOwner<PioneerSessionIO,pioneer_driver_recovery_guard_v2_info>* recoveryFirmwareResetOwner=nullptr;
#endif
	PioneerDriverRecoveryProof recoveryFinishProof{};uint64 recoveryFinishTicket=0;bool recoveryFinishPending=false;
	PioneerPciePayloadSnapshot recoveryPayload{};bool recoveryFinishNeedsAudit=false;
#ifdef TEST_RETAINED_GATEWAY
	bool recoveryFinishReady=false,recoveryFinishKeepOwnerCookie=false,recoveryFinishUnreserveAttempted=false;
	bool q35PciReservationHeld=true,q35PciReleaseAttempted=false;
#endif
	uint64 recoveryPrepareTicket=0;bool recoveryPreparePending=false;
	BlackholeAsicResetResult recoveryResetResult{};uint64 recoveryResetTicket=0;bool recoveryResetPending=false;
	uint64 recoveryFirmwareTicket=0;bool recoveryFirmwarePending=false;
	uint64 recoveryMappingTicket=0;bool recoveryMappingPending=false;
#ifdef TEST_GRAPH_RECOVERY
	FakeGraphOwner graphOwner;FakeGraphOwner* qwen35GraphOwner=&graphOwner;
	FakeSharedRoleOwner sharedRoleOwner;FakeSharedRoleOwner* qwen35SharedRoleBinding=&sharedRoleOwner;
#endif
	PioneerRecoveryDbiSnapshot recoveryDbi;
#ifdef TEST_LIVE_TAKEOVER
	bool bankStorageOnly=true,qwen35LiveFaultRecoveryRequested=false;
	FakeDataflowService service;FakeDataflowService* qwen35DataflowService=&service;
#endif
#ifdef TEST_RECOVERY_QUALIFY
	void* qwen35DataflowMemory=nullptr;bool unsafe=false;
	bool Unsafe() { return unsafe; }
#endif
	struct {bool config_attempted=true,map_attempted=true;} transaction;
	FakeProfile profile;FakePciResult pci;PioneerSessionIO io;
	FakePciResult& Pci(){return pci;}
	~PioneerStoreContext(){contextDeleted++;delete recoveryMapping;
#ifdef TEST_FIRMWARE_RESET
		delete recoveryFirmwareResetOwner;delete recoveryPreResetMapping;
#endif
	}
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
	const PioneerDriverRecoveryProof& proof,uint64 ticket,pioneer_driver_recovery_guard_v2_info* guard) {
	if (!owner) return true;
	assert(guard==&recoveryGuardV2);
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
#ifdef TEST_RETAINED_GATEWAY
static std::vector<int> sleepDurations;static bool failSleep=false;
static int snooze(int duration){sleepDurations.push_back(duration);return failSleep?B_DEV_NOT_READY:B_OK;}
#else
static int snooze(int){return B_OK;}
#endif
struct thread_info {int team;};static int activeTeam=17,sLock=1,held=0;
static int find_thread(void*){return 1;}static int get_thread_info(int,thread_info* t){t->team=activeTeam;return B_OK;}
static status_t acquire_sem(int){assert(!held);held=1;return B_OK;}static void release_sem(int){assert(held);held=0;}
static status_t user_memcpy(void* dst,const void* src,size_t n){memcpy(dst,src,n);return B_OK;}
#include "pioneer_recovery_run.inc"
static tt_pioneer_recovery_run Request(){tt_pioneer_recovery_run r={};r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=3;return r;}
static void Setup() {
	failStage=abortCount=completeCount=contextDeleted=cookieDeleted=0;trace.clear();
#ifdef TEST_LINK_RESET
	linkWrites=0;failLinkAssert=failLinkRestore=false;fakeLinkReady=true;
#endif
#ifdef TEST_RECOVERY_QUALIFY
	fakeCardPciStatus=fakeRootPciStatus=0;fakeAerAvailable=true;fakeAerFailAfterPre=false;
	fakeAerPre=0;fakeAerPost=0x20;fakeResetIssued=false;
#endif
	failPayloadRead=failPayloadRestore=failFirmwareQueue=failDbi=false;
	payloadReads=payloadDriftAt=0;
	firmwareNotReady=retryableMappingFailure=false;
	ticketPending=invalidTicket=guardV2Missing=false;
	validateCalls=invalidateOnValidate=0;
	resetTimeout=resumeResetTimeout=false;resetResumeWaits=0;
#ifdef TEST_FIRMWARE_RESET
	mappingCreates=preOpens=preProbes=firmwareStarts=firmwareResumes=firmwareRetires=0;
	failPreOpen=failPreProbe=failFirmwareStart=failFirmwareRetire=false;
#endif
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
#ifdef TEST_RETAINED_GATEWAY
int main(){
	PioneerProbeCookie inspector;Setup();auto* owner=sRuntimeOwner;auto* context=owner->context;
	// Exercise the actual adapter's initial and resumed delays. A failed
	// wait denies every subsequent root and endpoint read.
	sleepDurations.clear();failSleep=false;
	Qwen35RecoveryResetIO delayed(context->io,context);
	delayed.RecoveryDelay();delayed.RecoveryDelay();
	assert((sleepDurations==std::vector<int>{2000000,100000}) && !delayed.delayFailed);
	Qwen35RecoveryResetIO resumed(context->io,context);resumed.RecoveryDelay();
	assert(sleepDurations.back()==2000000);
	failSleep=true;Qwen35RecoveryResetIO failed(context->io,context);failed.RecoveryDelay();
	const size_t sleeps=sleepDurations.size();failed.RecoveryDelay();
	assert(failed.delayFailed && sleepDurations.size()==sleeps
		&& failed.RootRead(0,4)==UINT32_MAX && failed.CardRead(0,4)==UINT32_MAX && !failed.RootReady());
	failSleep=false;sleepDurations.clear();
	auto r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY);
	assert(r.stage==PIONEER_RECOVERY_STAGE_GUARD_COMPLETE && !r.new_generation
		&& sRuntimeOwner==owner && owner->context==context && sColdRecoveryRequired
		&& sReloadGeneration==3 && context->recoveryFinishReady
		&& completeCount==0 && ticketPending
		&& !contextDeleted && !cookieDeleted);
#ifdef TEST_LINK_RESET
	assert(linkWrites==2);
	const auto asserted=std::find(trace.begin(),trace.end(),9001);
	const auto released=std::find(trace.begin(),trace.end(),9002);
	const auto asic=std::find(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET);
	assert(asserted<released && released<asic && asic!=trace.end());
#endif
	assert(std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_UNRESERVE)==0);
	assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,r,false)==B_NOT_ALLOWED);
	assert(sRuntimeOwner==owner && sColdRecoveryRequired && !contextDeleted && !cookieDeleted);
	assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,r,true)==B_OK);
	assert(!sRuntimeOwner && !sColdRecoveryRequired && sReloadGeneration==4
		&& r.stage==PIONEER_RECOVERY_STAGE_COMPLETE && r.new_generation==4
		&& contextDeleted==1 && cookieDeleted==1 && completeCount==1 && !ticketPending);
	assert(std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_UNRESERVE)==1);
	Cleanup();
#ifdef TEST_LINK_RESET
	Setup();owner=sRuntimeOwner;context=owner->context;resetTimeout=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& context->recoveryResetPending && linkWrites==2 && ticketPending);
	resumeResetTimeout=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& context->recoveryResetPending && linkWrites==2 && ticketPending);
	resumeResetTimeout=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& context->recoveryFinishReady && linkWrites==2 && ticketPending && !completeCount);
	assert(Qwen35FinalizeRetainedRecoveryLocked(owner,context,r,true)==B_OK);
	assert(!sRuntimeOwner && !sColdRecoveryRequired && linkWrites==2 && completeCount==1);
	Cleanup();
	for(unsigned fault=0;fault<3;fault++) {
		Setup();failLinkAssert=fault==0;failLinkRestore=fault==1;fakeLinkReady=fault!=2;
		r=Request();assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK
			&& r.operation_status==B_DEV_NOT_READY && sRuntimeOwner && sColdRecoveryRequired
			&& linkWrites==2 && abortCount==1 && !completeCount);
		const auto previous=trace;r=Request();
		assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_NOT_ALLOWED
			&& linkWrites==2 && trace==previous);
		Cleanup();
	}
#endif
	puts("PASS actual recovery defers completion until lease retirement; SIMULATED HARDWARE ONLY");
}
#elif defined(TEST_LINK_RESET)
int main(){
	PioneerProbeCookie inspector;
	Setup();auto r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && linkWrites==2);
	const auto assertIt=std::find(trace.begin(),trace.end(),9001);
	const auto releaseIt=std::find(trace.begin(),trace.end(),9002);
	const auto restoreIt=std::find(trace.begin(),trace.end(),9003);
	const auto payloadIt=std::find(trace.begin(),trace.end(),9004);
	const auto disableIt=std::find(trace.begin(),trace.end(),9005);
	const auto asicIt=std::find(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET);
	assert(assertIt<releaseIt && releaseIt<restoreIt && restoreIt<payloadIt
		&& payloadIt<disableIt && disableIt<asicIt && asicIt!=trace.end());
	for(unsigned fault=0;fault<3;fault++) {
		Setup();failLinkAssert=fault==0;failLinkRestore=fault==1;fakeLinkReady=fault!=2;
		r=Request();assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK
			&& r.operation_status==B_DEV_NOT_READY && sRuntimeOwner && sColdRecoveryRequired
			&& linkWrites==2 && abortCount==1 && !completeCount);
		const auto previous=trace;r=Request();
		assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_NOT_ALLOWED
			&& linkWrites==2 && trace==previous);
		Cleanup();
	}
#ifdef TEST_FIRMWARE_RESET
	Setup();resetTimeout=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& sRuntimeOwner && sRuntimeOwner->context->recoveryResetPending && linkWrites==2
		&& firmwareStarts==1 && firmwareResumes==1);
	resetTimeout=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && linkWrites==2 && firmwareStarts==1 && firmwareResumes==2);
#endif
	puts("PASS actual recovery orchestration: link, PCI restore, ASIC reset and no replay after failure; SIMULATED ONLY");
}
#elif defined(TEST_FIRMWARE_RESET)
int main(){
	PioneerProbeCookie inspector;
	#ifdef TEST_RECOVERY_QUALIFY
	(void)&Qwen35RecoveryForce;(void)&Qwen35RecoveryPcieEvidence;
	#endif
	Setup();auto r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && completeCount==1 && !abortCount);
	assert(preOpens==1 && preProbes==1 && firmwareStarts==1 && firmwareResumes==1 && firmwareRetires==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	Setup();resetTimeout=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.detail==BLACKHOLE_ASIC_RESET_TIMEOUT && sRuntimeOwner->context->recoveryResetPending
		&& sRuntimeOwner->context->recoveryPreResetMapping && sRuntimeOwner->context->recoveryFirmwareResetOwner
		&& ticketPending && !abortCount && firmwareStarts==1 && firmwareResumes==1);
	r=Request();assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK
		&& r.operation_status==B_DEV_NOT_READY && firmwareStarts==1 && firmwareResumes==2);
	resetTimeout=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && firmwareStarts==1 && firmwareResumes==3 && firmwareRetires==1);
	Setup();firmwareNotReady=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& sRuntimeOwner->context->recoveryFirmwarePending && sRuntimeOwner->context->recoveryFirmwareResetOwner
		&& firmwareStarts==1 && firmwareResumes==1 && !firmwareRetires);
	firmwareNotReady=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& firmwareStarts==1 && firmwareResumes==1 && firmwareRetires==1 && !sRuntimeOwner);
	Setup();failStage=PIONEER_RECOVERY_STAGE_MAPPING;retryableMappingFailure=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& sRuntimeOwner->context->recoveryMappingPending && sRuntimeOwner->context->recoveryFirmwareResetOwner
		&& firmwareStarts==1 && firmwareResumes==1 && !firmwareRetires);
	failStage=0;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& firmwareStarts==1 && firmwareResumes==1 && firmwareRetires==1 && !sRuntimeOwner);
	Setup();invalidateOnValidate=2;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& sRuntimeOwner->context->recoveryResetPending && firmwareStarts==1 && firmwareResumes==1
		&& ticketPending && !abortCount && sColdRecoveryRequired);
	const auto afterCompletedReset=trace;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& trace==afterCompletedReset && firmwareStarts==1 && firmwareResumes==1);
	Cleanup();
	Setup();resetTimeout=true;r=Request();assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK);
	invalidTicket=true;const auto before=trace;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& trace==before && firmwareStarts==1 && firmwareResumes==1 && sColdRecoveryRequired);
	Cleanup();
	for (unsigned fault=0;fault<4;fault++) {
		Setup();if (fault==0) failPreOpen=true;else if (fault==1) failPreProbe=true;
		else if (fault==2) failFirmwareStart=true;else failStage=PIONEER_RECOVERY_STAGE_HOST_DMA;
		r=Request();assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK
			&& r.operation_status==B_DEV_NOT_READY && sRuntimeOwner && sColdRecoveryRequired
			&& sRuntimeOwner->context->recoveryPreResetMapping && !firmwareRetires && abortCount==1);
		if (fault==0) assert(r.detail==PIONEER_RECOVERY_DETAIL_RESET_PREMAP);
		if (fault==1) assert(r.detail==(PIONEER_RECOVERY_DETAIL_RESET_PREPROBE|2));
		if (fault==2) assert(r.detail==(PIONEER_RECOVERY_DETAIL_RESET_SUBMIT|(3<<8)));
		Cleanup();
	}
	Setup();failFirmwareRetire=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& sRuntimeOwner && sRuntimeOwner->context->recoveryFirmwareResetOwner && firmwareRetires==1
		&& sColdRecoveryRequired && abortCount==1 && !completeCount);
	Cleanup();
	#ifdef TEST_RECOVERY_QUALIFY
	Setup();auto* live=sRuntimeOwner;live->finalFreeRetained=false;sQwen35ExclusiveCookie=live;
	live->context->dispatchOwnerRetained=false;
	live->context->qwen35DataflowService=nullptr;live->context->qwen35SharedRoleBinding=nullptr;
	live->context->qwen35GraphOwner=nullptr;fakeAerPost=0;sColdRecoveryRequired=false;r=Request();
	assert(Qwen35RecoveryQualify(live,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !live->context && !sColdRecoveryRequired && firmwareStarts==1
		&& firmwareResumes==1 && firmwareRetires==1 && !abortCount && completeCount==1);
	sQwen35ExclusiveCookie=nullptr;delete live;
	#endif
	puts("PASS firmware reset wired into actual recovery sequence: one-shot resume, proof retirement and fault retention; SIMULATED ONLY");
}
#else
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
	Setup();failStage=PIONEER_RECOVERY_STAGE_DISABLE_MASTER;auto prepare=Request();
	assert(Qwen35RecoveryRun(&inspector,&prepare,sizeof(prepare))==B_OK
		&& prepare.operation_status==B_DEV_NOT_READY && prepare.detail==PIONEER_RECOVERY_DETAIL_PREPARE_WAIT
		&& sRuntimeOwner->context->recoveryPreparePending && ticketPending && !abortCount);
	prepare=Request();assert(Qwen35RecoveryRun(&inspector,&prepare,sizeof(prepare))==B_OK
		&& prepare.operation_status==B_DEV_NOT_READY && sRuntimeOwner->context->recoveryPreparePending);
	assert(!std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET));
	failStage=0;prepare=Request();
	assert(Qwen35RecoveryRun(&inspector,&prepare,sizeof(prepare))==B_OK && prepare.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && !abortCount && completeCount==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	Setup();payloadDriftAt=2;prepare=Request();
	assert(Qwen35RecoveryRun(&inspector,&prepare,sizeof(prepare))==B_OK
		&& prepare.operation_status==B_DEV_NOT_READY && prepare.detail==PIONEER_RECOVERY_DETAIL_PREPARE_WAIT
		&& sRuntimeOwner->context->recoveryPreparePending && !abortCount && ticketPending
		&& !std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET));
	prepare=Request();assert(Qwen35RecoveryRun(&inspector,&prepare,sizeof(prepare))==B_OK
		&& prepare.operation_status==B_OK && !sRuntimeOwner && !sColdRecoveryRequired
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	Setup();invalidateOnValidate=1;prepare=Request();
	assert(Qwen35RecoveryRun(&inspector,&prepare,sizeof(prepare))==B_OK
		&& prepare.operation_status==B_DEV_NOT_READY && sRuntimeOwner->context->recoveryPreparePending
		&& ticketPending && !abortCount && !std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET));
	const auto invalidPrepareTrace=trace;prepare=Request();
	assert(Qwen35RecoveryRun(&inspector,&prepare,sizeof(prepare))==B_OK
		&& prepare.operation_status==B_DEV_NOT_READY && trace==invalidPrepareTrace && sColdRecoveryRequired);
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
	Setup();failStage=PIONEER_RECOVERY_STAGE_MAPPING;retryableMappingFailure=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_MAPPING && r.detail==PIONEER_RECOVERY_DETAIL_MAPPING_WAIT
		&& sRuntimeOwner && sRuntimeOwner->context->recoveryMappingPending
		&& sRuntimeOwner->context->recoveryMappingTicket==9 && ticketPending && !abortCount);
	r=Request();assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK
		&& r.operation_status==B_DEV_NOT_READY && sRuntimeOwner->context->recoveryMappingPending);
	failStage=0;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && completeCount==1 && !abortCount
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_PCI_RESTORE)==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_MAPPING)==3);
	Setup();failStage=PIONEER_RECOVERY_STAGE_MAPPING;retryableMappingFailure=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY);
	invalidTicket=true;const auto mappingTrace=trace;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& trace==mappingTrace && sRuntimeOwner && sColdRecoveryRequired && ticketPending);
	Cleanup();
	Setup();failStage=PIONEER_RECOVERY_STAGE_MAPPING;retryableMappingFailure=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY);
	failStage=0;firmwareNotReady=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.detail==PIONEER_RECOVERY_DETAIL_FIRMWARE_WAIT
		&& !sRuntimeOwner->context->recoveryMappingPending && sRuntimeOwner->context->recoveryFirmwarePending);
	firmwareNotReady=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_PCI_RESTORE)==1);
	Setup();firmwareNotReady=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_FIRMWARE && r.detail==PIONEER_RECOVERY_DETAIL_FIRMWARE_WAIT && sRuntimeOwner
		&& sRuntimeOwner->context->recoveryFirmwarePending && !sRuntimeOwner->context->recoveryResetPending
		&& sRuntimeOwner->context->recoveryFirmwareTicket==9 && ticketPending && !abortCount && sColdRecoveryRequired);
	r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& sRuntimeOwner && sRuntimeOwner->context->recoveryFirmwarePending && ticketPending && !abortCount);
	firmwareNotReady=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && completeCount==1 && !abortCount
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_PCI_RESTORE)==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_MAPPING)==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_FIRMWARE)==3);
	Setup();firmwareNotReady=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY);
	invalidTicket=true;const auto firmwareTrace=trace;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& trace==firmwareTrace && sRuntimeOwner && sColdRecoveryRequired && ticketPending);
	Cleanup();
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
	Setup();invalidateOnValidate=4;r=Request();
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
		assert(sColdRecoveryRequired && sReloadGeneration==3 && abortCount==(stage==1?0U:1U));
		if (stage==1) assert(ticketPending && sRuntimeOwner->context->recoveryPreparePending
			&& r.detail==PIONEER_RECOVERY_DETAIL_PREPARE_WAIT);
		if(stage<12) assert(sRuntimeOwner && !contextDeleted && !cookieDeleted && !completeCount);
		else assert(sRuntimeOwner && !contextDeleted && !cookieDeleted && completeCount==1);
		Cleanup();
	}
	Setup();failPayloadRead=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_DISABLE_MASTER && trace.size()==1
		&& r.detail==PIONEER_RECOVERY_DETAIL_PREPARE_WAIT
		&& sRuntimeOwner && sColdRecoveryRequired && !abortCount && ticketPending
		&& sRuntimeOwner->context->recoveryPreparePending);
	const auto prepareTrace=trace;invalidTicket=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& trace==prepareTrace && ticketPending && sColdRecoveryRequired);
	invalidTicket=false;failPayloadRead=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && !abortCount && completeCount==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	Cleanup();Setup();failPayloadRestore=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_PCI_RESTORE && trace.size()==3
		&& sRuntimeOwner && sColdRecoveryRequired && abortCount==1);
	Cleanup();Setup();failFirmwareQueue=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_FIRMWARE && r.detail==(PIONEER_RECOVERY_DETAIL_FIRMWARE_QUEUE|3)
		&& sRuntimeOwner && sColdRecoveryRequired && abortCount==1);
	Cleanup();Setup();failDbi=true;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_FIRMWARE && r.detail==(PIONEER_RECOVERY_DETAIL_FIRMWARE_DBI|0x166084)
		&& sRuntimeOwner && sColdRecoveryRequired && abortCount==1);
	Cleanup();Setup();sRuntimeOwner->context->recoveryDbi.valid=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_NOT_ALLOWED
		&& trace.empty() && sRuntimeOwner && sColdRecoveryRequired && !abortCount);
	Cleanup();Setup();sRuntimeOwner->context->recoveryDbi.mps=0x40;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_DISABLE_MASTER && trace.size()==1
		&& r.detail==PIONEER_RECOVERY_DETAIL_PREPARE_WAIT && sRuntimeOwner && sColdRecoveryRequired
		&& sRuntimeOwner->context->recoveryPreparePending && !abortCount && ticketPending);
	Cleanup();
	Setup();sRuntimeOwner->finalFreeRetained=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && trace.empty());
	Cleanup();Setup();sOpenCookies=2;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && trace.empty());
	Cleanup();Setup();r=Request();r.expected_generation=2;
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_BAD_VALUE && trace.empty());
	Cleanup();
#ifdef TEST_LIVE_TAKEOVER
	#ifdef TEST_RECOVERY_QUALIFY
	Setup();auto* traced=sRuntimeOwner->context;traced->qualificationTraceActive=true;
	Qwen35RecoveryResetIO traceIo(traced->io,traced);
	traceIo.CardRead(0,4);traceIo.RecoveryDelay();
	assert(!traced->qualificationTrace[PIONEER_PCIE_TRACE_AFTER_TRIGGER].valid);
	traceIo.CardWrite(0x930,4,0x11);traceIo.RecoveryDelay();
	// Recovery retries create a new adapter but retain the same context. The
	// first post-trigger endpoint observations must survive that boundary.
	Qwen35RecoveryResetIO resumedIo(traced->io,traced);
	const uint32 firstPollAer=traced->qualificationTrace[PIONEER_PCIE_TRACE_BEFORE_FIRST_POLL].uncorrectable;
	fakeAerPost=0x40;resumedIo.RecoveryDelay();
	assert(traced->qualificationTrace[PIONEER_PCIE_TRACE_BEFORE_FIRST_POLL].uncorrectable==firstPollAer);
	resumedIo.CardRead(0,4);resumedIo.CardRead(4,2);
	assert(traced->qualificationTrace[PIONEER_PCIE_TRACE_AFTER_TRIGGER].valid
		&& traced->qualificationTrace[PIONEER_PCIE_TRACE_BEFORE_FIRST_POLL].valid
		&& traced->qualificationTrace[PIONEER_PCIE_TRACE_AFTER_FIRST_VENDOR].valid
		&& traced->qualificationTrace[PIONEER_PCIE_TRACE_AFTER_FIRST_MARKER].valid);
	assert((traced->qualificationTraceAttempted & 0x3c0)==0x3c0);
	Cleanup();
	Setup();traced=sRuntimeOwner->context;traced->qualificationTraceActive=true;
	fakeAerAvailable=false;
	Qwen35QualificationTrace(traced,PIONEER_PCIE_TRACE_AFTER_FIRST_VENDOR);
	assert((traced->qualificationTraceAttempted & (1U<<PIONEER_PCIE_TRACE_AFTER_FIRST_VENDOR))
		&& !traced->qualificationTrace[PIONEER_PCIE_TRACE_AFTER_FIRST_VENDOR].valid
		&& !(traced->qualificationTraceAttempted & (1U<<PIONEER_PCIE_TRACE_AFTER_FIRST_MARKER)));
	sQwen35ExclusiveCookie=sRuntimeOwner;
	tt_pioneer_pcie_evidence missingSample{};missingSample.version=2;
	missingSample.expected_tag=PIONEER_DRIVER_BUILD_TAG;missingSample.expected_generation=3;
	assert(Qwen35RecoveryPcieEvidence(sRuntimeOwner,&missingSample,sizeof(missingSample))==B_OK
		&& (missingSample.reserved[0] & (1U<<PIONEER_PCIE_TRACE_AFTER_FIRST_VENDOR))
		&& !missingSample.samples[PIONEER_PCIE_TRACE_AFTER_FIRST_VENDOR].valid
		&& !(missingSample.reserved[0] & (1U<<PIONEER_PCIE_TRACE_AFTER_FIRST_MARKER)));
	missingSample={};missingSample.version=1;
	missingSample.expected_tag=PIONEER_DRIVER_BUILD_TAG;missingSample.expected_generation=3;
	assert(Qwen35RecoveryPcieEvidence(sRuntimeOwner,&missingSample,sizeof(missingSample))==B_OK
		&& missingSample.reserved[0]==0);
	sQwen35ExclusiveCookie=nullptr;
	Cleanup();
	for (unsigned invalid=0;invalid<6;invalid++) {
		Setup();auto* candidate=sRuntimeOwner;candidate->finalFreeRetained=false;sQwen35ExclusiveCookie=candidate;
		auto* c=candidate->context;c->qwen35DataflowService=nullptr;c->qwen35GraphOwner=nullptr;
		c->qwen35SharedRoleBinding=nullptr;c->dispatchOwnerRetained=false;sColdRecoveryRequired=false;
		if (invalid==0) c->qwen35DataflowMemory=reinterpret_cast<void*>(1);
		if (invalid==1) c->qwen35DataflowService=&c->service;
		if (invalid==2) c->qwen35GraphOwner=&c->graphOwner;
		if (invalid==3) c->qwen35SharedRoleBinding=&c->sharedRoleOwner;
		if (invalid==4) c->unsafe=true;
		if (invalid==5) c->recoveryDbi.valid=false;
		r=Request();assert(Qwen35RecoveryQualify(candidate,&r,sizeof(r))==B_OK
			&& r.operation_status==B_BUSY && trace.empty() && !retainCalls && !sColdRecoveryRequired);
		sQwen35ExclusiveCookie=nullptr;Cleanup();
	}
	Setup();auto* candidate=sRuntimeOwner;candidate->finalFreeRetained=false;sQwen35ExclusiveCookie=candidate;
	candidate->context->qwen35DataflowService=nullptr;candidate->context->qwen35GraphOwner=nullptr;
	candidate->context->qwen35SharedRoleBinding=nullptr;candidate->context->dispatchOwnerRetained=false;
	sColdRecoveryRequired=false;r=Request();
	assert(Qwen35RecoveryForce(candidate,&r,sizeof(r))==B_OK && r.operation_status==B_BUSY && trace.empty());
	fakeRootPciStatus=6;r=Request();
	assert(Qwen35RecoveryQualify(candidate,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.output_reserved==0x00060000 && trace.empty() && !retainCalls && !sColdRecoveryRequired);
	fakeRootPciStatus=0;fakeCardPciStatus=0xffff;r=Request();
	assert(Qwen35RecoveryQualify(candidate,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.output_reserved==0x0000ffff && trace.empty() && !retainCalls && !sColdRecoveryRequired);
	fakeCardPciStatus=0;fakeAerAvailable=false;r=Request();
	assert(Qwen35RecoveryQualify(candidate,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.new_generation==UINT64_MAX && trace.empty() && !retainCalls && !sColdRecoveryRequired);
	fakeAerAvailable=true;fakeAerPre=1;r=Request();
	assert(Qwen35RecoveryQualify(candidate,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.new_generation==UINT64_C(0x00000001ffffffff) && trace.empty() && !retainCalls && !sColdRecoveryRequired);
	fakeAerPre=0;fakeAerFailAfterPre=true;
	failDbi=true;r=Request();
	assert(Qwen35RecoveryQualify(candidate,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.stage==PIONEER_RECOVERY_STAGE_FIRMWARE && r.new_generation==UINT64_C(0x00000000ffffffff)
		&& sColdRecoveryRequired && retainCalls==1);
	sQwen35ExclusiveCookie=nullptr;Cleanup();
	Setup();candidate=sRuntimeOwner;candidate->finalFreeRetained=false;sQwen35ExclusiveCookie=candidate;
	candidate->context->qwen35DataflowService=nullptr;candidate->context->qwen35GraphOwner=nullptr;
	candidate->context->qwen35SharedRoleBinding=nullptr;candidate->context->dispatchOwnerRetained=false;
	sColdRecoveryRequired=false;fakeAerAvailable=true;failDbi=true;r=Request();
	assert(Qwen35RecoveryQualify(candidate,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& r.new_generation==UINT64_C(0x0000000000000020) && sColdRecoveryRequired);
	tt_pioneer_pcie_evidence evidence{};evidence.version=1;
	evidence.expected_tag=PIONEER_DRIVER_BUILD_TAG;evidence.expected_generation=3;
	assert(Qwen35RecoveryPcieEvidence(candidate,&evidence,sizeof(evidence))==B_OK
		&& evidence.samples[PIONEER_PCIE_TRACE_BEFORE_RESET].valid
		&& evidence.samples[PIONEER_PCIE_TRACE_BEFORE_RESET].device_status==0
		&& evidence.samples[PIONEER_PCIE_TRACE_AFTER_RESET].valid
		&& evidence.samples[PIONEER_PCIE_TRACE_AFTER_RESET].device_status==6
		&& evidence.samples[PIONEER_PCIE_TRACE_ON_FAILURE].uncorrectable==0x20);
	#ifdef TEST_FIRMWARE_RESET
	assert(evidence.samples[PIONEER_PCIE_TRACE_BEFORE_FIRST_POLL].valid
		&& evidence.samples[PIONEER_PCIE_TRACE_AFTER_FIRST_VENDOR].valid
		&& evidence.samples[PIONEER_PCIE_TRACE_AFTER_FIRST_MARKER].valid);
	#endif
	assert(Qwen35RecoveryPcieEvidence(candidate,&evidence,sizeof(evidence))==B_BAD_VALUE);
	evidence={};evidence.version=2;evidence.expected_tag=PIONEER_DRIVER_BUILD_TAG;evidence.expected_generation=3;
	assert(Qwen35RecoveryPcieEvidence(candidate,&evidence,sizeof(evidence))==B_OK
		&& (evidence.reserved[0] & (1U<<PIONEER_PCIE_TRACE_BEFORE_RESET))
		&& (evidence.reserved[0] & (1U<<PIONEER_PCIE_TRACE_ON_FAILURE)));
	evidence={};evidence.version=1;evidence.expected_tag=PIONEER_DRIVER_BUILD_TAG;evidence.expected_generation=3;
	activeTeam=18;assert(Qwen35RecoveryPcieEvidence(candidate,&evidence,sizeof(evidence))==B_NOT_ALLOWED);
	activeTeam=17;sOpenCookies=2;assert(Qwen35RecoveryPcieEvidence(candidate,&evidence,sizeof(evidence))==B_NOT_ALLOWED);
	sOpenCookies=1;evidence.expected_generation=4;
	assert(Qwen35RecoveryPcieEvidence(candidate,&evidence,sizeof(evidence))==B_BAD_VALUE);
	evidence.expected_generation=3;sQwen35ExclusiveCookie=nullptr;
	assert(Qwen35RecoveryPcieEvidence(candidate,&evidence,sizeof(evidence))==B_NOT_ALLOWED);
	sQwen35ExclusiveCookie=nullptr;Cleanup();
	Setup();candidate=sRuntimeOwner;candidate->finalFreeRetained=false;sQwen35ExclusiveCookie=candidate;
	candidate->context->qwen35DataflowService=nullptr;candidate->context->qwen35GraphOwner=nullptr;
	candidate->context->qwen35SharedRoleBinding=nullptr;candidate->context->dispatchOwnerRetained=false;
	sColdRecoveryRequired=false;
	r=Request();assert(Qwen35RecoveryQualify(candidate,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& r.stage==PIONEER_RECOVERY_STAGE_COMPLETE && !sRuntimeOwner && !candidate->context
		&& !sColdRecoveryRequired && retainCalls==1 && completeCount==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
	sQwen35ExclusiveCookie=nullptr;delete candidate;
	#endif
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
	// If the live client exits while firmware is still starting, a new
	// inspector can resume the retained ticket without reissuing reset.
	Setup();fault=sRuntimeOwner;fault->finalFreeRetained=false;sQwen35ExclusiveCookie=fault;
	firmwareNotReady=true;r=Request();
	assert(Qwen35RecoveryForce(fault,&r,sizeof(r))==B_OK && r.operation_status==B_DEV_NOT_READY
		&& fault->context->recoveryFirmwarePending && ticketPending);
	fault->finalFreeRetained=true;sQwen35ExclusiveCookie=nullptr;firmwareNotReady=false;r=Request();
	assert(Qwen35RecoveryRun(&inspector,&r,sizeof(r))==B_OK && r.operation_status==B_OK
		&& !sRuntimeOwner && !sColdRecoveryRequired && cookieDeleted==1
		&& std::count(trace.begin(),trace.end(),PIONEER_RECOVERY_STAGE_ASIC_RESET)==1);
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
#endif
