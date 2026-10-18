/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PANEL_PROBE_H
#define PIONEER_Q38_PANEL_PROBE_H
#include "PioneerRuntimeMatvec.h"
#include "PioneerResidentBanks.h"
#include "PioneerQ38PanelProbeImage.h"
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#include "PioneerQ38AffineProbeImage.h"
enum { TT_Q38_PANEL_CURRENT_DRIVER=47,TT_Q38_PANEL_ABI_VERSION=2 };
#else
enum { TT_Q38_PANEL_CURRENT_DRIVER=46,TT_Q38_PANEL_ABI_VERSION=1 };
#endif

#define TT_PCI_PROBE_Q38_PANEL_SUPPORT (B_DEVICE_OP_CODES_END + 0x54ec)
#define TT_PCI_PROBE_Q38_PANEL_RUN (B_DEVICE_OP_CODES_END + 0x54ed)
struct tt_q38_panel_support {
	uint32_t driver,version,local_only,rows,columns,type_mask;
};

// Separate experimental46 LOCAL arithmetic qualification, NOT a resident
// model runner. Caller supplies DATA only; kernel owns the one fixed core,
// immutable image, all addresses, saved L1 and start/completion/stop proof.
enum { TT_Q38_PANEL_VIEW_BASE=0x6e000,TT_Q38_PANEL_VIEW_BYTES=0x24000,
	TT_Q38_PANEL_VIEW_WORDS=TT_Q38_PANEL_VIEW_BYTES/4 };
struct tt_q38_panel_result {
	uint32_t version,state,prepared,started,completed,stopped,recovery_required,context_reset;
	uint32_t phase,panels,marker,pc[2],before[8],after[8],polls;
};
struct tt_q38_panel_probe {
	uint32_t version,type,bytes,panel,reserved[2];
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	uint32_t activation_scale; // bounded signed FP32,not an address
#endif
	uint32_t tile[848],activation[8];
	tt_q38_panel_result result;
	uint32_t dot[16],coefficient[16],minimum[16];
};
struct PioneerQ38PanelLease {
	bool saved=false,dirty=false,ready=false,running=false,unsafe=false;
};
static inline unsigned PioneerQ38PanelProbeBytes(unsigned type)
{
	return type==8?576:type==12?2304:type==14?3392:0;
}
static inline unsigned PioneerQ38PanelProbeByte(const uint32_t* tile,unsigned byte)
{
	return (tile[byte/4]>>((byte%4)*8))&255;
}
static inline bool PioneerQ38PanelProbeValid(const tt_q38_panel_probe& r)
{
	unsigned bytes=PioneerQ38PanelProbeBytes(r.type),payload=r.type==8?544:r.type==12?2304:3360;
	if (r.version!=TT_Q38_PANEL_ABI_VERSION || r.reserved[0] || r.reserved[1] || !bytes || r.bytes!=bytes || r.panel>=(r.type==8?2U:16U)) return false;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	uint32_t scale=r.activation_scale&0x7fffffff;
	if (scale && (scale<0x2f800000 || scale>0x47800000)) return false; // explicit LOCAL probe range2^-32..2^16
#endif
	for (unsigned w=0;w<8;w++) if (!PioneerInt8OperandValid(uint16_t(r.activation[w])) || !PioneerInt8OperandValid(uint16_t(r.activation[w]>>16))) return false;
	for (unsigned row=0;row<16;row++) {
		unsigned at=(r.type==14?208:0)*16+row;
		uint16_t scale=uint16_t(PioneerQ38PanelProbeByte(r.tile,at)|(PioneerQ38PanelProbeByte(r.tile,at+16)<<8));
		if ((scale&0x7c00)==0x7c00) return false;
		if (r.type==12) {
			scale=uint16_t(PioneerQ38PanelProbeByte(r.tile,32+row)|(PioneerQ38PanelProbeByte(r.tile,48+row)<<8));
			if ((scale&0x7c00)==0x7c00) return false;
		}
	}
	for (unsigned byte=payload;byte<3392;byte++) if (PioneerQ38PanelProbeByte(r.tile,byte)) return false;
	return true;
}
static inline bool PioneerQ38PanelImmutable(uint32_t address)
{
	return !(address>=0x70500 && address<0x7050c) && !(address>=0x70700 && address<0x70a20);
}
static inline uint32_t PioneerQ38PanelProgram(uint32_t address)
{
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	return address<0x71000?PioneerQ38AffineMainWord((address-0x70000)/4):address<0x72000?
		PioneerQ38AffineBridgeWord((address-0x71000)/4):PioneerQ38AffineUnpackWord((address-0x72000)/4);
#else
	return address<0x71000?PioneerQ38PanelMainWord((address-0x70000)/4):address<0x72000?
		PioneerQ38PanelBridgeWord((address-0x71000)/4):PioneerQ38PanelUnpackWord((address-0x72000)/4);
#endif
}
template<class IO>
bool PioneerQ38PanelSources(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	const PioneerResidentLease& store,const PioneerResidentBanks& banks)
{
	return store.ready && !store.pending && !store.recovery_required && banks.Matches(dram)
		&& io.Q38PanelMappingStable() && PioneerRuntimeSources(io,dram,noc,store) && BlackholeResidentIdle(io);
}
template<class IO>
bool PioneerQ38PanelHeld(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	const PioneerResidentLease& store,const PioneerResidentBanks& banks)
{
	return io.ResetRead()==0x47800 && io.ResetRead()==0x47800 && PioneerQ38PanelSources(io,dram,noc,store,banks);
}
template<class IO>
bool PioneerQ38PanelCodeMatches(IO& io)
{
	bool okay=io.RuntimeCodeRead(0)==TT_Q38_PANEL_BOOT;
	for (uint32_t a=0x70000;a<0x74000;a+=4) if (PioneerQ38PanelImmutable(a)) okay &= io.Q38PanelRead(a)==PioneerQ38PanelProgram(a);
	return okay;
}
template<class IO>
bool PioneerQ38PanelPrepare(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	const PioneerResidentLease& store,const PioneerResidentBanks& banks,PioneerQ38PanelLease& lease)
{
	if (lease.ready) return !lease.unsafe && !lease.running && PioneerQ38PanelHeld(io,dram,noc,store,banks) && PioneerQ38PanelCodeMatches(io);
	uint32_t* saved=io.Q38PanelSavedWords();
	if (!saved || lease.saved || lease.unsafe || !PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
	for (unsigned i=0;i<3;i++) {
		uint32_t value=io.RuntimeControlRead(i); if (i?(value!=0):(value&1)) return false;
	}
	saved[0]=io.RuntimeCodeRead(0);
	for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) saved[1+w]=io.Q38PanelRead(TT_Q38_PANEL_VIEW_BASE+w*4);
	lease.saved=true;
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) { lease.unsafe=true; return false; }
	lease.dirty=true;
	for (uint32_t a=0x70000;a<0x74000;a+=4) io.Q38PanelWrite(a,PioneerQ38PanelProgram(a));
	io.RuntimeCodeWrite(0,TT_Q38_PANEL_BOOT);
	lease.ready=PioneerQ38PanelCodeMatches(io); return lease.ready;
}
template<class IO>
bool PioneerQ38PanelStage(IO& io,uint32_t address,const uint32_t* data,unsigned words)
{
	for (unsigned w=0;w<words;w++) io.Q38PanelWrite(address+w*4,data[w]);
	bool okay=true; for (unsigned w=0;w<words;w++) okay &= io.Q38PanelRead(address+w*4)==data[w]; return okay;
}
static inline uint32_t PioneerQ38PanelCounter(unsigned i)
{
	return i<4?0x208+i*4:BlackholeHostWriteCounter(i-4);
}
template<class IO>
bool PioneerQ38PanelRun(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,const PioneerResidentBanks& banks,PioneerQ38PanelLease& lease,tt_q38_panel_probe& job)
{
	auto& r=job.result; r={}; r.version=1;
	if (!PioneerQ38PanelProbeValid(job) || !lease.ready || lease.unsafe || lease.running
		|| !PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
	r.state=TT_RUNTIME_CONFIG_FAILED; r.prepared=1;
	if (!PioneerQ38PanelCodeMatches(io)) return false;
	{
		const uint32_t matrix[]={0,1,1},descriptor[]={0x50383355,job.type,job.bytes,job.panel
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
			,job.activation_scale
#endif
		};
		if (!PioneerQ38PanelStage(io,0x70500,matrix,3) || !PioneerQ38PanelStage(io,0x74000,descriptor,sizeof(descriptor)/4)
			|| !PioneerQ38PanelStage(io,0x90000,job.tile,848) || !PioneerQ38PanelStage(io,0x80000,job.activation,8)) return false;
		for (unsigned w=0;w<160;w++) io.Q38PanelWrite(0x91000+w*4,0x7fc00000);
		for (unsigned w=0;w<16;w++) io.Q38PanelWrite(0x84000+w*4,0x80000000);
		for (unsigned w=0;w<160;w++) if (io.Q38PanelRead(0x91000+w*4)!=0x7fc00000) return false;
		for (unsigned w=0;w<16;w++) if (io.Q38PanelRead(0x84000+w*4)!=0x80000000) return false;
	}
	store.l1_dirty=true; store.l1_restored=false;
	for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
	for (unsigned w=0;w<3;w++) if (io.RuntimeStatusRead(w)) return false;
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) { lease.unsafe=true; goto finished; }
	r.context_reset=1; io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerQ38PanelSources(io,dram,noc,store,banks)) { lease.unsafe=true; goto finished; }
	for (unsigned i=0;i<8;i++) r.before[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
	r.started=1; lease.running=true; r.state=TT_RUNTIME_COMPLETION_FAILED;
	io.RuntimeResetWrite(0x46800);
	for (unsigned attempt=0;attempt<10000;attempt++) {
		r.polls++;
		if (io.ResetRead()!=0x46800) break;
		r.phase=io.RuntimeStatusRead(0); r.panels=io.RuntimeStatusRead(1); r.marker=io.RuntimeStatusRead(2);
		if (r.marker==0x46383350) break;
		if (r.phase==TT_Q38_PANEL_DONE && r.panels==1 && r.marker==0x4d415431) {
			io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
			if (r.pc[0]==TT_Q38_PANEL_PARK && r.pc[1]==r.pc[0]) { r.completed=1; break; }
		}
		io.Pause();
	}
	if (!r.completed || io.ResetRead()!=0x46800 || !PioneerQ38PanelSources(io,dram,noc,store,banks)
		|| !PioneerQ38PanelCodeMatches(io)) goto uncertain;
	for (unsigned i=0;i<8;i++) {
		r.after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
		if (r.after[i]!=r.before[i]) goto uncertain; // image MUST issue zero NoC requests
	}
	r.state=TT_RUNTIME_PASSED;
	for (unsigned row=0;row<16;row++) {
		job.dot[row]=io.Q38PanelRead(0x84000+row*4);
		job.coefficient[row]=io.Q38PanelRead(0x91200+row*4); job.minimum[row]=io.Q38PanelRead(0x91240+row*4);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
		if ((job.dot[row]&0x7f800000)==0x7f800000
#else
		if (int32_t(job.dot[row])<-262144 || int32_t(job.dot[row])>262144
#endif
			|| (job.coefficient[row]&0x7f800000)==0x7f800000 || (job.minimum[row]&0x7f800000)==0x7f800000) r.state=TT_RUNTIME_NONFINITE;
	}
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) goto uncertain;
	io.RuntimeResetWrite(0x47800); io.Pause();
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) goto uncertain;
	r.stopped=1; lease.running=false; goto finished;
uncertain:
	lease.unsafe=true; // NO reset/retry/restore or host release after UNKNOWN
finished:
	r.recovery_required=lease.unsafe || lease.running;
	if (r.recovery_required) { lease.unsafe=true; store.recovery_required=true; }
	return r.state==TT_RUNTIME_PASSED && r.stopped && !r.recovery_required;
}
template<class IO>
bool PioneerQ38PanelClose(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,const PioneerResidentBanks& banks,PioneerQ38PanelLease& lease,bool clean)
{
	if (!lease.saved) return clean && !lease.unsafe;
	bool safe=clean && !lease.running && !lease.unsafe && PioneerQ38PanelHeld(io,dram,noc,store,banks);
	const uint32_t* saved=io.Q38PanelSavedWords(); safe &= saved!=nullptr;
	if (safe && lease.dirty) {
		for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) io.Q38PanelWrite(TT_Q38_PANEL_VIEW_BASE+w*4,saved[1+w]);
		io.RuntimeCodeWrite(0,saved[0]);
		for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) safe &= io.Q38PanelRead(TT_Q38_PANEL_VIEW_BASE+w*4)==saved[1+w];
		safe &= io.RuntimeCodeRead(0)==saved[0];
		if (safe) lease.dirty=false;
	}
	lease.ready=false; lease.unsafe |= !safe || lease.dirty;
	if (lease.unsafe) store.recovery_required=true;
	return !lease.unsafe;
}
#endif
