/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstring>
#define PIONEER_QWEN35_DATAFLOW_REWRITE
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentReadProbe.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowNoC1Audit.h"

struct MockNoC1 {
	uint32_t source[16]={},node=0,endpoint=0x01010000,logical=0;
	unsigned reads=0,pauses=0,busy=0;bool change=false;
	uint32_t Read(unsigned offset) {
		reads++;
		if (offset==0x44) return node;
		if (offset==0x48) return endpoint;
		if (offset==0x148) return logical^(change && pauses ? 1U : 0U);
		for (unsigned i=0;i<16;i++) if (offset==BlackholeDramSourceOffset(i)) return source[i];
		for (unsigned i=0;i<4;i++) if (offset==0x40+i*0x800) return busy==1;
		if (offset>=0x240 && offset<0x2c0 && !(offset&3)) return busy==2;
		assert(false);return UINT32_MAX;
	}
	void Pause() { pauses++; }
};

static void SetEntry(uint32_t* source,unsigned base,unsigned index,unsigned value)
{
	const unsigned slot=base+index/6,shift=(index%6)*5;
	source[slot]=(source[slot]&~(31U<<shift))|((value&31U)<<shift);
}

int main()
{
	assert(BlackholeResidentReadRegister(0x10044));
	assert(BlackholeResidentReadRegister(0x10148));
	assert(BlackholeResidentReadRegister(0x11840));
	assert(!BlackholeResidentReadRegister(0x12000));
	assert(!BlackholeResidentWriteRegister(0x10040));
	tt_dram_samples_result dram{};
	const uint32_t source[]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	memcpy(dram.source,source,sizeof(source));assert(BlackholeDramBuildRoutes(dram));
	for (unsigned core=0;core<120;core++) {
		MockNoC1 io;io.source[0]=0x4000;
		for (unsigned i=0;i<32;i++) {
			const unsigned x=BlackholeDramSourceX(dram.source,i),y=BlackholeDramSourceY(dram.source,i);
			SetEntry(io.source,1,i,x<17?16-x:0);
			SetEntry(io.source,7,i,y<12?11-y:0);
		}
		const unsigned x=PioneerWorkerX(core),y=PioneerWorkerY(core);
		io.logical=x|(y<<6);
		io.node=(16-BlackholeDramSourceX(dram.source,x))|((11-y)<<6)|(17<<12)|(12<<19);
		assert(PioneerQwen35DataflowNoC1Audit(io,dram,core) && io.pauses==1);
		auto bad=io;bad.change=true;bad.pauses=0;
		assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.source[0]|=0x10000;assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.source[15]|=0x400;assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.logical^=1;assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.node^=1;assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.endpoint=0x10000;assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.source[14]|=1U<<y;assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.source[13]|=1U<<dram.translated[0][0];assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;SetEntry(bad.source,7,dram.translated[0][1],0);
		assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;
		const unsigned otherX=PioneerWorkerX(((core/10+1)%12)*10);
		const unsigned mapped=BlackholeDramSourceX(bad.source,otherX);
		SetEntry(bad.source,1,otherX,(mapped+1)%17);
		assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.busy=1;assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
		bad=io;bad.busy=2;assert(!PioneerQwen35DataflowNoC1Audit(bad,dram,core));
	}
	MockNoC1 invalid;assert(!PioneerQwen35DataflowNoC1Audit(invalid,dram,120) && !invalid.reads);
	puts("PASS read-only NoC1 audit: all 120 cores, seven bank routes, ID, translation, drift and idle failures; SIMULATED ONLY");
}
