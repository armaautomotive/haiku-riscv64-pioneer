/* SPDX-License-Identifier: MIT */
#define PIONEER_Q8_STREAM_LOADER_ONLY
#include "pioneer_q38_q8_fpu_stream_loader_test.cpp"
#define PIONEER_Q8_PIPELINE_FIXTURE_ONLY
#include "pioneer_q38_q8_fpu_pipeline_test.cpp"
#include <memory>
struct StreamPipeline:Q8FpuPipelineMachine {
	StreamIO* bus=nullptr;unsigned blocks=0;mutable unsigned readerStart=0;
	unsigned OperandOffset()const override{return ((inputLoads/8)&1)*0x10000;}
	bool OperandReady(unsigned address,unsigned bytes)const override {
		if(address==0x97000)return bytes==1024;
		unsigned slot=(inputLoads/8)&1;
		bool valid=bus && bus->ready[slot] && bus->copies[slot]==56 && bus->dataBlock[slot]==(inputLoads/8)%blocks
			&& address>=0x90000+slot*0x10000 && address+bytes<=0x963c0+slot*0x10000;
		if(valid && address==SL::Metadata+slot*0x10000 && !(metadataLoads%16)){bus->reader=slot;readerStart=matrixOps;}
		return valid && bus->reader==slot;
	}
};
struct StreamEmitter {
	StreamPipeline& machine;StreamIO& bus;
	void Op(unsigned op){machine.Op(op);bus.Progress();}void Config(unsigned i,unsigned v){machine.Config(i,v);bus.Progress();}
	void Drain(){machine.Drain();bus.Progress();}
};
struct StreamData {
	std::vector<Q8PipelineFixture> blocks;std::array<uint32_t,32> expected{};
	explicit StreamData(unsigned width,unsigned trial) {
		for(unsigned k=0;k<width/256;k++)blocks.emplace_back(trial+k);
		Refresh();
	}
	void Refresh() {
		expected.fill(0);for(auto& block:blocks)block.Refresh();
		for(const auto& block:blocks)for(unsigned row=0;row<32;row++) {
			float weight[256];block.Weights(row,weight);
			for(unsigned first=0;first<256;first+=16) {
				uint16_t a[16],b[16];for(unsigned i=0;i<16;i++){a[i]=uint16_t(MixedRound(PioneerFmaBits(weight[first+i]))>>16);b[i]=block.input[first+i];}
				for(unsigned phase=0;phase<4;phase++)assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],expected[row]));
			}
		}
	}
	void Stage(StreamIO& io)const {
		assert(io.width/256==blocks.size());for(unsigned k=0;k<blocks.size();k++) {
			memcpy(io.dram.data()+k*9216,blocks[k].prepared.data(),9216);
			for(unsigned i=0;i<256;i++){unsigned a=SL::DramInput-SL::DramWeight+k*512+i*2;io.dram[a]=uint8_t(blocks[k].input[i]);io.dram[a+1]=uint8_t(blocks[k].input[i]>>8);}
		}
		memset(io.l1.data()+SL::Zero-64,0,1088);
	}
};
#ifndef PIONEER_Q8_STREAM_PIPELINE_ONLY
int main() {
	unsigned passed=0;
	for(unsigned width:{5120U,6144U}) {
		StreamData data(width,0);auto machine=std::make_unique<StreamPipeline>();machine->blocks=width/256;
		std::fill(machine->l1.begin(),machine->l1.end(),0xa7);machine->delay[0]=100003;machine->delay[1]=7;
		for(unsigned pass=0;pass<2;pass++) {
			StreamIO io(width,17|(12<<6),&machine->l1);machine->bus=&io;data.Stage(io);Loader loader;StreamEmitter emitter{*machine,io};
			assert(loader.Begin(io,width,io.route) && loader.Open(io,0,0));for(unsigned s=0;s<4;s++)assert(loader.Slice(io));assert(loader.Wait(io));
			PioneerQ38Q8FpuStreamCompute::Begin(emitter);
			for(unsigned block=0;block<width/256;block++) {
				unsigned buffer=block&1;bool next=block+1<width/256;assert(loader.Acquire(block,buffer));io.reader=buffer;
				if(next)assert(loader.Open(io,block+1,buffer^1));
				for(unsigned group=0;group<8;group++){if(next && group<4)assert(loader.Slice(io));assert(PioneerQ38Q8FpuStreamCompute::Group(emitter,buffer,group));}
				io.reader=2;assert(loader.Release(block,buffer));if(next)assert(loader.Wait(io));
			}
			assert(loader.Complete(io) && machine->Clean() && machine->Results()==data.expected && io.overlap);
			assert(machine->zeroLoads==5*(pass+1));NoMore(loader,io);passed++;
		}
		printf("PASS OFFBOARD emitted Q8 fullK%u two_jobs/all64 exact,ONE initialization/job,canonical dual L1 buffers from actual NoC responses,FIFO/counter/fence-qualified consumer reads,prefetch during math;NO host payload loop or GDDR writes;NOT native/compiled-image/hardware/quality/speed proof\n",width);
	}
	{
		StreamData data(5120,3);auto machine=std::make_unique<StreamPipeline>();machine->blocks=20;StreamIO io(5120,17|(12<<6),&machine->l1);machine->bus=&io;data.Stage(io);
		Loader loader;assert(loader.Begin(io,5120,io.route) && loader.Open(io,0,0));for(unsigned s=0;s<4;s++)assert(loader.Slice(io));
		StreamEmitter emitter{*machine,io};PioneerQ38Q8FpuStreamCompute::Begin(emitter);bool refused=false;
		try{PioneerQ38Q8FpuStreamCompute::Group(emitter,0,0);}catch(Q8FpuPipelineMachine::Fault){refused=true;}
		assert(refused && !machine->matrixOps && !machine->metadataLoads && !machine->outputReads);
	}
	assert(passed==4);puts("PASS OFFBOARD streaming producer-completion omission refuses BEFORE any metadata/weight arithmetic;NOT native authorization");
}
#endif
