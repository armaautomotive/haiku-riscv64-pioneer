/* SPDX-License-Identifier: MIT */
// REAL native client with synthetic syscalls only. No card/model access.
#include <cassert>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
static unsigned calls=0,ends=0,closes=0,fault=0,target=0,field=0,missing=0;
static bool failed=false;
static int MockOpen(const char*,int) { return 91; }
static int MockClose(int fd) { assert(fd==91); closes++; return 0; }
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes) {
	assert(fd==91 && !failed); calls++;
	if (op==TT_PCI_PROBE_PIONEER_SESSION_SUPPORT) {
		auto& s=*(tt_resident_session_result*)data; s.version=1; s.capabilities=TT_PIONEER_RESIDENT_CAPABILITIES&~missing; return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_SESSION_BEGIN) { auto& s=*(tt_resident_session_result*)data; s.state=TT_RESIDENT_SESSION_READY; s.active=s.host_ready=1; return 0; }
	if (op==TT_PCI_PROBE_PIONEER_SESSION_END) { auto& s=*(tt_resident_session_result*)data; s.state=TT_RESIDENT_SESSION_CLOSED; s.pci_restored=s.host_released=1; ends++; return 0; }
	if (op==TT_PCI_PROBE_PIONEER_RUNTIME_Q8_FLEET) {
		auto& p=*(tt_pioneer_runtime_fleet*)data; assert(p.version==2 && bytes==PioneerFleetRequestBytes(p.outputs));
		PioneerFleetPlan plan; assert(PioneerFleetPartition(p.inputs,p.outputs,p.workers,plan));
		auto& r=p.result; r={}; r.version=1; r.state=TT_RUNTIME_PASSED; r.workers=r.started=r.completed=r.stopped=plan.workers;
		r.staged_pages=(p.inputs*2+4095)/4096+1;
		for (unsigned w=0;w<plan.workers;w++) {
			auto& c=r.core[w]; c.version=1; c.state=1; c.prepared=c.context_reset=c.started=c.completed=c.stopped=1;
			c.phase=TT_RUNTIME_Q8_FINAL_PHASE; c.tiles=(p.inputs/16)*(plan.rows[w]/16); c.marker=0x4d415431; c.pc[0]=c.pc[1]=TT_RUNTIME_Q8_PARK_PC;
			for (unsigned i=0;i<4;i++) { c.before[i]=0xfffffffe; c.after[i]=c.before[i]+(i==1?c.tiles/32*144+256*(w!=0):c.tiles/32+(w!=0)); }
		}
		for (unsigned row=0;row<p.outputs;row++) p.output[row]=0x3f800000+row;
		if (fault) {
			failed=true;
			if (fault==1) { errno=EIO; return -1; }
			if (fault==2) ((uint32_t*)&r.core[target])[field]^=1;
			if (fault==3) ((uint32_t*)&r)[field]^=1;
		}
		return 0;
	}
	assert(op==TT_PCI_PROBE_PIONEER_FLEET_ATTENTION);
	auto& p=*(tt_pioneer_fleet_attention*)data; assert(p.version==1 && bytes==PioneerFleetAttentionBytes(p.tokens));
	for (unsigned g=0;g<8;g++) {
		for (unsigned i=0;i<256;i++) assert(p.query[g*256+i]==0x3f800000);
		for (unsigned i=0;i<p.tokens*64;i++) { assert(p.prefix[g*p.tokens*128+i]==0x38003800); assert(p.prefix[g*p.tokens*128+p.tokens*64+i]==0x3c003c00); }
		auto& r=p.result[g]; r={}; r.version=1; r.state=TT_ATTN_TILE_PASSED; r.prepared=r.context_reset=r.started=r.completed=r.stopped=1;
		r.tokens=r.count=p.tokens; r.staged_words=256+p.tokens*128; r.phase=TT_ATTENTION_TILE_DONE; r.marker=TT_ATTENTION_TILE_MARKER;
		r.pc[0]=r.pc[1]=TT_ATTENTION_TILE_PARK;
		for (unsigned i=0;i<4;i++) r.before[i]=r.after[i]=0xfffffffe;
	}
	for (auto& word:p.output) word=0x3f800000;
	if (fault) { failed=true; if (fault==1) { errno=EIO; return -1; } ((uint32_t*)&p.result[target])[field]^=1; }
	return 0;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#include "../TenstorrentHaikuResident.h"
#undef ioctl
#undef close
#undef open
static void Reset(unsigned f=0,unsigned t=0,unsigned word=0) { calls=ends=closes=missing=0; failed=false; fault=f; target=t; field=word; }
static bool Open(PioneerResidentDevice& d) { return d.Open(2*1024*1024,false,true,true,true,true,true,true,true,1,120); }
int main() {
	uint32_t x[512],scale[32]; for (auto& v:x) v=0x40014001; for (auto& v:scale) v=0x3c800000;
	std::vector<uint32_t> output(8192,0xdeadbeef);
	for (unsigned w=0;w<120;w++) for (unsigned f:{0U,1U,2U}) for (unsigned word:{0U,1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U}) {
		if ((f<2 && (w || word)) || (!f && w)) continue;
		Reset(f,w,word); std::fill(output.begin(),output.end(),0xdeadbeef);
		{ PioneerResidentDevice d; assert(Open(d)); bool okay=d.Q8Matvec(0,1024,8192,x,scale,output.data()); assert(okay==!f);
			if (!f) { assert(d.peakWorkers==120); for (unsigned row=0;row<8192;row++) assert(output[row]==0x3f800000+row); assert(d.Finish()); }
			else { unsigned previous=calls; assert(!d.Q8Matvec(0,1024,8192,x,scale,output.data()) && !d.Finish() && calls==previous); for (auto v:output) assert(v==0xdeadbeef); }
		}
		assert(closes==1 && ends==unsigned(!f));
	}
	for (unsigned word=0;word<8;word++) {
		Reset(3,0,word); PioneerResidentDevice d; assert(Open(d)); assert(!d.Q8Matvec(0,1024,8192,x,scale,output.data())); assert(!d.Finish() && !ends);
	}
	for (unsigned n:{1U,2U,255U,256U}) for (unsigned g=0;g<8;g++) for (unsigned word:{0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U,24U}) {
		Reset(2,g,word); uint32_t query[2048]; for (auto& v:query) v=0x3f800000;
		std::vector<uint32_t> keys(n*512,0x38003800),values(n*512,0x3c003c00),result(2048,0xdeadbeef);
		{ PioneerResidentDevice d; assert(Open(d)); assert(!d.AttentionGroups(n,query,keys.data(),values.data(),result.data()));
			unsigned previous=calls; assert(!d.Finish() && calls==previous); for (auto v:result) assert(v==0xdeadbeef); }
		assert(!ends && closes==1);
	}
	for (unsigned n:{1U,256U}) {
		Reset(); uint32_t query[2048]; for (auto& v:query) v=0x3f800000;
		std::vector<uint32_t> keys(n*512,0x38003800),values(n*512,0x3c003c00),result(2048,0xdeadbeef);
		{ PioneerResidentDevice d; assert(Open(d) && d.AttentionGroups(n,query,keys.data(),values.data(),result.data()));
			assert(d.attentionCalls==8); for (auto v:result) assert(v==0x3f800000); assert(d.Finish()); }
		assert(ends==1 && closes==1);
	}
	for (unsigned absent:{TT_PIONEER_CAP_Q8_FLEET,TT_PIONEER_CAP_FLEET_ATTENTION}) {
		Reset(); missing=absent; { PioneerResidentDevice d; assert(!Open(d)); } assert(calls==1 && closes==1 && !ends);
	}
	puts("PASS REAL fleet client, variable inline ABI,120 worker proofs/all8 attention groups,wrapped counters,no partial outputs,close-only/no retry on faults,capability refusal BEFORE BEGIN; NO hardware");
}
