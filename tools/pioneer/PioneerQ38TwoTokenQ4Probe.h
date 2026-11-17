/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWO_TOKEN_Q4_PROBE_H
#define PIONEER_Q38_TWO_TOKEN_Q4_PROBE_H
#if !defined(TT_PIONEER_EXPERIMENTAL_Q38_STREAM) || !defined(TT_PIONEER_EXPERIMENTAL_Q38_BLOCK) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Q8T4 needs a separate contained full-K diagnostic
#endif
#include "PioneerQ38TwoTokenQ4StreamLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38StreamProtocol.h"

// Private diagnostic policy; not an ioctl, user mapping, or card permission.
// The eventual caller owns the same exclusive reserved worker, resident bank
// bitmap, and proven MMIO maps throughout Prepare, Run and Close. It must
// validate the request before Prepare, close the panel and resident leases
// before returning any output, and quarantine rather than close on UNKNOWN.
namespace PioneerQ38TwoTokenQ4Probe {
namespace L=PioneerQ38TwoTokenQ4StreamLayout;
enum : unsigned {Boot=0x0006c06f,Done=0x7200c,Park=0x72038,OutputWords=32,
	Poison=0x7fc00000,TextBytes=7784};
static_assert(TT_Q38_PANEL_VIEW_BASE==L::SavedBase &&
	TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES==L::SavedEnd,
	"paired scratch is entirely in the original saved worker view");

struct Job {
	uint32_t inputs=0,bank=0;
	uint64_t offset=0;
	uint32_t activation[2][L::WidthMax/2]={},scale[2][L::WidthMax/256]={};
	tt_q38_panel_result result={};
	uint64_t elapsed_us=0;
	uint32_t output[OutputWords]={};
};
struct Lease { PioneerQ38PanelLease panel; };

static inline bool Authorize(const PioneerResidentBanks& banks,const tt_dram_samples_result& dram,
	const uint8_t* initialized,unsigned pages,const Job& job,PioneerQ38StreamExtent& out)
{
	if (!initialized || pages!=PioneerResidentBanks::Pages || (job.inputs!=5120 && job.inputs!=6144
		&& job.inputs!=L::WidthMax) || job.bank>=7 || (job.offset&4095)
		|| job.offset>=PioneerResidentBanks::Capacity) return false;
	PioneerQ38StreamExtent e;
	e.blocks=job.inputs/256;e.bytes=e.blocks*L::TileBytes;e.pages=(e.bytes+4095)/4096;e.flits=e.bytes/64;
	if (e.bytes>PioneerResidentBanks::Capacity-job.offset || !banks.Page(dram,job.bank,job.offset,e.address,e.route,e.index)
		|| e.route!=PioneerQ38ProjectionRoute(job.bank) || e.index>=pages || e.pages>pages-e.index) return false;
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	if (!banks.ImmutableRange(dram,job.bank,job.offset,e.bytes)) return false;
#endif
	for (unsigned p=0;p<e.pages;p++) if (initialized[e.index+p]!=1) return false;
	for (unsigned t=0;t<2;t++) {
		for (unsigned w=0;w<L::WidthMax/2;w++) {
			const uint32_t v=job.activation[t][w];
			if (w<job.inputs/2 ? (!PioneerInt8OperandValid(uint16_t(v))
				|| !PioneerInt8OperandValid(uint16_t(v>>16))) : v!=0) return false;
		}
		for (unsigned b=0;b<L::WidthMax/256;b++) {
			const uint32_t bits=job.scale[t][b],magnitude=bits&0x7fffffff;
			if (b<e.blocks ? magnitude && (magnitude<0x2f800000 || magnitude>0x47800000) : bits!=0) return false;
		}
	}
	out=e;return true;
}
template<class Image> static uint32_t Code(uint32_t at)
{
	static_assert(Image::Bytes==TextBytes && Image::BootWord==Boot && Image::Marker==L::VectorMagic,
		"only the byte-pinned Q8T4 image");
	return at>=0x72000 && at<0x72000+TextBytes?Image::Word((at-0x72000)/4):0;
}
template<class Image,class IO> static bool CodeMatches(IO& io)
{
	if (io.RuntimeCodeRead(0)!=Boot) return false;
	for (unsigned a=0x70000;a<0x74000;a+=4)
		if (PioneerQ38PanelImmutable(a) && io.Q38PanelRead(a)!=Code<Image>(a)) return false;
	return true;
}
template<class Image,class IO> bool Prepare(IO& io,const tt_dram_samples_result& dram,
	const tt_compute_noc_result& noc,PioneerResidentLease& store,const PioneerResidentBanks& banks,Lease& l)
{
	auto& p=l.panel;
	if (p.saved || p.unsafe || !PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
	auto* saved=io.Q38PanelSavedWords();
	if (!saved || (io.RuntimeControlRead(0)&1) || io.RuntimeControlRead(1) || io.RuntimeControlRead(2)) return false;
	saved[0]=io.RuntimeCodeRead(0);
	for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) saved[1+w]=io.Q38PanelRead(L::SavedBase+w*4);
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks) || io.RuntimeCodeRead(0)!=saved[0]) return false;
	for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) if (io.Q38PanelRead(L::SavedBase+w*4)!=saved[1+w]) return false;
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
	p.saved=p.dirty=true;
	for (unsigned a=0x70000;a<0x74000;a+=4) io.Q38PanelWrite(a,Code<Image>(a));
	io.RuntimeCodeWrite(0,Boot); // stage boot after all code words
	return p.ready=CodeMatches<Image>(io);
}
static inline void Command(uint32_t (&cmd)[10],const PioneerQ38StreamExtent& e,bool complete)
{
	const uint32_t values[]={e.address+(complete?(e.blocks-1)*L::TileBytes:0),0,e.route,
		L::Tile,0,L::Self,0,0,L::TileBytes,0};
	for (unsigned i=0;i<10;i++) cmd[i]=values[i];
}
template<class IO> static bool CommandMatches(IO& io,const PioneerQ38StreamExtent& e,bool complete)
{
	uint32_t expected[10];Command(expected,e,complete);
	if (!BlackholeResidentIdle(io)) return false;
	for (unsigned i=0;i<10;i++) if (io.ResidentNocRead(i*4)!=expected[i]) return false;
	return true;
}
template<class Image,class IO> bool Run(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,const PioneerResidentBanks& banks,Lease& l,const uint8_t* initialized,unsigned pages,Job& job)
{
	auto& p=l.panel;auto& r=job.result;r={};r.version=1;job.elapsed_us=0;
	PioneerQ38StreamExtent e;
	if (!p.ready || p.unsafe || p.running || !Authorize(banks,dram,initialized,pages,job,e)
		|| !PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
	r.state=TT_RUNTIME_CONFIG_FAILED;r.prepared=1;
	if (!CodeMatches<Image>(io)) return false;
	const uint32_t descriptor[L::Words]={L::VectorMagic,12,L::TileBytes,0,0,job.inputs,e.address,job.bank};
	uint32_t expected[10],result[OutputWords];Command(expected,e,false);
	if (!L::Shape<L::VectorMagic>(descriptor)
		|| !PioneerQ38PanelStage(io,L::Descriptor,descriptor,L::Words)) return false;
	for (unsigned t=0;t<2;t++) if (!PioneerQ38PanelStage(io,L::Scale(t),job.scale[t],e.blocks)
		|| !PioneerQ38PanelStage(io,L::VectorInput(t),job.activation[t],job.inputs/2)) return false;
	for (unsigned i=0;i<OutputWords;i++) io.Q38PanelWrite(L::Output+i*4,Poison);
	for (unsigned i=0;i<OutputWords;i++) if (io.Q38PanelRead(L::Output+i*4)!=Poison) return false;
	if (!BlackholeHostWriteCommand(io,expected) || !CommandMatches(io,e,false)) return false;
	store.l1_dirty=true;store.l1_restored=false;
	for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
	for (unsigned w=0;w<3;w++) if (io.RuntimeStatusRead(w)) return false;

	auto unsafe=[&]() {p.unsafe=true;store.recovery_required=true;r.recovery_required=1;return false;};
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) return unsafe();
	r.context_reset=1;io.RuntimeResetWrite(0x87ffff);io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff
		|| !PioneerQ38PanelSources(io,dram,noc,store,banks) || !CommandMatches(io,e,false)) return unsafe();
	for (unsigned i=0;i<8;i++) r.before[i]=io.ResidentNocRead(L::Counter(i));
	const uint64_t began=io.Q38PairNowUs();
	r.started=1;p.running=true;r.state=TT_RUNTIME_COMPLETION_FAILED;io.RuntimeResetWrite(0x46800);
	uint64_t ended=0;
	for (unsigned attempt=0;attempt<150000;attempt++) {
		r.polls++;if (io.ResetRead()!=0x46800) return unsafe();
		r.marker=io.RuntimeStatusRead(2);
		if (r.marker) {
			ended=io.Q38PairNowUs();r.phase=io.RuntimeStatusRead(0);r.panels=io.RuntimeStatusRead(1);
			if (r.marker!=L::VectorMagic || r.phase!=Done || r.panels!=job.inputs/16) return unsafe();
			io.Pause();r.pc[0]=io.RuntimePcRead();io.Pause();r.pc[1]=io.RuntimePcRead();
			if (r.pc[0]!=Park || r.pc[1]!=Park) return unsafe();
			r.completed=1;break;
		}
		io.Pause();
	}
	if (!r.completed || ended<=began || io.ResetRead()!=0x46800 || !PioneerQ38PanelSources(io,dram,noc,store,banks)
		|| !CodeMatches<Image>(io) || !CommandMatches(io,e,true)) return unsafe();
	for (unsigned i=0;i<8;i++) {
		r.after[i]=io.ResidentNocRead(L::Counter(i));
		if (uint32_t(r.after[i]-r.before[i])!=L::Delta(i)*e.blocks) return unsafe();
	}
	for (unsigned i=0;i<OutputWords;i++) {
		result[i]=io.Q38PanelRead(L::Output+i*4);
		if ((result[i]&0x7f800000)==0x7f800000) return unsafe();
	}
	if (io.ResetRead()!=0x46800 || io.RuntimeStatusRead(2)!=L::VectorMagic
		|| io.RuntimeStatusRead(0)!=Done || io.RuntimeStatusRead(1)!=job.inputs/16
		|| !PioneerQ38PanelSources(io,dram,noc,store,banks) || !CommandMatches(io,e,true)) return unsafe();
	for (unsigned i=0;i<8;i++) if (io.ResidentNocRead(L::Counter(i))!=r.after[i]) return unsafe();
	io.RuntimeResetWrite(0x87ffff);io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) return unsafe();
	io.RuntimeResetWrite(0x47800);io.Pause();
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) return unsafe();
	r.stopped=1;p.running=false;r.state=TT_RUNTIME_PASSED;job.elapsed_us=ended-began;
	for (unsigned i=0;i<OutputWords;i++) job.output[i]=result[i];
	return true;
}
template<class IO> bool Close(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,const PioneerResidentBanks& banks,Lease& l)
{
	return PioneerQ38PanelClose(io,dram,noc,store,banks,l.panel,true);
}
}
#endif
