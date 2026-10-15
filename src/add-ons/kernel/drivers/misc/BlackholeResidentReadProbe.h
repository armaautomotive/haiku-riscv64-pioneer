/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_RESIDENT_READ_PROBE_H
#define BLACKHOLE_RESIDENT_READ_PROBE_H
#include "BlackholeComputeNocProbe.h"

// A single fixed, host-dispatched *device-to-device* read. This is not host
// DMA, a resident model allocator, or a benchmark. No RISC is released.
// GDDR bank 0 at 64 MiB -> final 64 bytes of sole tile (1,2)'s L1.
enum {
	TT_RESIDENT_READ_REFUSED=0, TT_RESIDENT_READ_PASSED=1,
	TT_RESIDENT_READ_CONFIG_FAILED=2, TT_RESIDENT_READ_CHANGED=3,
	TT_RESIDENT_READ_TIMEOUT=4, TT_RESIDENT_READ_MISMATCH=5,
	TT_RESIDENT_READ_RESTORE_FAILED=6
};
struct tt_resident_read_result {
	uint32_t version,state,phase,issued,completed,polls,mismatches,recovery_required;
	uint32_t l1_restored,niu_restored,tlb_restored;
	uint32_t command_before[10],irq_config,before[4],after[4],tlb_before[2][3],tlb_after[2][3];
};
static inline bool BlackholeResidentReadRegister(uint32_t offset)
{
	return BlackholeComputeNocReadOffset(offset) || (offset<=0x24 && (offset&3)==0)
		|| (offset>=0x208 && offset<=0x214 && (offset&3)==0)
		|| (offset>=0x280 && offset<0x2c0 && (offset&3)==0) || offset==0x178;
}
static inline bool BlackholeResidentWriteRegister(uint32_t offset)
{
	return (offset<=0x24 && (offset&3)==0) || offset==0x40 || offset==0x100;
}
template<class IO>
bool BlackholeResidentIdle(IO& io)
{
	bool idle=true;
	for (unsigned i=0;i<4;i++) idle &= io.ResidentNocRead(i*0x800+0x40)==0;
	for (unsigned i=0;i<32;i++) idle &= (io.ResidentNocRead(0x240+i*4)&0xff)==0;
	return idle;
}
template<class IO>
bool BlackholeResidentSourceStable(IO& io,const tt_compute_noc_result& n,uint32_t config)
{
	bool okay=io.ResetRead()==0x47800;
	for (unsigned w=0;w<TT_DRAM_SOURCE_WORDS;w++)
		okay &= io.ResidentNocRead(BlackholeDramSourceOffset(w))==(w==0?config:n.source[0][w]);
	return okay;
}
template<class IO>
bool BlackholeResidentReadProbe(IO& io,tt_dram_samples_result& baseline,
	const tt_compute_noc_result& n,tt_resident_read_result& r)
{
	r={}; r.version=1; r.l1_restored=r.niu_restored=r.tlb_restored=1;
	if (baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK
		|| baseline.routes_verified!=7 || !baseline.data_restored || !baseline.tlb_restored || baseline.recovery_required
		|| n.version!=1 || n.state!=TT_COMPUTE_NOC_PASSED || !n.tlb_restored || n.recovery_required
		|| !BlackholeComputeNocRouteValid(n.source[0]) || !BlackholeDramBuildRoutes(baseline)) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
		r.tlb_before[s][w]=io.DramTlbRead(s,w);
		if (w==2 && (r.tlb_before[s][w]&0xffffc000)!=0) return false;
	}
	uint32_t source[16]={},saved[16]={};
	uint32_t config=n.source[0][0];
	bool modified=false,configured=false,canRestore=true;
	r.tlb_restored=0;
	do {
		r.state=TT_RESIDENT_READ_CONFIG_FAILED;
		r.phase=1;
		baseline.cores_checked=0;
		if (!BlackholeDramAllCoresHeld(io,baseline)) break;
		if (!BlackholeDramConfigure(io,1,0xffa00000,1,2,false)) break;
		if (!BlackholeDramSourceStable(io,baseline)
			|| !BlackholeResidentSourceStable(io,n,config)) { r.state=TT_RESIDENT_READ_CHANGED; break; }
		r.phase=2;
		r.irq_config=io.ResidentNocRead(0x178);
		if (!BlackholeResidentIdle(io) || (r.irq_config&0xffff)!=0) break;
		// Deliberately narrower than a general backend: untouched initiator 0
		// only. Do not restore unknown command flags/reserved fields.
		bool pristine=true;
		r.phase=3;
		for (unsigned w=0;w<10;w++) { r.command_before[w]=io.ResidentNocRead(w*4); pristine &= r.command_before[w]==0; }
		if (!pristine) break;
		r.phase=4;
		if (!BlackholeDramConfigure(io,0,0x04000000,baseline.translated[0][0],baseline.translated[0][1],true)) break;
		for (unsigned w=0;w<16;w++) source[w]=io.ResidentGddrRead(w);
		if (!BlackholeDramConfigure(io,0,0,1,2,false)) break;
		for (unsigned w=0;w<16;w++) saved[w]=io.ResidentL1Read(w);
		if (!BlackholeResidentSourceStable(io,n,config)) { r.state=TT_RESIDENT_READ_CHANGED; break; }
		modified=true; r.l1_restored=0;
		r.phase=5;
		for (unsigned w=0;w<16;w++) io.ResidentL1Write(w,~source[w]);
		bool poisoned=true;
		for (unsigned w=0;w<16;w++) poisoned &= io.ResidentL1Read(w)==~source[w];
		if (!poisoned) break;
		configured=true; r.niu_restored=0;
		r.phase=6;
		io.ResidentNocWrite(0x100,config|0x10000);
		if (!BlackholeResidentSourceStable(io,n,config|0x10000)) break;
		const uint32_t command[10]={0x04000000,0,18|(12<<6),0x17ffc0,0,1|(2<<6),0,0,64,0};
		r.phase=7;
		for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,command[w]);
		bool ready=BlackholeResidentIdle(io);
		for (unsigned w=0;w<10;w++) ready &= io.ResidentNocRead(w*4)==command[w];
		if (!ready || !BlackholeResidentSourceStable(io,n,config|0x10000)
			|| !BlackholeDramSourceStable(io,baseline)) break;
		for (unsigned w=0;w<4;w++) r.before[w]=io.ResidentNocRead(0x208+w*4);
		r.issued=1; // uncertain even when the posted doorbell is dropped
		r.phase=8;
		io.ResidentNocWrite(0x40,1);
		r.state=TT_RESIDENT_READ_TIMEOUT;
		for (unsigned attempt=0;attempt<256;attempt++) {
			r.polls++;
			// Required ordering: command readback BEFORE the counter reads.
			uint32_t pending=io.ResidentNocRead(0x40);
			for (unsigned w=0;w<4;w++) r.after[w]=io.ResidentNocRead(0x208+w*4);
			if (pending==0 && uint32_t(r.after[0]-r.before[0])==1
				&& uint32_t(r.after[1]-r.before[1])==1
				&& uint32_t(r.after[2]-r.before[2])==1 && uint32_t(r.after[3]-r.before[3])==1
				&& BlackholeResidentIdle(io)) { r.completed=1; break; }
			io.Pause();
		}
		if (!r.completed) { canRestore=false; break; }
		r.phase=9;
		if (!BlackholeResidentSourceStable(io,n,config|0x10000)
			|| !BlackholeDramSourceStable(io,baseline)) {
			r.state=TT_RESIDENT_READ_CHANGED; canRestore=false; break;
		}
		for (unsigned w=0;w<16;w++) r.mismatches += io.ResidentL1Read(w)!=source[w];
		r.state=r.mismatches?TT_RESIDENT_READ_MISMATCH:TT_RESIDENT_READ_PASSED;
	} while (false);
	// Never clear counters, cancel/reissue a request, assert reset or rewrite
	// the destination/initiator/FIFO after an unproved completion.
	if (modified && canRestore) {
		canRestore=BlackholeDramSourceStable(io,baseline)
			&& BlackholeResidentSourceStable(io,n,configured?config|0x10000:config)
			&& BlackholeResidentIdle(io);
		if (canRestore && configured) {
			for (unsigned attempt=0;attempt<2 && !r.niu_restored;attempt++) {
				for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,0);
				io.ResidentNocWrite(0x100,config);
				r.niu_restored=BlackholeResidentSourceStable(io,n,config);
				for (unsigned w=0;w<10;w++) r.niu_restored &= io.ResidentNocRead(w*4)==0;
			}
			canRestore=r.niu_restored;
		}
		if (canRestore) {
			for (unsigned attempt=0;attempt<2 && !r.l1_restored;attempt++) {
				for (unsigned w=0;w<16;w++) io.ResidentL1Write(w,saved[w]);
				r.l1_restored=io.ResetRead()==0x47800;
				for (unsigned w=0;w<16;w++) r.l1_restored &= io.ResidentL1Read(w)==saved[w];
			}
		}
	}
	for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,r.tlb_before[s][w]);
		r.tlb_restored=1;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
			r.tlb_after[s][w]=io.DramTlbRead(s,w);
			r.tlb_restored &= r.tlb_after[s][w]==r.tlb_before[s][w];
		}
	}
	if (!r.l1_restored || !r.niu_restored || !r.tlb_restored) {
		r.recovery_required=1;
		if (r.state!=TT_RESIDENT_READ_TIMEOUT && r.state!=TT_RESIDENT_READ_CHANGED) r.state=TT_RESIDENT_READ_RESTORE_FAILED;
	}
	return r.state==TT_RESIDENT_READ_PASSED && !r.recovery_required;
}
#endif
