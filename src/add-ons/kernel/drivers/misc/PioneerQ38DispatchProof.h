/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_PROOF_H
#define PIONEER_Q38_DISPATCH_PROOF_H
#include "PioneerQ38DispatchProtocol.h"
#include "PioneerComputeFleet.h"
// KERNEL-PRIVATE expected authority,NOT a userspace struct/capability. Every
// extent MUST first come from full initialized all-bank authorization and
// every source from the privately proved immutable boot cohort. Physical
// root before/after snapshots are independent of GPU-written statistics.
// This DATA-only helper has NO MMIO/start/reset/allocation authority.
struct PioneerQ38DispatchAuthority {
	PioneerQ38CohortExtent extent[119];
	uint32_t nextPeer[119]={},nextRoot=0;
	uint32_t source[120][16]={},before[8]={},after[8]={};
};
// Controller MUST first call with finalHeld=false (root_held==0),while
// root is physically PARKED but still released. Only AFTER every proof
// succeeds may it reset/hold root,prove that hold,and validate finalHeld.
// Thus a late peer/source/counter failure never authorizes an earlier reset.
static inline bool PioneerQ38DispatchProof(const tt_q38_dispatch_probe& job,const PioneerQ38DispatchAuthority& a,bool finalHeld=true,bool verifyOutputs=true)
{
	if (!PioneerQ38DispatchValid(job) || !a.nextRoot || a.nextRoot>=0x7fffffff) return false;
	const auto& r=job.result;
	if (r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.peers!=job.peers || r.started!=job.peers || r.completed!=job.peers
		|| r.waiting!=job.peers || r.recovery_required || r.root_sources_verified!=1 || r.nonce!=a.nextRoot
		|| r.root_started!=1 || r.root_completed!=1 || r.root_held!=unsigned(finalHeld) || r.root_phase!=0x7200c || r.root_count!=job.peers
		|| r.root_marker!=0x314d3851 || r.root_pc[0]!=0x72038 || r.root_pc[1]!=r.root_pc[0]) return false;
	const auto* s=r.stats;
	if (s[0]!=0x314d3851 || s[1]!=1 || s[2]!=1 || s[3]!=job.type || s[4]!=job.inputs || s[5]!=job.peers
		|| s[12]!=a.nextRoot || s[13] || s[14] || s[15] || s[10]<2*job.peers || s[11]<2*job.peers || s[11]>s[10]
		|| s[10]>2*65536*job.peers) return false;
	unsigned groups=0; for (unsigned w=0;w<job.peers;w++) groups+=job.part[w].groups;
	uint32_t reads=s[10]+3*s[11]+2*job.peers,flits=s[10]+3*s[11]+5*job.peers+groups,writes=2*job.peers;
	if (s[6]!=reads || s[7]!=writes || s[8]!=flits || s[9]!=writes) return false;
	for (unsigned c=0;c<8;c++) {
		uint32_t expected=c==0 || c==3?reads:c==1?flits:c==2 || c==6?reads+writes:writes;
		if (r.before[c]!=a.before[c] || r.after[c]!=a.after[c] || s[16+c]!=a.before[c] || s[24+c]!=a.after[c]
			|| uint32_t(a.after[c]-a.before[c])!=expected) return false;
	}
	for (unsigned i=0;i<16;i++) if (s[32+i]!=a.source[0][i]) return false;
	for (unsigned i=48;i<64;i++) if (s[i]) return false;
	for (unsigned w=0;w<119;w++) {
		const auto* p=r.peer[w];
		if (w>=job.peers) {
			for (unsigned value:r.peer[w]) if (value) return false;
			for (unsigned value:job.output[w]) if (value) return false;
			continue;
		}
		const auto& e=a.extent[w]; unsigned rows=job.part[w].groups*16;
		unsigned blocks=job.inputs/PioneerQ38CohortBlockInputs(job.type);
		unsigned expectedBytes=job.part[w].groups*blocks*PioneerQ38PanelProbeBytes(job.type);
		unsigned expectedReads=job.part[w].groups*blocks+(job.inputs*2+16383)/16384+1;
		unsigned expectedFlits=expectedBytes/64+(job.inputs*2+((blocks*4+63)&~63U))/64;
		if (!a.nextPeer[w] || a.nextPeer[w]>a.nextRoot || e.bytes!=expectedBytes || e.blocks!=job.part[w].groups*blocks
			|| e.reads!=expectedReads || e.flits!=expectedFlits || e.address<0x04000000 || e.address>=0xc4000000
			|| (e.address&63) || expectedBytes>0xc4000000-e.address || !e.route) return false;
		for (unsigned c=0;c<8;c++) if (uint32_t(p[8+c]-p[c])!=(c==1?expectedFlits:(c==0 || c==2 || c==3 || c==6)?expectedReads:0)) return false;
		const uint32_t command[]={e.address+(e.blocks-1)*PioneerQ38PanelProbeBytes(job.type),0,e.route,0x90000,0,
			PioneerWorkerX(w+1)|(PioneerWorkerY(w+1)<<6),0,0,PioneerQ38PanelProbeBytes(job.type),0};
		if (p[16]!=job.part[w].groups*(job.inputs/16) || p[17]!=a.nextPeer[w] || p[18]!=w+1 || p[19]!=0x32503851
			|| p[30]!=a.source[w+1][0] || p[31]!=command[5]) return false;
		for (unsigned i=0;i<10;i++) if (p[20+i]!=command[i]) return false;
		for (unsigned i=0;i<16;i++) if (p[32+i]!=a.source[w+1][i]) return false;
		for (unsigned i=48;i<64;i++) if (p[i]) return false;
		for (unsigned row=0;verifyOutputs && row<256;row++) {
			uint32_t bits=job.output[w][row]; if (row<rows?(bits&0x7f800000)==0x7f800000:bits!=0) return false;
		}
	}
	return true;
}
#endif
