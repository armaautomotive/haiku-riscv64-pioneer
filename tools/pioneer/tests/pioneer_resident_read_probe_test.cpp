/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentReadProbe.h"
struct Mock {
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}},command[10]={},l1[16]={},gddr[16]={};
	uint32_t counters[4]={10,20,30,40},config=0x6005;
	unsigned writes=0,drop=0,requests=0,orderedReads=0,postIssueWrites=0,pauses=0;
	bool timeout=false,acceptedOnly=false,badFlits=false,badData=false,changed=false,active=false,irq=false,restoreFails=false;
	bool doorbellRead=false,done=false;
#if defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE)
 bool nativePhysicalWindow=false;unsigned nativeWindowX=1,nativeWindowY=2;
#endif
	Mock() { for (unsigned w=0;w<16;w++) { l1[w]=100+w; gddr[w]=0x98765432+w; } }
	void Pause() { pauses++; }
	uint32_t ResetRead() { return active?0:0x47800; }
	uint32_t SourceRead(unsigned w) { assert(w<16); return source[w]; }
	uint32_t DramTlbRead(unsigned slot,unsigned w) { assert(slot<2 && w<3); return tlb[slot][w]; }
	void DramTlbWrite(unsigned slot,unsigned w,uint32_t value) {
		assert(slot<2 && w<3); if (++writes!=drop) tlb[slot][w]=value;
	}
	void Window(unsigned slot,uint32_t base,unsigned x,unsigned y,bool readOnly) {
#if defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE)
  assert(NativeWindowMatches(slot,base,x,y,readOnly));
#else
  for (unsigned w=0;w<3;w++) assert(tlb[slot][w]==BlackholeDramFixedTlbWord(base,x,y,readOnly,w));
#endif
 }
#if defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE)
 bool NativeWindowMatches(unsigned slot,uint32_t base,unsigned x,unsigned y,bool readOnly)const {
  if(slot>=2)return false;
  if(nativePhysicalWindow && x==1 && y==2 && !readOnly && (base==0 || base==0xffa00000)){
   x=nativeWindowX;y=nativeWindowY;
  }
  for(unsigned w=0;w<3;w++)if(tlb[slot][w]!=BlackholeDramFixedTlbWord(base,x,y,readOnly,w))return false;
  return true;
 }
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE)
 uint32_t NativeFifoStatusWord(uint32_t address) {
  assert(address==0x64 || address==0x68);Window(1,0xffa00000,1,2,false);
  return address==0x64 && (config&0x10000U)?0x10101010U:0U;
 } // TEST synchronous endpoint only;CURRENT register,not immutable baseline source[0]
#endif
	uint32_t ResidentGddrRead(unsigned w) { assert(w<16); Window(0,0x04000000,18,12,true); return gddr[w]; }
	uint32_t ResidentL1Read(unsigned w) { assert(w<16); Window(0,0,1,2,false); return l1[w]; }
	void ResidentL1Write(unsigned w,uint32_t value) {
		assert(w<16); Window(0,0,1,2,false);
		if (requests) { assert(done); postIssueWrites++; }
		if (++writes!=drop && !(restoreFails && value==100+w)) l1[w]=value;
	}
	uint32_t ResidentNocRead(uint32_t offset) {
		assert(BlackholeResidentReadRegister(offset)); Window(1,0xffa00000,1,2,false);
		if (offset<=0x24) return command[offset/4];
		if (offset==0x40) { if (requests) { doorbellRead=true; orderedReads++; } return requests && !done && !acceptedOnly?1:0; }
		for (unsigned i=1;i<4;i++) if (offset==i*0x800+0x40) return 0;
		if (offset>=0x208 && offset<=0x214) { if (requests) assert(doorbellRead); return counters[(offset-0x208)/4]; }
		if (offset>=0x240 && offset<0x2c0) return offset==0x240 && requests && !done?1:0;
		if (offset==0x178) return irq?1:0;
		for (unsigned w=0;w<16;w++) if (offset==BlackholeDramSourceOffset(w)) return (w==0?config:source[w])^((changed && requests && w==1)?1:0);
		assert(false); return 0;
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		assert(BlackholeResidentWriteRegister(offset)); Window(1,0xffa00000,1,2,false);
		if (requests) { assert(done); postIssueWrites++; }
		if (++writes==drop) return;
		if (offset<=0x24) command[offset/4]=value;
		else if (offset==0x100) config=value;
		else {
			assert(offset==0x40 && value==1 && requests==0 && config==0x16005);
			const uint32_t expected[10]={0x04000000,0,18|(12<<6),0x17ffc0,0,1|(2<<6),0,0,64,0};
			assert(!memcmp(command,expected,sizeof(command))); requests++;
			if (acceptedOnly) { counters[2]++; counters[3]++; }
			if (!timeout && !acceptedOnly) {
				done=true; counters[0]++; counters[1]+=badFlits?2:1; counters[2]++; counters[3]++;
				for (unsigned w=0;w<16;w++) l1[w]=gddr[w]^(badData?1:0);
			}
		}
	}
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result r={}; r.version=1; r.state=TT_DRAM_SAMPLE_ROUTES_OK;
		r.routes_verified=7; r.data_restored=r.tlb_restored=1; memcpy(r.source,source,sizeof(source));
		assert(BlackholeDramBuildRoutes(r)); return r;
	}
	tt_compute_noc_result Compute() {
		tt_compute_noc_result r={}; r.version=1; r.state=TT_COMPUTE_NOC_PASSED; r.tlb_restored=1;
		memcpy(r.source[0],source,sizeof(source)); return r;
	}
	void Restored() {
		for (unsigned w=0;w<3;w++) { assert(tlb[0][w]==w+1); assert(tlb[1][w]==w+4); }
		for (unsigned w=0;w<16;w++) { assert(l1[w]==100+w); assert(gddr[w]==0x98765432+w); }
		for (unsigned w=0;w<10;w++) assert(command[w]==0);
		assert(source[0]==0x6005 && config==0x6005);
	}
};
#ifndef PIONEER_RESIDENT_READ_MOCK_ONLY
int main()
{
	Mock io; auto b=io.Baseline(); auto n=io.Compute(); tt_resident_read_result r;
	assert(BlackholeResidentReadProbe(io,b,n,r));
	assert(r.issued && r.completed && !r.recovery_required && io.requests==1 && io.orderedReads>0); io.Restored();
	unsigned writes=io.writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; b=io.Baseline(); n=io.Compute(); io.drop=drop;
		BlackholeResidentReadProbe(io,b,n,r);
		assert(io.requests<=1);
		if (!r.recovery_required) io.Restored();
		if (r.issued && !r.completed) assert(r.recovery_required && io.postIssueWrites==0);
	}
	io={}; b=io.Baseline(); n=io.Compute(); io.timeout=true;
	assert(!BlackholeResidentReadProbe(io,b,n,r));
	assert(r.issued && !r.completed && r.recovery_required && io.requests==1 && io.postIssueWrites==0 && io.pauses==256);
	io={}; b=io.Baseline(); n=io.Compute(); io.acceptedOnly=true;
	assert(!BlackholeResidentReadProbe(io,b,n,r));
	assert(r.issued && !r.completed && r.recovery_required && io.postIssueWrites==0);
	io={}; b=io.Baseline(); n=io.Compute(); io.badFlits=true;
	assert(!BlackholeResidentReadProbe(io,b,n,r) && r.recovery_required && io.postIssueWrites==0);
	io={}; b=io.Baseline(); n=io.Compute();
	for (unsigned w=0;w<4;w++) io.counters[w]=UINT32_MAX;
	assert(BlackholeResidentReadProbe(io,b,n,r) && !r.recovery_required); io.Restored();
	io={}; b=io.Baseline(); n=io.Compute(); io.badData=true;
	assert(!BlackholeResidentReadProbe(io,b,n,r) && r.mismatches==16 && !r.recovery_required); io.Restored();
	io={}; b=io.Baseline(); n=io.Compute(); io.changed=true;
	assert(!BlackholeResidentReadProbe(io,b,n,r) && r.recovery_required && io.postIssueWrites==0);
	io={}; b=io.Baseline(); n=io.Compute(); io.restoreFails=true;
	assert(!BlackholeResidentReadProbe(io,b,n,r) && r.recovery_required && !r.l1_restored);
	io={}; b=io.Baseline(); n=io.Compute(); io.active=true;
	assert(!BlackholeResidentReadProbe(io,b,n,r) && !r.issued && !r.recovery_required); io.Restored();
	io={}; b=io.Baseline(); n=io.Compute(); io.irq=true;
	assert(!BlackholeResidentReadProbe(io,b,n,r) && !r.issued && !r.recovery_required); io.Restored();
	io={}; b=io.Baseline(); n=io.Compute(); b.recovery_required=1;
	assert(!BlackholeResidentReadProbe(io,b,n,r) && io.writes==0);
	assert(!BlackholeResidentWriteRegister(0x60) && !BlackholeResidentWriteRegister(0x178)
		&& !BlackholeResidentWriteRegister(0x208) && !BlackholeResidentWriteRegister(0x840));
}
#endif
