/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_RUN_MOCK_ONLY
#include "pioneer_q38_dispatch_run_test.cpp"
struct PostedIO {
	std::vector<uint32_t> memory=std::vector<uint32_t>(0x40000/4,0);
	std::vector<std::pair<uint32_t,uint32_t>> pending;
	unsigned writes=0,reads=0,fences=0; bool stable=true,corrupt=false;
	bool Q38PanelMappingStable() { return stable; }
	uint32_t Q38PanelRead(uint32_t a) {
		assert(a>=0x74000 && a<0xb4000 && !(a&3)); reads++;
		for (const auto& p:pending) memory[(p.first-0x74000)/4]=p.second;
		pending.clear(); return memory[(a-0x74000)/4]^(corrupt?1:0);
	}
	void Q38PanelWrite(uint32_t a,uint32_t v) { assert(a>=0x74000 && a<0xb4000 && !(a&3)); writes++; pending.emplace_back(a,v); }
};
struct PostedPairs:PostedIO {
	struct Data {
		PostedPairs& io; uint32_t base;
		void PayloadFence() { io.fences++; } // CPU fence DOES NOT drain PCIe
		uint32_t PayloadRead(unsigned w) { return io.Q38PanelRead(base+w*4); }
		void PayloadWrite(unsigned w,uint32_t v) { io.Q38PanelWrite(base+w*4,v); }
		void PayloadWritePair(unsigned w,uint32_t lo,uint32_t hi) { assert(!(w&1)); PayloadWrite(w,lo); PayloadWrite(w+1,hi); }
		uint64_t PayloadReadPair(unsigned w) { assert(!(w&1)); uint64_t lo=PayloadRead(w); return lo|(uint64_t(PayloadRead(w+1))<<32); }
	};
	Data Q38PayloadData(uint32_t a) { return Data{*this,a}; }
};
template<class IO> static void CheckPosted()
{
	std::vector<uint32_t> input(8704); for (unsigned w=0;w<input.size();w++) input[w]=0xdead0000+w;
	for (auto shape:{std::pair<uint32_t,unsigned>{0x74000,16},{0x74080,8},{0x74f40,8},
		{0xa0000,1707},{0xa0000,2048},{0xa0000,5803},{0xa0000,2560},{0xa0000,3072},{0xa0000,8704},
		{0xab000,32},{0xab000,80},{0xab000,160},{0xab000,192},{0xab000,544}}) {
		IO io;
		assert((PioneerQ38WorkStage<IO,true>(io,shape.first,input.data(),shape.second,0)));
		assert(io.writes==shape.second && !io.reads && io.pending.size()==shape.second);
		assert(PioneerQ38WorkDrain(io,0) && io.reads==1 && io.pending.empty());
		for (unsigned w=0;w<shape.second;w++) assert(io.memory[(shape.first-0x74000)/4+w]==input[w]);
		IO checked; checked.corrupt=true;
		assert((!PioneerQ38WorkStage<IO,false>(checked,shape.first,input.data(),shape.second,0)) && checked.reads==shape.second);
	}
	IO frame;
	assert((PioneerQ38WorkStage<IO,true>(frame,0xa0000,input.data(),5803,0)));
	assert((PioneerQ38WorkStage<IO,true>(frame,0xab000,input.data(),80,0)));
	for (unsigned w=0;w<119;w++) assert((PioneerQ38WorkStage<IO,true>(frame,0x74080+w*32,input.data(),8,0)));
	assert((PioneerQ38WorkStage<IO,true>(frame,0x74000,input.data(),16,0)));
	assert(!frame.reads && frame.pending.size()==5803+80+119*8+16);
	assert(PioneerQ38WorkDrain(frame,0) && frame.reads==1 && frame.pending.empty());
	for (auto bad:{std::pair<uint32_t,unsigned>{0x6000,16},{0xffb20000,16},{0x74004,16},{0x74000,15},{0x74080,9},{0x74f60,8},{0xa0000,5804},{0xab000,81}}) {
		IO io; assert((!PioneerQ38WorkStage<IO,true>(io,bad.first,input.data(),bad.second,0)) && !io.reads && !io.writes);
	}
	IO absent; absent.stable=false;
	assert((!PioneerQ38WorkStage<IO,true>(absent,0x74000,input.data(),16,0)) && !PioneerQ38WorkDrain(absent,0) && !absent.reads && !absent.writes);
	IO null; assert((!PioneerQ38WorkStage<IO,true>(null,0x74000,nullptr,16,0)) && !null.reads && !null.writes);
}
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	CheckPosted<PostedIO>(); CheckPosted<PostedPairs>();
	puts("PASS WORK posted data staging:ALL fixed shapes,odd tails,32/64-bit writes,zero numeric readback,ONE drain after complete119-task job frame;CPU fences alone deliberately leave writes pending;checked stage retains full comparison,invalid/control/code ranges reject BEFORE IO;NO DEVICE IO");
}
