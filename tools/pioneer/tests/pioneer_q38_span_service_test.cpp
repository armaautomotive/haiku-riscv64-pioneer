/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_SPAN_SERVICE_MOCK_ONLY
#include "pioneer_q38_span_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanService.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchBootstrap.h"
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StateZeroController.h"
#endif
using status_t=int; using uint32=uint32_t;
constexpr status_t B_OK=0,B_DEV_NOT_READY=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,B_BAD_VALUE=-4,B_NO_MEMORY=-5,B_ENTRY_NOT_FOUND=-6;
struct thread_info { int team; };
static int team=17,threadStatus=0;
static int find_thread(void*) { return 1; }
static int get_thread_info(int,thread_info* info) { info->team=team; return threadStatus; }
struct SpanRef1 { tt_dram_samples_result& dram; };
struct SpanRef2 { SpanRef1 baseline; tt_compute_noc_result& noc; };
struct SpanRef3 { SpanRef2 baseline; };
struct SpanRef4 { SpanRef3 baseline; };
struct NativeSpanContext {
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	PioneerQ38GdnBanks* q38GdnBanks=nullptr;
	PioneerQ38StateZeroLease* q38StateZero=nullptr;
#endif
	SpanControlFixture fixture;
	SpanControlIO& io;
	PioneerResidentBanks& banks;
	tt_host_page_result& host;
	PioneerResidentLease& lease;
	PioneerBulkUploadLease& bulk;
	PioneerOutputBufferLease& bulkHost;
	PioneerWideOutputLease& wideHost;
	PioneerQ38SpanLease* q38Span=nullptr;
	PioneerQ38DispatchLease* q38Dispatch=nullptr;
	void* q38Cohort=nullptr; void* fleet=nullptr;
	struct { bool saved=false; } panelProbe,streamProbe;
	int runtimeCoreMode=-1;
	bool bankStorageOnly=true,dispatchOwnerRetained=false;
	uint64_t bytes=PioneerResidentBanks::Total;
	std::vector<uint32_t> hashWords=std::vector<uint32_t>(PioneerResidentBanks::Pages,0);
	uint8_t* initialized; uint32_t* hashes=hashWords.data(); uint32_t uploadedPages=0;
	SpanRef4 baseline;
	explicit NativeSpanContext(const char* image):fixture(image),io(fixture.io),banks(fixture.owner.banks),host(fixture.owner.host),
		lease(fixture.owner.store),bulk(fixture.bulk),bulkHost(fixture.transfer),wideHost(fixture.wide),initialized(fixture.initialized.data()),
		baseline{{{{fixture.owner.dram},fixture.owner.noc}}} {}
	bool Unsafe() { return banks.Quarantined() || lease.recovery_required || bulk.unsafe || wideHost.quarantined
		|| bulkHost.quarantined || (q38Span && (q38Span->unsafe || q38Span->running)); }
	~NativeSpanContext() { delete q38Span; }
};
using PioneerStoreContext=NativeSpanContext;
struct PioneerProbeCookie { uint32_t magic=0x50313030; int team=17; NativeSpanContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=false,sColdRecoveryRequired=false;
static int sLock=1,acquired=0,released=0,copyCalls=0,failCopy=0;
static status_t acquire_sem(int lock) { assert(lock==1); acquired++; return B_OK; }
static void release_sem(int lock) { assert(lock==1); released++; }
#include "pioneer_lock.inc"
static status_t user_memcpy(void* to,const void* from,size_t bytes) {
	if (++copyCalls==failCopy) return -99; memcpy(to,from,bytes); return B_OK;
}
#include "pioneer_retain_memory.inc"
#include "pioneer_span_control.inc"
static std::unique_ptr<tt_q38_span_upload> Request(unsigned bytes=4096,unsigned bank=0,uint64_t offset=0)
{
	auto r=std::make_unique<tt_q38_span_upload>(); r->version=1; r->bank=bank; r->offset=offset; r->bytes=bytes;
	for (unsigned w=0;w<bytes/4;w++) r->data[w]=0x7fc00000U+w*0x9e3779b9U;
	return r;
}
static void Bind(PioneerProbeCookie& cookie,NativeSpanContext& c)
{ cookie.context=&c; sRuntimeOwner=&cookie; sBankStorageOnly=true; }
static void Unbind() { sRuntimeOwner=nullptr; sBankStorageOnly=sColdRecoveryRequired=false; failCopy=0; }
int main(int argc,char** argv)
{
	assert(argc==2);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	PioneerProbeCookie cookie,foreign; tt_q38_span_support support={};
	assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_SUPPORT,&support,sizeof(support))==B_OK
		&& support.driver==55 && support.version==1 && support.banks==7 && support.max_bytes==1048576
		&& support.before_cohort==1 && support.explicit_end==1 && !support.reserved && !cookie.context);
	{
		NativeSpanContext c(argv[1]); Bind(cookie,c); unsigned touches=c.io.touches;
		auto request=Request();
		assert(Q38SpanControl(&foreign,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_BUSY);
		team=18; assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_NOT_ALLOWED); team=17;
		for (unsigned bad:{0U,1U,4097U,1048577U}) {
			request->bytes=bad; assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_BAD_VALUE);
		}
		request->bytes=4096; request->reserved=1;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_BAD_VALUE);
		request->reserved=0; c.initialized[0]=1;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_ENTRY_NOT_FOUND);
		c.initialized[0]=0;
		c.q38Cohort=&c; assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_NOT_ALLOWED); c.q38Cohort=nullptr;
		assert(c.io.touches==touches && !c.q38Span); Unbind();
	}
	{
		NativeSpanContext c(argv[1]); Bind(cookie,c);
		for (unsigned bank=0;bank<7;bank++) {
			auto request=Request(bank%2?4096:1048576,bank,PioneerResidentBanks::Capacity-(bank%2?4096:1048576));
			assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_OK);
			const auto& r=request->result;
			assert(r.state==TT_RUNTIME_PASSED && r.pages==request->bytes/4096 && r.host_exact==1 && r.held==1 && !r.recovery_required);
			uint32_t address=0,route=0,index=0;
			assert(c.banks.Page(c.fixture.owner.dram,bank,request->offset,address,route,index));
			for (unsigned p=0;p<r.pages;p++) assert(c.initialized[index+p]==1 && c.hashes[index+p]==PioneerResidentPageHash(request->data+p*1024));
			unsigned touches=c.io.touches;
			assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_ENTRY_NOT_FOUND && touches==c.io.touches);
		}
		tt_q38_span_end end={1,0,0,0};
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK && end.closed && !end.recovery_required);
		unsigned touches=c.io.touches; assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK && touches==c.io.touches);
		auto request=Request(); assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_NOT_ALLOWED && touches==c.io.touches);
		assert(!c.Unsafe() && !c.dispatchOwnerRetained); Unbind();
	}
	for (unsigned fault=0;fault<4;fault++) {
		NativeSpanContext c(argv[1]); Bind(cookie,c); auto request=Request();
		if (fault==0) c.io.packetFault=1;
		if (fault==1) c.io.summaryFault=7;
		if (fault==2) c.io.lateWideFault=0;
		if (fault==3) c.io.corruptCode=true;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_OK);
		assert(request->result.recovery_required && !request->result.pages && !request->result.host_exact
			&& !c.uploadedPages && !c.initialized[0] && c.Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired);
		unsigned touches=c.io.touches; tt_q38_span_end end={1,0,0,0};
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_DEV_NOT_READY);
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_DEV_NOT_READY && touches==c.io.touches);
		Unbind();
	}
	{
		NativeSpanContext c(argv[1]); Bind(cookie,c); auto request=Request(); unsigned touches=c.io.touches;
		failCopy=copyCalls+1;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==-99
			&& c.io.touches==touches && !c.q38Span);
		failCopy=copyCalls+2; // completed hardware transfer, failed delivery ONLY
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==-99
			&& c.q38Span && c.q38Span->ready && !c.q38Span->closed && c.uploadedPages==1
			&& !c.Unsafe() && sRuntimeOwner==&cookie);
		Unbind(); // real cookie-release/explicit-END gate tested separately
	}
	{
		NativeSpanContext c(argv[1]); Bind(cookie,c); auto request=Request();
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))==B_OK);
		c.io.dropBulk=c.io.bulkWrites+1; tt_q38_span_end end={1,0,0,0};
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK
			&& !end.closed && end.recovery_required && c.Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired);
		unsigned touches=c.io.touches;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_DEV_NOT_READY && touches==c.io.touches);
		Unbind();
	}
#ifdef PIONEER_Q38_SPAN_FAST
	{
		NativeSpanContext c(argv[1]); Bind(cookie,c);
		for (unsigned bank=0;bank<7;bank++) {
			auto request=Request(1048576,bank); unsigned loads=c.io.wideLoads;
			unsigned codeLoads=c.io.codeLoads; bool prepared=c.q38Span!=nullptr;
			assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK);
			assert(request->result.state==TT_RUNTIME_PASSED && !request->result.host_exact && !request->result.matched_words
				&& !request->result.recovery_required && request->result.pages==256 && loads==c.io.wideLoads);
			assert(c.io.lastDeviceWords.size()==262144);
			for (unsigned w=0;w<262144;w++) assert(c.io.lastDeviceWords[w]==request->data[w]);
#ifdef PIONEER_Q38_SPAN_REUSE_CODE
			if (prepared) assert(c.io.codeLoads==codeLoads); // not even ONE main-code read per Work job
#else
			if (prepared) assert(c.io.codeLoads==codeLoads+8192);
#endif
		}
		tt_q38_span_end end={1,0,0,0}; assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK && end.closed);
		Unbind();
	}
#ifdef PIONEER_Q38_SPAN_REUSE_CODE
	for (unsigned fault=0;fault<15;fault++) {
		NativeSpanContext c(argv[1]); Bind(cookie,c); auto request=Request();
		if (!fault) c.io.noCompletion=true;
		if (fault==1) c.io.corruptCounter=true;
		if (fault==2) c.io.sourceFault=11;
		if (fault==3) c.io.summaryFault=7;
		if (fault==4) c.io.descriptorFault=4;
		if (fault==5) c.io.commandFault=3;
		if (fault==6) c.io.othersHeld=false;
		if (fault==7) c.io.latePcFault=3;
		if (fault==8) c.io.lateMapLoss=true;
		if (fault==9) c.io.lateCohort=true;
		if (fault==10) c.io.authorityChanged=true;
		if (fault==11) c.io.packetFault=2;
		if (fault==12) c.io.dropReset=1;
		if (fault==13) c.io.dropPci=1;
		if (fault==14) c.io.stageLost=true;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK
			&& request->result.recovery_required && !c.uploadedPages && c.Unsafe()
			&& c.dispatchOwnerRetained && sColdRecoveryRequired);
		unsigned touches=c.io.touches; tt_q38_span_end end={1,0,0,0};
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_DEV_NOT_READY
			&& c.io.touches==touches); Unbind();
	}
	{
		NativeSpanContext c(argv[1]); Bind(cookie,c); auto request=Request();
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK);
		// Simulate an out-of-contract bit flip in UNUSED code padding AFTER
		// setup. Work reuses the immutable proof; strict MUST see it before
		// its first launch. END MUST detect it before ANY restoration.
		c.io.view[(0x73ffc-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
		request->offset=4096;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK
			&& request->result.state==TT_RUNTIME_PASSED && c.uploadedPages==2);
		unsigned loads=c.io.codeLoads,writes=c.io.panelWrites,bulk=c.io.bulkWrites;
		tt_q38_span_end end={1,0,0,0};
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK
			&& !end.closed && end.recovery_required && c.Unsafe() && c.dispatchOwnerRetained
			&& c.io.codeLoads>loads && c.io.panelWrites==writes && c.io.bulkWrites==bulk);
		unsigned touches=c.io.touches;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_DEV_NOT_READY
			&& c.io.touches==touches); Unbind();
	}
	for (bool strict:{false,true}) {
		NativeSpanContext c(argv[1]); Bind(cookie,c); auto request=Request();
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK);
		if (strict) c.io.view[(0x73ffc-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
		else c.io.entry^=1; // boot control remains live-checked EVEN in Work
		request->offset=4096; unsigned starts=c.io.rootStarts;
		assert(Q38SpanControl(&cookie,strict?TT_PCI_PROBE_Q38_SPAN_UPLOAD:TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,
			request.get(),sizeof(*request))==B_OK && request->result.recovery_required
			&& c.io.rootStarts==starts && c.Unsafe() && c.uploadedPages==1);
		Unbind();
	}
	puts("PASS Work immutable code proof reuse: ZERO per-job main-code reads, live boot checked, strict code checks unchanged, END ALWAYS full-audits before restoration and retains ALL on corruption; NO DEVICE IO");
#endif
	{
		NativeSpanContext c(argv[1]); Bind(cookie,c); auto request=Request(); c.io.deviceCorruptPacket=0;
		assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK);
		assert(request->result.state==TT_RUNTIME_PASSED && !request->result.host_exact && c.uploadedPages==1
			&& c.io.lastDeviceWords[0]!=request->data[0]); // Work intentionally trusts DMA numbers; debug must compare
		Unbind();
	}
	puts("PASS REAL WORK span handler: separate opcode, all7 banks/1MiB exact instruction-model transfers, ZERO CPU byte reads/comparison, strict mode preserved; deliberately well-formed wrong bytes accepted ONLY in Work; NO DEVICE IO");
#endif
	assert(acquired==released);
	puts("PASS REAL span handler+service+actual loader: logical bounds/team/FD/cohort isolation before IO, all7 banks/1MiB exact transport, atomic page/hash publication, explicit END/no second prepare, UNKNOWN retains ALL owners and refuses later IO; NO DEVICE IO");
}
