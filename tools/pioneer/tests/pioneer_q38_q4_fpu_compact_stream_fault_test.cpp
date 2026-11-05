/* SPDX-License-Identifier: MIT */
#define PIONEER_Q4_COMPACT_STREAM_IMAGE_ONLY
#include "pioneer_q38_q4_fpu_compact_stream_image_test.cpp"
static void InertPark(CompactStreamRV32& image){
 assert(image.pc==0x70058 && !image.stores && !image.native.outputReads);
 const unsigned reads=image.bus.reads,writes=image.bus.writes,math=image.native.matrixOps,issues=image.bus.issues;
 for(unsigned i=0;i<2048;i++){image.Step();image.Progress();}
 // Autonomous replies may still arrive. Do NOT rewind counters/reclaim
 // buffers or falsely demand the failed engine has become idle.
 assert(image.pc==0x70058 && image.bus.reads==reads && image.bus.writes==writes && image.bus.issues==issues
  && image.native.matrixOps==math && !image.stores && !image.native.outputReads);
}
int main(int argc,char** argv) {
	assert(argc==2);static_cast<void>(&NoMore);unsigned refused=0,numeric=0;CompactStreamData data(5120,0);
	for(unsigned fault:{1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,16U}) {
		auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);image->bus.fault=fault;
		assert(!image->Execute() && !image->stores && !image->native.outputReads);if(image->pc==0x70058)InertPark(*image);refused++;
	}
	for(unsigned field=0;field<8;field++) {
		auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);
		unsigned value=image->ReadL1(SL::Frame+field*4);image->WriteL1(SL::Frame+field*4,field<4?value^1:field==4?0:1);
		bool ran=image->Execute();
		// The real fault park has a FENCE,not an NIU read/write. Keep
		// counting it for no-more-IO checks,without mislabeling it a request.
		assert(!ran && !image->bus.reads && !image->bus.writes && !image->native.matrixOps && !image->stores);if(image->pc==0x70058)InertPark(*image);refused++;
	}
	for(unsigned config:{72U,112U}) {
		auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);image->dropConfig=config;
		assert(!image->Execute() && !image->stores && !image->native.metadataLoads);if(image->pc==0x70058)InertPark(*image);refused++;
	}
	for(unsigned ack:{1U,100U}) {
		auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);image->dropAckAt=ack;
		assert(!image->Execute() && !image->stores && !image->native.outputReads);if(image->pc==0x70058)InertPark(*image);refused++;
	}
	for(unsigned which:{1U,2U}) {
		auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);image->native.dropProducer=which;bool unknown=false;
		try{image->Execute();}catch(uint32_t marker){unknown=marker==NativeBfpMachine::Unknown;}
		assert(unknown && !image->stores && !image->native.outputReads);if(image->pc==0x70058)InertPark(*image);refused++;
	}
	for(unsigned missing:{1U,2U}) {
		auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);
		if(missing==1)image->native.skipZero=true;else image->native.skipMove=1;bool unknown=false;
		try{image->Execute();}catch(Q4FpuPipelineMachine::Fault){unknown=true;}
		assert(unknown && !image->stores && !image->native.outputReads);if(image->pc==0x70058)InertPark(*image);refused++;
	}
	for(unsigned missing:{1U,2U}){
  auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);
  if(missing==1)image->native.skipExtraZero=true;else image->native.dropCompactImport=1;
  bool unknown=false;try{image->Execute();}catch(Q4FpuPipelineMachine::Fault){unknown=true;}
  assert(unknown && !image->stores && !image->native.outputReads);refused++;
 }
 for(unsigned store:{1U,2U,71U,72U}){
  auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);image->native.dropCompactStore=store;
  assert(image->Execute() && !image->Matches(data.expected));numeric++;
 }
	for(unsigned phase=0;phase<4;phase++) {
		auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);image->native.dropPhase=phase;
		assert(image->Execute() && !image->Matches(data.expected));numeric++;
	}
	{
		auto image=std::make_unique<CompactStreamRV32>(argv[1],5120);data.Stage(image->bus);
		memset(image->bus.dram.data()+SL::DramInput-SL::DramWeight,0,5120*2);
		assert(image->Execute() && !image->Matches(data.expected));numeric++;
	}
	printf("PASS actual RV32 compact Q4 stream:%u unknown/frame/NIU/config/completion/producer failures refuse before output,%u numeric controls(missing metadata stores/four phases/zero input) fail original reference;unchanged Step,retained autonomous replies NOT reclaimed/reset/retried;NO native/hardware/quality/speed proof\n",refused,numeric);
}

