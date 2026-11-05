/* SPDX-License-Identifier: MIT */
// Distinct fixed synthetic diagnostic. Legacy driver supplies PCI discovery,
// reload guard and read-only baseline machinery ONLY; none of its compute,
// model,Work,arbitrary-data or automatic-close routes are published here.
#include <Drivers.h>
#if !defined(PIONEER_NATIVE_BFP_DIAGNOSTIC) || PIONEER_NATIVE_BFP_DIAGNOSTIC != 1
#error Build only through build_native_bfp_diagnostic.py
#endif
#if defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT)
#error Native BFP fixed qualification never selects a model/Work/old native context
#endif
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
#if PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC != 1 || defined(PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC) || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110500010096ULL
#error Distinct fixed full-K original-affine identity required
#endif
#elif defined(PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC)
#if PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110500010095ULL
#error Distinct exact-Q4 fixed qualification identity required
#endif
#elif !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110300010078ULL
#error Distinct exact native-BFP diagnostic identity required
#endif
#define find_device PioneerNativeBfpLegacyFindDevice
#include "tenstorrent_pci_probe.cpp"
#undef find_device
#include "PioneerNativeBfpLease.h"
#include "PioneerNativeBfpGeneratedImage.h"
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
#include "PioneerNativeExactQ4AffineProtocol.h"
#endif
#ifdef PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC
#include "PioneerNativeExactQ4Protocol.h"
#endif

namespace {
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
using ActiveRequest=PioneerNativeExactQ4AffineRequest;
static constexpr uint32 kSupport=PIONEER_NATIVE_EXACT_Q4_AFFINE_SUPPORT,kRun=PIONEER_NATIVE_EXACT_Q4_AFFINE_RUN,kEnd=PIONEER_NATIVE_EXACT_Q4_AFFINE_END;
static bool RequestValid(const ActiveRequest& r){return PioneerNativeExactQ4AffineRequestValid(r);}
#elif defined(PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC)
using ActiveRequest=PioneerNativeExactQ4Request;
static constexpr uint32 kSupport=PIONEER_NATIVE_EXACT_Q4_SUPPORT,kRun=PIONEER_NATIVE_EXACT_Q4_RUN,kEnd=PIONEER_NATIVE_EXACT_Q4_END;
static bool RequestValid(const ActiveRequest& r){return PioneerNativeExactQ4RequestValid(r);}
#else
using ActiveRequest=PioneerNativeBfpRequest;
static constexpr uint32 kSupport=PIONEER_NATIVE_BFP_SUPPORT,kRun=PIONEER_NATIVE_BFP_RUN,kEnd=PIONEER_NATIVE_BFP_END;
static bool RequestValid(const ActiveRequest& r){return PioneerNativeBfpRequestValid(r);}
#endif
struct NativeCookie { PioneerProbeCookie* base=NULL;bool used=false; };
struct NativeIO: PioneerComputeNocIO {
	area_id views[7]={-1,-1,-1,-1,-1,-1,-1};void* pages[7]={};bool invalid=false;
	NativeIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,tt_arc_telemetry_result& t,
		tt_tensix_reset_result& c,tt_dram_samples_result& d,tt_compute_noc_result& n):PioneerComputeNocIO(p,a,t,c,d,n) {}
	bool Map() {
		if(!PioneerComputeNocIO::Map())return false;
		const uint32_t offsets[]={0x19006000,0x19070000,0x1906e000,0x1908f000,0x1917f000,0x19312000,0x19313000};
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
		const unsigned sizes[]={4096,8192,8192,32768,4096,4096,4096};
#else
		const unsigned sizes[]={4096,4096,8192,16384,4096,4096,4096};
#endif
		for(unsigned i=0;i<7;i++) {
			views[i]=map_physical_memory("P100A isolated native BFP owned view",profile.bar0_host_start+offsets[i],sizes[i],
				B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|(i==6?0:B_KERNEL_WRITE_AREA)|B_UNCACHED_MEMORY,&pages[i]);
			if(views[i]<B_OK)return false;
		}
		return true;
	}
	bool Unmap() {
		// Virtual views ONLY. Never inherit diagnostic TLB/ARC/reset rollback.
		for(unsigned i=0;i<7;i++)if(views[i]>=B_OK) {
			if(delete_area(views[i])!=B_OK)return false;
			views[i]=-1;pages[i]=NULL;
		}
		area_id* inherited[]={&nocArea,&dataArea,&dramArea,&resetArea,&csmArea,&scratchArea,&tlbArea,&area};
		for(auto* view:inherited)if(*view>=B_OK) { if(delete_area(*view)!=B_OK)return false;*view=-1; }
		data=NULL;nocRegisters=dramNiu=reset=csm=scratch=nodeID=NULL;tlb=NULL;return true;
	}
	static bool Within(uint32_t a,uint32_t first,unsigned bytes) { return !(a&3) && a>=first && a-first<bytes; }
	volatile uint32* Pointer(uint32_t address) {
		unsigned view=0,offset=0;
		if(address==0x6000) {view=0;offset=0;}
		else if(Within(address,0x70000,
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
			8192
#else
			4096
#endif
		)) {view=1;offset=address-0x70000;}
		else if(Within(address,0x6e000,8192)) {view=2;offset=address-0x6e000;}
		else if(Within(address,0x8fff0,1104) || Within(address,0x90ff0,528) || Within(address,0x92000,256)) {view=3;offset=address-0x8f000;}
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
		else if(Within(address,0x93ff0,80) || Within(address,0x94070,80) || Within(address,0x95ff0,1040)) {view=3;offset=address-0x8f000;}
#endif
		else if(Within(address,0x17ffc0,12)) {view=4;offset=address-0x17f000;}
		else {invalid=true;return NULL;}
		if(!pages[view]) {invalid=true;return NULL;}
		return reinterpret_cast<volatile uint32*>(static_cast<uint8*>(pages[view])+offset);
	}
	uint32 BfpRead(uint32 a) { auto* p=Pointer(a);return p?PioneerRead32(p):UINT32_MAX; }
	void BfpWrite(uint32 a,uint32 value) { auto* p=Pointer(a);if(p)PioneerWrite32(p,value); }
	uint32 BfpControlRead(unsigned i) {
		const unsigned offsets[]={0x234,0xa0,0x90};
		if(i>=3 || !pages[5]) {invalid=true;return UINT32_MAX;}
		return PioneerRead32(reinterpret_cast<const volatile uint32*>(static_cast<const uint8*>(pages[5])+offsets[i]));
	}
	void BfpResetWrite(uint32 value) {
		if(!pages[5] || (value!=0x87ffff && value!=0x46800 && value!=0x47800)) {invalid=true;return;}
		PioneerWrite32(reinterpret_cast<volatile uint32*>(static_cast<uint8*>(pages[5])+0x1b0),value);
	}
	uint32 BfpPcRead() {
		if(!pages[6]) {invalid=true;return UINT32_MAX;}
		return PioneerRead32(reinterpret_cast<const volatile uint32*>(static_cast<const uint8*>(pages[6])+0x140));
	}
	bool BfpHealthy() const { return !invalid && noc.version==1 && noc.state==TT_COMPUTE_NOC_PASSED && noc.tlb_restored && !noc.recovery_required; }
	uint64 BfpNow() { return uint64(system_time())*1000; }
	void Pause() { spin(100); }
	bool BaselineClean() const {
		return (!dirty || arc.tlb_restored) && (!telemetryAttempted || telemetry.tlb_restored)
			&& (!resetAttempted || tensix.tlb_restored)
			&& (!dramAttempted || (dram.tlb_restored && dram.data_restored && !dram.recovery_required))
			&& (!nocAttempted || (noc.tlb_restored && !noc.recovery_required));
	}
};
struct NativeOwner {
	NativeCookie* cookie;
	unsigned bits;
	tt_pioneer_profile profile;
	tt_arc_scratch_result arc={};tt_arc_telemetry_result telemetry={};tt_tensix_reset_result tensix={};tt_dram_samples_result dram={};
	tt_compute_noc_result noc={};
	NativeIO io;PioneerNativeBfpLease lease;
	tt_pioneer_mmio_result mmio={};tt_pioneer_mmio_transaction transaction={};
	bool reserved=false;
	NativeOwner(NativeCookie* c,unsigned b,const tt_pioneer_profile& p):cookie(c),bits(b),profile(p),io(profile,arc,telemetry,tensix,dram,noc) {}
};
static NativeOwner* owner=NULL; // under sLock; retained intact on UNKNOWN
static void Fault(NativeOwner& o) { o.lease.Abandon();sColdRecoveryRequired=true; }
static void Reply(ActiveRequest& r,const NativeOwner& o) {
	r.result=o.lease.Result();r.nonce=1;
	if(sColdRecoveryRequired) {r.result.state=PIONEER_NATIVE_BFP_UNKNOWN;r.result.cold=1;}
}
static bool FinishMmio(NativeOwner& o) {
	// Do NOT inherit generic End's best-effort continuation after a VM or
	// PCI restore failure. Each failed step is terminal: NO second attempt
	// and NO subsequent config/device write. Caller latches/retains UNKNOWN.
	if(!o.transaction.config_attempted)return true;
	if(o.transaction.map_attempted) {
		if(!o.io.Unmap())return false;
		o.transaction.map_attempted=false;
	}
	const auto identity=[&] {
		return o.io.CardRead(0,4)==0xb1401e52 && o.io.CardRead(0x2c,4)==0x00431e52
			&& o.io.RootRead(0,4)==0x20421e30 && o.io.RootRead(0x18,4)==0x003f0100;
	};
	if(!identity())return false;
	o.io.CardWrite(4,2,0);if(o.io.CardRead(4,2)!=0)return false;
	if(!identity())return false;
	o.io.RootWrite(4,2,0);if(o.io.RootRead(4,2)!=0)return false;
	const unsigned order[]={5,4,1,0,3,2};
	for(unsigned i:order) {
		if(!identity() || o.io.CardRead(4,2)!=0 || o.io.RootRead(4,2)!=0)return false;
		o.io.CardWrite(0x10+i*4,4,o.mmio.bars_before[i]);
		if(o.io.CardRead(0x10+i*4,4)!=o.mmio.bars_before[i])return false;
		o.mmio.bars_after[i]=o.mmio.bars_before[i];
	}
	const unsigned offsets[]={0x28,0x2c,0x24},indices[]={1,2,0};
	for(unsigned i=0;i<3;i++) {
		if(!identity() || o.io.CardRead(4,2)!=0 || o.io.RootRead(4,2)!=0)return false;
		o.io.RootWrite(offsets[i],4,o.mmio.bridge_before[indices[i]]);
		if(o.io.RootRead(offsets[i],4)!=o.mmio.bridge_before[indices[i]])return false;
		o.mmio.bridge_after[indices[i]]=o.mmio.bridge_before[indices[i]];
	}
	if(!identity() || o.io.CardRead(4,2)!=0 || o.io.RootRead(4,2)!=0)return false;
	o.mmio.commands_after[0]=o.mmio.commands_after[1]=0;o.mmio.restored=1;o.mmio.stage=5;
	o.transaction.config_attempted=false;return true;
}
static void DisposeKnown(NativeOwner* o) {
	if(o->reserved)sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
	sRuntimeOwner=NULL;owner=NULL;delete o;
}
static status_t Open(const char* name,uint32 flags,void** cookie) {
	auto* n=new(std::nothrow) NativeCookie;if(!n)return B_NO_MEMORY;
	void* original=NULL;status_t status=ProbeOpen(name,flags,&original);
	if(status!=B_OK) {delete n;return status;}
	n->base=static_cast<PioneerProbeCookie*>(original);*cookie=n;return B_OK;
}
static status_t Free(void* value) {
	auto* n=static_cast<NativeCookie*>(value);if(!n || !n->base)return B_BAD_VALUE;
	status_t status=acquire_sem(sLock);if(status!=B_OK)return status;
	if(owner && owner->cookie==n) {
		// Close/EOF/failed user validation is NOT END. Persist quarantine;
		// retain cookie, reservation, mappings, saved bytes and live context.
		Fault(*owner);if(sOpenCookies)sOpenCookies--;release_sem(sLock);return B_OK;
	}
	release_sem(sLock);status=ProbeFree(n->base);if(status==B_OK)delete n;return status;
}
static status_t Control(void* value,uint32 operation,void* data,size_t length) {
	auto* n=static_cast<NativeCookie*>(value);
	if(geteuid()!=0 || !n || !n->base || n->base->magic!=0x50313030 || n->base->context)return B_NOT_ALLOWED;
	thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
	if(status!=B_OK || thread.team!=n->base->team)return status!=B_OK?status:B_NOT_ALLOWED;
	if(operation==TT_PCI_PROBE_RELOAD_STATUS || operation==TT_PCI_PROBE_RELOAD_QUIESCE)
		return DriverReloadControl(n->base,operation,data,length);
	if(operation!=kSupport && operation!=kRun && operation!=kEnd)return B_NOT_ALLOWED;
	if(!data || length!=sizeof(ActiveRequest))return B_BAD_VALUE;
	ActiveRequest r={};status=user_memcpy(&r,data,sizeof(r));
	if(status!=B_OK || !RequestValid(r))return status!=B_OK?status:B_BAD_VALUE;
	if(operation==kSupport) {
		if(r.bits || r.nonce || r.accept)return B_BAD_VALUE;
		status=acquire_sem(sLock);if(status!=B_OK)return status;
		if(r.expected_generation!=sReloadGeneration)status=B_BAD_VALUE;
		else {r.result.state=sColdRecoveryRequired?PIONEER_NATIVE_BFP_UNKNOWN:PIONEER_NATIVE_BFP_EMPTY;r.result.cold=sColdRecoveryRequired;status=user_memcpy(data,&r,sizeof(r));}
		release_sem(sLock);return status;
	}
#if defined(PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC) || defined(PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC)
	if(r.bits!=4)return B_BAD_VALUE;
#endif
	if((r.bits!=4 && r.bits!=8) || (operation==kRun?(r.nonce || r.accept):r.nonce!=1 || r.accept!=1))return B_BAD_VALUE;
	status=AcquireProbeLock(n->base);if(status!=B_OK)return status;
	if(r.expected_generation!=sReloadGeneration || !sReloadGuard || sReloadGuard->version!=1 || sOpenCookies!=1)status=B_BUSY;
	else if(operation==kRun) {
		if(n->used || owner || sRuntimeOwner)status=B_BUSY;
		else {
			n->used=true;
			if(sDeviceManager==NULL)status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
			tt_pioneer_profile p={};if(status==B_OK)status=GetPioneerPciProfile(sDeviceManager,sCard,p);p.status=status;
			if(status==B_OK && !PioneerMinimumBarLayoutMatches(p))status=B_BAD_VALUE;
			NativeOwner* o=status==B_OK?new(std::nothrow) NativeOwner(n,r.bits,p):NULL;
			if(status==B_OK && !o)status=B_NO_MEMORY;
			if(o) {
				status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
				if(status!=B_OK)delete o;
				else {
					o->reserved=true;owner=o;sRuntimeOwner=n->base;
					bool mapped=BlackholePioneerMmioBegin(o->io,o->mmio,o->transaction);
					bool baseline=mapped && o->io.PioneerComputeNocIO::AfterNodeId(o->mmio.node_id[0]);
					if(baseline)o->lease.Run<NativeIO,PioneerNativeBfpGeneratedImage>(o->io,o->dram,r.bits);
					if(!o->io.BaselineClean() || o->lease.Result().state==PIONEER_NATIVE_BFP_UNKNOWN)Fault(*o);
					else if(!baseline || o->lease.Result().state!=PIONEER_NATIVE_BFP_RESULT) {
						if(!o->transaction.config_attempted || FinishMmio(*o)) {
							r.result.state=PIONEER_NATIVE_BFP_REFUSED;DisposeKnown(o);o=NULL;
						} else Fault(*o);
					}
					if(o)Reply(r,*o);
					status=user_memcpy(data,&r,sizeof(r));if(status!=B_OK && o)Fault(*o);
				}
			}
		}
	} else {
		if(!owner || owner->cookie!=n || owner->bits!=r.bits || owner->lease.Result().state!=PIONEER_NATIVE_BFP_RESULT)status=B_BAD_VALUE;
		else {
			auto* o=owner;
			if(!o->lease.End(o->io,o->dram) || !FinishMmio(*o))Fault(*o);
			Reply(r,*o);if(!r.result.cold)DisposeKnown(o);
			status=user_memcpy(data,&r,sizeof(r));
		}
	}
	release_sem(sLock);return status;
}
}
device_hooks* find_device(const char* name) {
	static device_hooks hooks={Open,ProbeClose,Free,Control,NULL,NULL,NULL,NULL,NULL,NULL};
	return name && !strcmp(name,TT_PCI_PROBE_DEVICE)?&hooks:NULL;
}
