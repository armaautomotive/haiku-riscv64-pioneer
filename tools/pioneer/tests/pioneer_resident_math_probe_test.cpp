/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentMathProbe.h"
struct Mock {
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}},command[10]={},config=0x6005,reset=0x47800;
	uint32_t image[TT_RESIDENT_MATH_IMAGE_WORDS]={},gddr[TT_RESIDENT_MATH_GDDR_WORDS]={};
	uint32_t saved[TT_RESIDENT_MATH_IMAGE_WORDS+TT_RESIDENT_MATH_GDDR_WORDS]={};
	uint32_t counters[4]={10,20,30,40};
	unsigned writes=0,drop=0,starts=0,stopWrites=0,payloadAfterStart=0,pcReads=0;
	bool timeout=false,badFlits=false,badData=false,badPc=false,changed=false,active=false,irq=false;
	bool gddrRestoreFails=false,l1RestoreFails=false,noHeap=false;
	bool started=false,stopped=false,doorbellRead=false;
	Mock() {
		for (unsigned w=0;w<TT_RESIDENT_MATH_IMAGE_WORDS;w++) image[w]=0x98760000+w;
		for (unsigned w=0;w<TT_RESIDENT_MATH_GDDR_WORDS;w++) gddr[w]=0x54320000+w;
	}
	uint32_t* ResidentMathSavedWords() { return noHeap?nullptr:saved; }
	void Pause() {}
	uint32_t ResetRead() { return active?0:reset; }
	uint32_t SourceRead(unsigned w) { assert(w<16); return source[w]; }
	uint32_t DramTlbRead(unsigned s,unsigned w) { assert(s<2 && w<3); return tlb[s][w]; }
	void DramTlbWrite(unsigned s,unsigned w,uint32_t value) { assert(s<2 && w<3); if (++writes!=drop) tlb[s][w]=value; }
	void Window(unsigned s,uint32_t base,unsigned x,unsigned y,bool readOnly=false) {
		for (unsigned w=0;w<3;w++) assert(tlb[s][w]==BlackholeDramFixedTlbWord(base,x,y,readOnly,w));
	}
	uint32_t ResidentGddrRead(unsigned w) { assert(w<TT_RESIDENT_MATH_GDDR_WORDS); Window(0,0x04000000,18,12); return gddr[w]; }
	void ResidentGddrWrite(unsigned w,uint32_t value) {
		assert(w<TT_RESIDENT_MATH_GDDR_WORDS); Window(0,0x04000000,18,12);
		if (started) { assert(stopped); payloadAfterStart++; }
		if (++writes!=drop && !(gddrRestoreFails && value==0x54320000+w)) gddr[w]=value;
	}
	uint32_t MathImageRead(unsigned w) { assert(w<TT_RESIDENT_MATH_IMAGE_WORDS); Window(0,0,1,2); return image[w]; }
	void MathImageWrite(unsigned w,uint32_t value) {
		assert(w<TT_RESIDENT_MATH_IMAGE_WORDS); Window(0,0,1,2);
		if (started) { assert(stopped); payloadAfterStart++; }
		if (++writes!=drop && !(l1RestoreFails && value==0x98760000+w)) image[w]=value;
	}
	uint32_t MathControlRead(unsigned w) { assert(w<3); Window(1,0xffa00000,1,2); return 0; }
	uint32_t MathPcRead() { assert(started); pcReads++; return badPc?0:TT_UNPACK_PARK_PC; }
	void MathResetWrite(uint32_t value) {
		Window(1,0xffa00000,1,2);
		assert(value==0x87ffff || value==0x46800 || value==0x47800);
		if (started) { assert(!timeout && !badFlits && !badPc && !changed); stopWrites++; }
		if (++writes==drop) return;
		reset=value;
		if (value==0x46800) {
			assert(!started && config==0x16005); started=true; starts++;
			for (unsigned w=0;w<10;w++) assert(command[w]==BlackholeResidentMathCommandWord(w));
			for (unsigned w=0;w<TT_RESIDENT_MATH_GDDR_WORDS;w++) assert(gddr[w]==BlackholeResidentMathGddrWord(w));
			for (unsigned w=0;w<TT_RESIDENT_MATH_IMAGE_WORDS;w++) assert(image[w]==BlackholeResidentMathImageWord(w));
			if (!timeout) {
				counters[0]+=2; counters[1]+=badFlits?33:34; counters[2]+=2; counters[3]+=2;
				image[TT_RESIDENT_MATH_STATUS_INDEX]=TT_UNPACK_FINAL_PHASE;
				image[TT_RESIDENT_MATH_STATUS_INDEX+1]=2;
				image[TT_RESIDENT_MATH_STATUS_INDEX+2]=0x4d415431;
				for (unsigned w=0;w<256;w++) image[1+TT_RESIDENT_MATH_CODE_WORDS+w]=BlackholeNonuniformExpected(21,w)^(badData?1:0);
			}
		}
		if (started && value==0x47800) stopped=true;
	}
	uint32_t ResidentNocRead(uint32_t offset) {
		assert(BlackholeResidentReadRegister(offset)); Window(1,0xffa00000,1,2);
		if (offset<=0x24) return command[offset/4];
		for (unsigned i=0;i<4;i++) if (offset==i*0x800+0x40) { doorbellRead=true; return 0; }
		if (offset>=0x208 && offset<=0x214) { if (started) assert(doorbellRead); return counters[(offset-0x208)/4]; }
		if (offset>=0x240 && offset<0x2c0) return offset==0x240 && started && timeout?1:0;
		if (offset==0x178) return irq?1:0;
		for (unsigned w=0;w<16;w++) if (offset==BlackholeDramSourceOffset(w)) return (w==0?config:source[w])^((changed && started && w==1)?1:0);
		assert(false); return 0;
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		Window(1,0xffa00000,1,2);
		assert(BlackholeResidentWriteRegister(offset) && offset!=0x40); // HOST NEVER rings the doorbell
		if (started) { assert(stopped); payloadAfterStart++; }
		if (++writes==drop) return;
		if (offset==0x100) config=value; else command[offset/4]=value;
	}
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result b={}; b.version=1; b.state=TT_DRAM_SAMPLE_ROUTES_OK;
		b.routes_verified=7; b.data_restored=b.tlb_restored=1; memcpy(b.source,source,sizeof(source));
		assert(BlackholeDramBuildRoutes(b)); return b;
	}
	tt_compute_noc_result Compute() {
		tt_compute_noc_result n={}; n.version=1; n.state=TT_COMPUTE_NOC_PASSED; n.tlb_restored=1;
		memcpy(n.source[0],source,sizeof(source)); return n;
	}
	void Restored() {
		for (unsigned w=0;w<3;w++) { assert(tlb[0][w]==w+1); assert(tlb[1][w]==w+4); }
		for (unsigned w=0;w<TT_RESIDENT_MATH_IMAGE_WORDS;w++) assert(image[w]==0x98760000+w);
		for (unsigned w=0;w<TT_RESIDENT_MATH_GDDR_WORDS;w++) assert(gddr[w]==0x54320000+w);
		for (unsigned w=0;w<10;w++) assert(command[w]==0);
		assert(config==0x6005 && reset==0x47800);
	}
};
int main()
{
	Mock io; auto b=io.Baseline(); auto n=io.Compute(); tt_resident_math_result r;
	assert(BlackholeResidentMathProbe(io,b,n,r));
	assert(r.math.started && r.math.completed && r.math.stopped && !r.recovery_required && io.starts==1); io.Restored();
	unsigned writes=io.writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; b=io.Baseline(); n=io.Compute(); io.drop=drop;
		BlackholeResidentMathProbe(io,b,n,r); assert(io.starts<=1);
		if (!r.recovery_required) io.Restored();
		if (r.math.started && !r.math.completed) assert(r.recovery_required && io.stopWrites==0 && io.payloadAfterStart==0);
	}
	for (unsigned fault=0;fault<5;fault++) {
		io={}; b=io.Baseline(); n=io.Compute(); io.timeout=fault==0; io.badFlits=fault==1;
		io.badPc=fault==2; io.changed=fault==3; io.badData=fault==4;
		assert(!BlackholeResidentMathProbe(io,b,n,r));
		if (fault==4) { assert(r.math.mismatches==256 && !r.recovery_required); io.Restored(); }
		else assert(r.recovery_required && io.stopWrites==0 && io.payloadAfterStart==0);
	}
	for (unsigned fault=0;fault<2;fault++) {
		io={}; b=io.Baseline(); n=io.Compute(); io.gddrRestoreFails=fault==0; io.l1RestoreFails=fault==1;
		assert(!BlackholeResidentMathProbe(io,b,n,r) && r.recovery_required);
	}
	io={}; b=io.Baseline(); n=io.Compute(); io.active=true;
	assert(!BlackholeResidentMathProbe(io,b,n,r) && !r.math.started && !r.recovery_required); io.Restored();
	io={}; b=io.Baseline(); n=io.Compute(); io.irq=true;
	assert(!BlackholeResidentMathProbe(io,b,n,r) && !r.math.started && !r.recovery_required); io.Restored();
	io={}; b=io.Baseline(); n=io.Compute(); io.noHeap=true;
	assert(!BlackholeResidentMathProbe(io,b,n,r) && io.writes==0);
	io={}; b=io.Baseline(); n=io.Compute(); b.recovery_required=1;
	assert(!BlackholeResidentMathProbe(io,b,n,r) && io.writes==0);
}
