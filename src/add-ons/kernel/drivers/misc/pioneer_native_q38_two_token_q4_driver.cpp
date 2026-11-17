/* SPDX-License-Identifier: MIT */
// Separate tag215 diagnostic. No Work or legacy compute endpoint.
#if !defined(_KERNEL_MODE) || !defined(PIONEER_NATIVE_Q38_TWO_TOKEN_Q4) \
 || PIONEER_NATIVE_Q38_TWO_TOKEN_Q4 != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) \
 || PIONEER_DRIVER_BUILD_TAG != 0x2026111700010215ULL
#error Build only the exact Q8T4 tag215 reload role
#endif
#if defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__) \
 || defined(PIONEER_NATIVE_BFP_DIAGNOSTIC) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_DISPATCH_ROOT_INFERENCE) || defined(PIONEER_Q38_GDN_MUTABLE_STATE)
#error Q8T4 tag215 cannot alias inference, mutable, or old diagnostic roles
#endif

#include <Drivers.h>
#define PIONEER_NATIVE_BFP_DIAGNOSTIC 1 // Inert legacy find_device only.
#define find_device PioneerQ38PairLegacyFindDevice
#include "tenstorrent_pci_probe.cpp"
#undef find_device
#undef PIONEER_NATIVE_BFP_DIAGNOSTIC
#include "../../../../../tools/pioneer/PioneerQ38TwoTokenQ4NativeProtocol.h"
#include "PioneerQ38TwoTokenQ4GeneratedImage.h"

namespace PioneerQ38TwoTokenQ4Native {
using Image=PioneerQ38TwoTokenQ4GeneratedImage;
static_assert(Tag==PIONEER_DRIVER_BUILD_TAG,"exact reload tag");

static void Retain(PioneerStoreContext& c)
{
	// Memory-only. There must be no legacy END/reset path after UNKNOWN.
	c.banks.Quarantine();c.streamProbe.unsafe=true;c.lease.recovery_required=true;
	c.io.QuarantineHostPage();sColdRecoveryRequired=true;
}

static status_t Free(void* value)
{
	auto* fd=static_cast<PioneerProbeCookie*>(value);
	if (!fd || fd->magic!=0x50313030) return B_BAD_VALUE;
	status_t locked=acquire_sem(sLock);
	if (locked!=B_OK) return locked;
	if (fd->context) {
		Retain(*fd->context); // FD close is NEVER an implicit device END.
		if (sOpenCookies) sOpenCookies--;
		release_sem(sLock);return B_OK; // retain FD/context and OS ownership
	}
	release_sem(sLock);return ProbeFree(fd);
}

static status_t Compare(PioneerProbeCookie* fd,void* data,size_t length)
{
	if (!data || length!=sizeof(Request)) return B_BAD_VALUE;
	auto* request=new(std::nothrow) Request;
	auto* control=new(std::nothrow) Q::Job;
	if (!request || !control) {delete request;delete control;return B_NO_MEMORY;}
	status_t status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=Version || request->tag!=Tag
		|| !request->generation || request->reserved || request->padding) {
		delete request;delete control;return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(fd,true);
	if (status!=B_OK) {delete request;delete control;return status;}
	auto* c=fd->context;
	if (!c || sRuntimeOwner!=fd || !c->bankStorageOnly || sOpenCookies!=1
		|| request->generation!=sReloadGeneration || c->Unsafe() || c->panelProbe.saved || c->streamProbe.saved
		|| c->bytes!=PioneerResidentBanks::Total) {
		release_sem(sLock);delete request;delete control;return B_NOT_ALLOWED;
	}
	auto& dram=c->baseline.baseline.baseline.baseline.dram;
	auto& noc=c->baseline.baseline.baseline.noc;
	const unsigned pages=unsigned(c->bytes/4096);
	Q::Job& job=request->job;
	PioneerQ38StreamExtent extent;
	if (!Q::Authorize(c->banks,dram,c->initialized,pages,job,extent)) {
		release_sem(sLock);delete request;delete control;return B_ENTRY_NOT_FOUND;
	}
	for (auto& v:request->micros) v=0;
	for (auto& v:request->arms) v={};
	for (auto& v:job.output) v=0;
	job.result={};job.elapsed_us=0;request->cleanup={};
	request->compared=request->matched=request->cold=0;
	Q::Lease lease;
	uint32_t reference[Q::OutputWords]={};
	auto now=[]() {return uint64_t(system_time());};
	bool ready=c->lease.ready;
	if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
	bool okay=ready && c->io.SelectRuntimeCore(0) && c->io.MapQ38Panel()
		&& Q::Prepare<Image>(c->io,dram,noc,c->lease,c->banks,lease);
	control->inputs=job.inputs;control->bank=job.bank;control->offset=job.offset;
	const uint32_t zero=PioneerInt8Operand(0);
	for (unsigned w=0;w<job.inputs/2;w++) control->activation[1][w]=zero|(zero<<16);
	for (unsigned t=0;t<Arms && okay;t++) {
		if (t!=Paired) {
			for (unsigned w=0;w<job.inputs/2;w++) control->activation[0][w]=job.activation[t][w];
			for (unsigned b=0;b<extent.blocks;b++) control->scale[0][b]=job.scale[t][b];
		}
		Q::Job& input=t==Paired?job:*control;
		okay=Q::Run<Image>(c->io,dram,noc,c->lease,c->banks,lease,c->initialized,pages,input,now);
		request->arms[t]=input.result;
		request->micros[t]=input.elapsed_us;
		if (okay && t!=Paired) for (unsigned n=0;n<16;n++) {
			reference[t*16+n]=input.output[n];okay &= input.output[n+16]==0;
		}
	}
	if (okay) {
		request->compared=1;
		bool match=true;
		for (unsigned i=0;i<Q::OutputWords;i++) match &= job.output[i]==reference[i];
		request->matched=match;
	}
	if (lease.panel.unsafe || c->Unsafe()) Retain(*c);
	else if (!Q::Close(c->io,dram,noc,c->lease,c->banks,lease)) Retain(*c);
	else EndResidentSession(fd,request->cleanup,true); // explicit, verified END before reply
	// EndResidentSession may have retired c; do not dereference it here.
	if (sColdRecoveryRequired || !Closed(request->cleanup) || fd->context || sRuntimeOwner) {
		request->cold=1;
		if (fd->context) Retain(*fd->context);
	}
	if (request->cold || !request->matched) {
		for (auto& v:request->micros) v=0;
		job.elapsed_us=0;
		for (auto& v:job.output) v=0;
	}
	status=user_memcpy(data,request,sizeof(*request));
	release_sem(sLock);delete request;delete control;return status;
}

static status_t Control(void* value,uint32 operation,void* data,size_t length)
{
	auto* fd=static_cast<PioneerProbeCookie*>(value);
	if (geteuid()!=0 || !fd || fd->magic!=0x50313030) return B_NOT_ALLOWED;
	thread_info thread={};
	const status_t info=get_thread_info(find_thread(NULL),&thread);
	if (info!=B_OK || thread.team!=fd->team) return info!=B_OK?info:B_NOT_ALLOWED;
	switch (operation) {
		case TT_PCI_PROBE_RELOAD_STATUS:case TT_PCI_PROBE_RELOAD_QUIESCE:
			return DriverReloadControl(fd,operation,data,length);
		case TT_PCI_PROBE_Q38_BANK_SUPPORT:case TT_PCI_PROBE_Q38_BANK_BEGIN:
		case TT_PCI_PROBE_Q38_BANK_UPLOAD:case TT_PCI_PROBE_Q38_BANK_END:
			return Q38BankControl(fd,operation,data,length);
		case CompareOperation:return Compare(fd,data,length);
		default:return B_NOT_ALLOWED;
	}
}
}

device_hooks* find_device(const char* name)
{
	using namespace PioneerQ38TwoTokenQ4Native;
	static device_hooks hooks={ProbeOpen,ProbeClose,Free,Control,NULL,NULL,NULL,NULL,NULL,NULL};
	return strcmp(name,TT_PCI_PROBE_DEVICE)==0?&hooks:NULL;
}
