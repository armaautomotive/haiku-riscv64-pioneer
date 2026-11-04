/* SPDX-License-Identifier: MIT */
#define PIONEER_Q6_STREAM_IMAGE_ONLY
#include "pioneer_q38_q6_fpu_stream_image_test.cpp"
int main(int argc,char** argv) {
	assert(argc==2);static_cast<void>(&NoMore);unsigned refused=0,numeric=0;StreamData data(5120,0);
	for(unsigned fault:{1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,16U}) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);image->bus.fault=fault;
		assert(!image->Execute() && !image->stores && !image->native.outputReads);refused++;
	}
	for(unsigned field=0;field<8;field++) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);
		unsigned value=image->ReadL1(SL::Frame+field*4);image->WriteL1(SL::Frame+field*4,field<4?value^1:field==4?0:1);
		bool ran=image->Execute();
		// The real fault park has a FENCE,not an NIU read/write. Keep
		// counting it for no-more-IO checks,without mislabeling it a request.
		assert(!ran && !image->bus.reads && !image->bus.writes && !image->native.matrixOps && !image->stores);refused++;
	}
	for(unsigned config:{72U,112U}) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);image->dropConfig=config;
		assert(!image->Execute() && !image->stores && !image->native.metadataLoads);refused++;
	}
	for(unsigned ack:{1U,100U}) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);image->dropAckAt=ack;
		assert(!image->Execute() && !image->stores && !image->native.outputReads);refused++;
	}
	for(unsigned which:{1U,2U}) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);image->native.dropProducer=which;bool unknown=false;
		try{image->Execute();}catch(uint32_t marker){unknown=marker==NativeBfpMachine::Unknown;}
		assert(unknown && !image->stores && !image->native.outputReads);refused++;
	}
	for(unsigned missing:{1U,2U}) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);
		if(missing==1)image->native.skipZero=true;else image->native.skipMove=1;bool unknown=false;
		try{image->Execute();}catch(Q6FpuPipelineMachine::Fault){unknown=true;}
		assert(unknown && !image->stores && !image->native.outputReads);refused++;
	}
	for(unsigned phase=0;phase<4;phase++) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);image->native.dropPhase=phase;
		assert(image->Execute() && !image->Matches(data.expected));numeric++;
	}
	{
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);
		memset(image->bus.dram.data()+SL::DramInput-SL::DramWeight,0,5120*2);
		assert(image->Execute() && !image->Matches(data.expected));numeric++;
	}
	for(unsigned missing:{14U,17U}) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);image->native.skipUnpack=missing;bool unknown=false;
		try{image->Execute();}catch(Q6FpuPipelineMachine::Fault){unknown=true;}
		assert(unknown && !image->stores && !image->native.outputReads);refused++;
	}
	for(unsigned corrupt=0;corrupt<2;corrupt++) {
		auto image=std::make_unique<StreamRV32>(argv[1],5120);data.Stage(image->bus);
		for(unsigned block=0;block<20;block++) {
			uint8_t* at=image->bus.dram.data()+block*8192;
			if(!corrupt)memset(at+4096,0,2048);
			else for(unsigned group=1;group<16;group+=2)memcpy(at+6144+group*128,at+6144+(group-1)*128,128);
		}
		assert(image->Execute() && !image->Matches(data.expected));numeric++;
	}
	printf("PASS actual RV32 Q6 stream:%u unknown/frame/NIU/config/completion/producer failures refuse before output,%u numeric controls(four missing phases+zero input+high plane+K16 scale) fail original reference;unchanged Step,retained autonomous replies NOT reclaimed/reset/retried;NO native/hardware/quality/speed proof\n",refused,numeric);
}
