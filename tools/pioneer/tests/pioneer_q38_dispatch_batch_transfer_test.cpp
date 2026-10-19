/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_RUN_MOCK_ONLY
#include "pioneer_q38_dispatch_run_test.cpp"
struct FrameIO {
	uint32_t memory[119*64+2]={};
	unsigned reads=0,writes=0,fences=0,views=0,corrupt=~0U;
	bool stable=true,reading=false;
	bool Q38PanelMappingStable() { return stable; }
	struct View {
		FrameIO& io;
		void PayloadFence() { io.fences++; }
		uint32_t PayloadRead(unsigned w) { assert(w<119*64); io.reads++; io.reading=true; return io.memory[w+1]^(w==io.corrupt?1:0); }
		void PayloadWrite(unsigned w,uint32_t value) { assert(w<119*64 && !io.reading); io.writes++; io.memory[w+1]=value; }
		uint64_t PayloadReadPair(unsigned w) { assert(!(w&1)); uint64_t lo=PayloadRead(w); return lo|(uint64_t(PayloadRead(w+1))<<32); }
		void PayloadWritePair(unsigned w,uint32_t lo,uint32_t hi) { assert(!(w&1)); PayloadWrite(w,lo); PayloadWrite(w+1,hi); }
	};
	View Q38PayloadData(uint32_t a) { assert(a==0x74080 || a==0x76000); views++; return View{*this}; }
};
int main() {
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	assert(PioneerQ38DispatchStageRange(0x74080,119*8));
	assert(!PioneerQ38DispatchStageRange(0x74084,119*8));
	assert(!PioneerQ38DispatchStageRange(0x74080,119*8-1));
	assert(!PioneerQ38DispatchStageRange(0x74080,119*8+1));
	for (unsigned n=1;n<=119;n++) assert(PioneerQ38DispatchReadRange(0x76000,n*64));
	assert(!PioneerQ38DispatchReadRange(0x76000,0));
	assert(!PioneerQ38DispatchReadRange(0x76000,119*64+64));
	assert(!PioneerQ38DispatchReadRange(0x76004,119*64));
	assert(!PioneerQ38DispatchReadRange(0x76000,119*64-1));
	uint32_t task[119*8]; for (unsigned i=0;i<119*8;i++) task[i]=i*0x23456789U+17;
	for (unsigned bad=0;bad<=119*8;bad++) {
		FrameIO io; io.corrupt=bad; io.memory[0]=io.memory[119*8+1]=0xfeedface;
		assert(PioneerQ38DispatchStage(io,0x74080,task,119*8,0)==(bad==119*8));
		assert(io.views==1 && io.fences==3 && io.reads==119*8 && io.writes==119*8);
		assert(io.memory[0]==0xfeedface && io.memory[119*8+1]==0xfeedface);
		for (unsigned i=0;i<119*8;i++) assert(io.memory[i+1]==task[i]);
	}
	for (unsigned n:{1U,2U,7U,119U}) {
		FrameIO io; uint32_t output[119*64+2];
		for (unsigned i=0;i<119*64+2;i++) { io.memory[i]=i*0x76543211U; output[i]=0xdeadbeef; }
		assert(PioneerQ38DispatchRead(io,0x76000,output+1,n*64,0));
		assert(io.views==1 && io.fences==2 && io.reads==n*64 && !io.writes);
		assert(output[0]==0xdeadbeef && output[n*64+1]==0xdeadbeef);
		for (unsigned i=0;i<n*64;i++) assert(output[i+1]==io.memory[i+1]);
	}
	{
		FrameIO io; io.stable=false;
		assert(!PioneerQ38DispatchStage(io,0x74080,task,119*8,0));
		assert(!PioneerQ38DispatchRead(io,0x76000,task,64,0));
		assert(!io.views && !io.reads && !io.writes && !io.fences);
	}
	puts("PASS batched PCIe data ONLY:exact private ranges,ALL952 task-word corruptions refused after complete readback,all writes before reads,one3-fence stage/one2-fence proof prefix,1/2/7/119 proof boundaries and canaries,no mapping/no IO;NO native speed claim");
}
