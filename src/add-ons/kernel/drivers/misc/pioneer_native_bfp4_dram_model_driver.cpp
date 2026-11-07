/* SPDX-License-Identifier: MIT */
// Separate fixed BFP4 GDDR->native FPU diagnostic120. No model/Work endpoints.
#include <Drivers.h>
#if !defined(PIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC) || PIONEER_NATIVE_BFP4_DRAM_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) \
 || (defined(PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC) && (PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC != 1 || PIONEER_DRIVER_BUILD_TAG != 0x2026110700010121ULL)) \
 || (!defined(PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC) && PIONEER_DRIVER_BUILD_TAG != 0x2026110600010120ULL)
#error Build only the exact BFP4 GDDR diagnostic120 reload role
#endif
#if defined(PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC) || defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64) || defined(PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64) || defined(PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC) || defined(PIONEER_NATIVE_BFP_DIAGNOSTIC) || defined(PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC) || defined(PIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC) || defined(PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC) || defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC) || defined(PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Diagnostic120 cannot alias other contexts,Work or CPU reference
#endif
#define PIONEER_NATIVE_BFP_DIAGNOSTIC 1 // PRIVATE inert-base hook ONLY
#define find_device PioneerNativeDramBandLegacyFindDevice
#include "tenstorrent_pci_probe.cpp"
#undef find_device
#undef PIONEER_NATIVE_BFP_DIAGNOSTIC
#include "PioneerNativeDramModelBandNative.inc"
#define PioneerNativeDram81 PioneerNativeBfp4Dram120
#define PioneerNativeDramModelBandRequest PioneerNativeBfp4DramRequest
#define PioneerNativeDramModelBandRequestValid PioneerNativeBfp4DramRequestValid
#define PioneerNativeDramModelBandNone PioneerNativeBfp4DramNone
#define PioneerNativeDramModelBandGeneratedImage PioneerNativeBfp4DramModelGeneratedImage
#define PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT PIONEER_NATIVE_BFP4_DRAM_SUPPORT
#define PIONEER_NATIVE_DRAM_MODEL_BAND_RUN PIONEER_NATIVE_BFP4_DRAM_RUN
#define PIONEER_NATIVE_DRAM_MODEL_BAND_END PIONEER_NATIVE_BFP4_DRAM_END
namespace PioneerNativeDram81 {
static void Reply(PioneerNativeDramModelBandRequest& r,const Owner& o) {
	r.result=o.lease.Result();r.nonce=1;
	if(o.retained || sColdRecoveryRequired){r.result.compute.state=PIONEER_NATIVE_BFP_UNKNOWN;r.result.compute.cold=1;}
}
static bool DisposeKnown(Owner* o) {
	if(!o || !o->Scope() || o->retained || !o->reserved || (o->transaction.config_attempted && (!o->viewsClosed || !o->mapping.restored)))return false;
	if(o->pci->unreserve_device(o->card.bus,o->card.device,o->card.function,TT_PCI_PROBE_DEVICE,&sCookie)!=B_OK){o->Retain();return false;}
	o->reserved=false;sRuntimeOwner=NULL;current=NULL;delete o;return true;
}
static bool Finish(Owner& o) {
	const auto& r=o.lease.Result();
	if(o.closeAttempted || !o.Scope() || o.retained || o.clockEntered || r.compute.state!=PIONEER_NATIVE_BFP_CLOSED
		|| !r.compute.l1_restored || !r.compute.tlb_restored || !r.gddr_restored || !r.niu_restored || !r.clock_restored)return false;
	o.closeAttempted=true;o.closing=true;o.ready=false;
	return o.CloseViews() && o.ClosePci();
}
static status_t Open(const char* name,uint32 flags,void** value) {
	auto* n=new(std::nothrow) Cookie;if(!n)return B_NO_MEMORY;
	void* original=NULL;const status_t status=ProbeOpen(name,flags,&original);
	if(status!=B_OK){delete n;return status;}n->base=static_cast<PioneerProbeCookie*>(original);*value=n;return B_OK;
}
static status_t Free(void* value) {
	auto* n=static_cast<Cookie*>(value);if(!n || !n->base)return B_BAD_VALUE;
	status_t status=acquire_sem(sLock);if(status!=B_OK)return status;
	if(current && current->cookie==n){current->Retain();if(sOpenCookies)sOpenCookies--;release_sem(sLock);return B_OK;}
	release_sem(sLock);status=ProbeFree(n->base);if(status==B_OK)delete n;return status;
}
static status_t Control(void* value,uint32 operation,void* data,size_t length) {
	auto* n=static_cast<Cookie*>(value);
	if(geteuid()!=0 || !n || !n->base || n->base->magic!=0x50313030 || n->base->context)return B_NOT_ALLOWED;
	thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
	if(status!=B_OK || thread.team!=n->base->team)return status!=B_OK?status:B_NOT_ALLOWED;
	if(operation==TT_PCI_PROBE_RELOAD_STATUS || operation==TT_PCI_PROBE_RELOAD_QUIESCE)return DriverReloadControl(n->base,operation,data,length);
	if(operation!=PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT && operation!=PIONEER_NATIVE_DRAM_MODEL_BAND_RUN && operation!=PIONEER_NATIVE_DRAM_MODEL_BAND_END)return B_NOT_ALLOWED;
	if(!data || length!=sizeof(PioneerNativeDramModelBandRequest))return B_BAD_VALUE;
	PioneerNativeDramModelBandRequest r={};status=user_memcpy(&r,data,sizeof(r));
	if(status!=B_OK || !PioneerNativeDramModelBandRequestValid(r))return status!=B_OK?status:B_BAD_VALUE;
	if(operation==PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT) {
		if(r.sample!=PioneerNativeDramModelBandNone || r.nonce || r.accept)return B_BAD_VALUE;
		status=acquire_sem(sLock);if(status!=B_OK)return status;
		if(r.expected_generation!=sReloadGeneration)status=B_BAD_VALUE;
		else {r.result.compute.state=sColdRecoveryRequired?PIONEER_NATIVE_BFP_UNKNOWN:PIONEER_NATIVE_BFP_EMPTY;r.result.compute.cold=sColdRecoveryRequired;status=user_memcpy(data,&r,sizeof(r));}
		release_sem(sLock);return status;
	}
	if(r.sample>=3 || (operation==PIONEER_NATIVE_DRAM_MODEL_BAND_RUN?(r.nonce || r.accept):(r.nonce!=1 || r.accept!=1)))return B_BAD_VALUE;
	status=AcquireProbeLock(n->base);if(status!=B_OK)return status;
	if(r.expected_generation!=sReloadGeneration || !sReloadGuard || sReloadGuard->version!=1 || sOpenCookies!=1)status=B_BUSY;
	else if(operation==PIONEER_NATIVE_DRAM_MODEL_BAND_RUN) {
		if(n->used || current || sRuntimeOwner)status=B_BUSY;
		else {
			n->used=true;auto* o=new(std::nothrow) Owner(n,thread,r.sample);
			if(!o)status=B_NO_MEMORY;
			else {
				status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
				if(status!=B_OK)delete o;
				else {
					o->reserved=true;current=o;sRuntimeOwner=n->base;
					if(!sDeviceManager)status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
					const bool profile=status==B_OK && o->Scope() && GetPioneerPciProfile(sDeviceManager,sCard,o->profile)==B_OK && PioneerMinimumBarLayoutMatches(o->profile);
					const bool begun=profile && BlackholePioneerMmioBegin(o->io,o->mapping,o->transaction);
					const bool proved=begun && o->Views() && o->io.PioneerTensixIO::AfterNodeId(o->mapping.node_id[0])
						&& BlackholeDramSweepRoutes(o->io,o->telemetry,o->tensix,o->dram) && BlackholeComputeNocProbe(o->io,o->dram,o->noc,2);
					o->ready=proved && o->Scope() && o->Views() && o->Identity(true);
					const bool ran=o->ready && o->lease.Run<IO,PioneerNativeDramModelBandGeneratedImage>(o->io,o->dram,r.sample);
					if(!ran) {
						if(!o->transaction.config_attempted && !o->retained) {
							if(DisposeKnown(o)){r.result.compute.state=PIONEER_NATIVE_BFP_REFUSED;o=NULL;}
							else o->Retain();
						} else o->Retain();
					}
					if(o)Reply(r,*o);
					status=user_memcpy(data,&r,sizeof(r));if(status!=B_OK && o)o->Retain();
				}
			}
		}
	} else {
		if(!current || current->cookie!=n || current->sample!=r.sample || current->lease.Result().compute.state!=PIONEER_NATIVE_BFP_RESULT)status=B_BAD_VALUE;
		else {
			auto* o=current;
			if(!o->lease.End<IO,PioneerNativeDramModelBandGeneratedImage>(o->io) || !Finish(*o))o->Retain();
			Reply(r,*o);
			if(!r.result.compute.cold && !DisposeKnown(o)){o->Retain();Reply(r,*o);}
			status=user_memcpy(data,&r,sizeof(r));
		}
	}
	release_sem(sLock);return status;
}
} // namespace PioneerNativeBfp4Dram120
device_hooks* find_device(const char* name) {
	static device_hooks hooks={PioneerNativeDram81::Open,ProbeClose,PioneerNativeDram81::Free,PioneerNativeDram81::Control,NULL,NULL,NULL,NULL,NULL,NULL};
	return name && !strcmp(name,TT_PCI_PROBE_DEVICE)?&hooks:NULL;
}
