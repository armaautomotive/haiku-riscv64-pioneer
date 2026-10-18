/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_STREAM_PROBE_H
#define PIONEER_Q38_STREAM_PROBE_H
#include "PioneerQ38StreamProtocol.h"
#include "PioneerQ38StreamProbeImage.h"
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
#include "PioneerQ38RowsProbeImage.h"
#define PioneerQ38ActiveStreamWord PioneerQ38RowsWord
#define TT_Q38_ACTIVE_BOOT TT_Q38_ROWS_BOOT
#define TT_Q38_ACTIVE_DONE TT_Q38_ROWS_DONE
#define TT_Q38_ACTIVE_PARK TT_Q38_ROWS_PARK
#define TT_Q38_ACTIVE_MARKER TT_Q38_ROWS_MARKER
#define TT_Q38_ACTIVE_FAULT TT_Q38_ROWS_FAULT
#else
#define PioneerQ38ActiveStreamWord PioneerQ38StreamWord
#define TT_Q38_ACTIVE_BOOT TT_Q38_STREAM_BOOT
#define TT_Q38_ACTIVE_DONE TT_Q38_STREAM_DONE
#define TT_Q38_ACTIVE_PARK TT_Q38_STREAM_PARK
#define TT_Q38_ACTIVE_MARKER TT_Q38_STREAM_MARKER
#define TT_Q38_ACTIVE_FAULT TT_Q38_STREAM_FAULT
#endif
// Same full-view save/restore and UNKNOWN quarantine policy as qualified48.
// A session must choose local-block OR fullK lease,never both over one L1.
using PioneerQ38StreamLease=PioneerQ38PanelLease;
template<class IO> bool PioneerQ38StreamCodeMatches(IO& io)
{
	bool okay=io.RuntimeCodeRead(0)==TT_Q38_ACTIVE_BOOT;
	for (uint32_t a=0x70000;a<0x74000;a+=4) if (PioneerQ38PanelImmutable(a))
		okay &= io.Q38PanelRead(a)==(a<0x72000?0:PioneerQ38ActiveStreamWord((a-0x72000)/4));
	return okay;
}
template<class IO> bool PioneerQ38StreamPrepare(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	const PioneerResidentLease& store,const PioneerResidentBanks& banks,PioneerQ38StreamLease& lease)
{
	if (TT_Q38_PANEL_VIEW_BYTES<0x42000) return false;
	if (lease.ready) return !lease.unsafe && !lease.running && PioneerQ38PanelHeld(io,dram,noc,store,banks) && PioneerQ38StreamCodeMatches(io);
	uint32_t* saved=io.Q38PanelSavedWords();
	if (!saved || lease.saved || lease.unsafe || !PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
	for (unsigned i=0;i<3;i++) { uint32_t value=io.RuntimeControlRead(i); if (i?value!=0:value&1) return false; }
	saved[0]=io.RuntimeCodeRead(0);
	for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) saved[1+w]=io.Q38PanelRead(TT_Q38_PANEL_VIEW_BASE+w*4);
	lease.saved=true;
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) { lease.unsafe=true; return false; }
	lease.dirty=true;
	for (uint32_t a=0x70000;a<0x74000;a+=4) io.Q38PanelWrite(a,a<0x72000?0:PioneerQ38ActiveStreamWord((a-0x72000)/4));
	io.RuntimeCodeWrite(0,TT_Q38_ACTIVE_BOOT); lease.ready=PioneerQ38StreamCodeMatches(io); return lease.ready;
}
template<class IO> bool PioneerQ38StreamCommandMatches(IO& io,const PioneerQ38StreamExtent& extent,unsigned type,bool complete)
{
	const uint32_t command[]={extent.address+(complete?(extent.blocks-1)*PioneerQ38PanelProbeBytes(type):0),0,extent.route,
		0x90000,0,1|(2<<6),0,0,PioneerQ38PanelProbeBytes(type),0};
	bool okay=BlackholeResidentIdle(io);
	for (unsigned w=0;w<10;w++) okay &= io.ResidentNocRead(w*4)==command[w];
	return okay;
}
template<class IO> bool PioneerQ38StreamRun(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,const PioneerResidentBanks& banks,PioneerQ38StreamLease& lease,
	const uint8_t* initialized,uint32_t initializedPages,tt_q38_stream_probe& job)
{
	auto& r=job.result; r={}; r.version=1;
	PioneerQ38StreamExtent extent;
	if (!PioneerQ38StreamAuthorize(banks,dram,initialized,initializedPages,job,extent)
		|| !lease.ready || lease.unsafe || lease.running || !PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
	r.state=TT_RUNTIME_CONFIG_FAILED; r.prepared=1;
	if (!PioneerQ38StreamCodeMatches(io)) return false;
	{
		const uint32_t descriptor[]={TT_Q38_ACTIVE_MARKER,job.type,PioneerQ38PanelProbeBytes(job.type),0,0,job.inputs,extent.address
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
			,job.groups
#endif
		};
		if (!PioneerQ38PanelStage(io,0x74000,descriptor,sizeof(descriptor)/4) || !PioneerQ38PanelStage(io,0x94000,job.scales,
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
			job.inputs/PioneerQ38StreamBlockInputs(job.type)
#else
			extent.blocks
#endif
			)
			|| !PioneerQ38PanelStage(io,0xa0000,job.activation,job.inputs/2)) return false;
		for (unsigned w=0;w<PioneerQ38StreamRows(job);w++) io.Q38PanelWrite(0x84000+w*4,0x7fc00000);
		for (unsigned w=0;w<PioneerQ38StreamRows(job);w++) if (io.Q38PanelRead(0x84000+w*4)!=0x7fc00000) return false;
		const uint32_t command[]={extent.address,0,extent.route,0x90000,0,1|(2<<6),0,0,PioneerQ38PanelProbeBytes(job.type),0};
		if (!BlackholeHostWriteCommand(io,command) || !PioneerQ38StreamCommandMatches(io,extent,job.type,false)) return false;
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
	for (unsigned attempt=0;attempt<
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_ROWS
		150000; // explicit15s bound for up to16 row groups,not a retry
#else
		10000;
#endif
		attempt++) {
		r.polls++; if (io.ResetRead()!=0x46800) break;
		r.phase=io.RuntimeStatusRead(0); r.panels=io.RuntimeStatusRead(1); r.marker=io.RuntimeStatusRead(2);
		if (r.marker==TT_Q38_ACTIVE_FAULT) break;
		if (r.phase==TT_Q38_ACTIVE_DONE && r.panels==PioneerQ38StreamGroups(job)*(job.inputs/16) && r.marker==TT_Q38_ACTIVE_MARKER) {
			io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
			if (r.pc[0]==TT_Q38_ACTIVE_PARK && r.pc[1]==r.pc[0]) { r.completed=1; break; }
		}
		io.Pause();
	}
	if (!r.completed || io.ResetRead()!=0x46800 || !PioneerQ38PanelSources(io,dram,noc,store,banks)
		|| !PioneerQ38StreamCodeMatches(io) || !PioneerQ38StreamCommandMatches(io,extent,job.type,true)) goto uncertain;
	for (unsigned i=0;i<8;i++) {
		r.after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
		// 0x210 is SHARED read/write accepted count,not a forbidden write.
		uint32_t expected=i==1?extent.flits:(i==0 || i==2 || i==3 || i==6)?extent.blocks:0;
		if (uint32_t(r.after[i]-r.before[i])!=expected) goto uncertain;
	}
	r.state=TT_RUNTIME_PASSED;
	for (unsigned row=0;row<PioneerQ38StreamRows(job);row++) {
		job.output[row]=io.Q38PanelRead(0x84000+row*4);
		if ((job.output[row]&0x7f800000)==0x7f800000) r.state=TT_RUNTIME_NONFINITE;
	}
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) goto uncertain;
	io.RuntimeResetWrite(0x47800); io.Pause();
	if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) goto uncertain;
	r.stopped=1; lease.running=false; goto finished;
uncertain:
	lease.unsafe=true; // NO retry/reset/restore/free after UNKNOWN
finished:
	r.recovery_required=lease.unsafe || lease.running;
	if (r.recovery_required) { lease.unsafe=true; store.recovery_required=true; }
	return r.state==TT_RUNTIME_PASSED && r.stopped && !r.recovery_required;
}
#endif
