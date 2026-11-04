/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_q4_fpu_pipeline_machine.h"
struct PipelineIssue {
	Q4FpuPipelineMachine& machine;
	void Op(unsigned op){machine.Op(op);}void Config(unsigned i,unsigned v){machine.Config(i,v);}void Drain(){machine.Drain();}
};
struct Q4PipelineFixture {
	std::array<uint8_t,32*144> original;std::array<uint8_t,6144> prepared;std::array<uint16_t,256> input;
	std::array<uint32_t,32> expected{};
	explicit Q4PipelineFixture(unsigned trial) {
		uint32_t seed=0x712f3b49+trial;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
		for(auto& b:original)b=uint8_t(random());
		for(unsigned row=0;row<32;row++)for(unsigned at:{0U,2U}) {
			uint8_t& high=original[row*144+at+1];if((high&0x7c)==0x7c)high^=4;
			if(trial==1){original[row*144+at]=uint8_t(row+1);high=uint8_t(row&1?0x80:0);}
		}
		for(unsigned c=0;c<256;c++)input[c]=uint16_t(0x3e80+(c*19+trial*11)%384+((c&3)==2?0x8000:0));
		Refresh();
	}
	void Refresh() {
		expected.fill(0);
		assert(PioneerQ38Q4FpuBlockData::Prepare(original.data(),original.size(),prepared.data(),prepared.size()));
		for(unsigned row=0;row<32;row++) {
			float weights[256];assert(PioneerDecodeQ4K(original.data()+row*144,weights));
			for(unsigned first=0;first<256;first+=16)for(unsigned phase=0;phase<4;phase++) {
				uint16_t a[16],b[16];for(unsigned k=0;k<16;k++){a[k]=uint16_t(Round(Bits(weights[first+k]))>>16);b[k]=input[first+k];}
				uint32_t next;assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],next));expected[row]=next;
			}
		}
	}
	static uint32_t Bits(float x){uint32_t b;memcpy(&b,&x,4);return b;}
};
#ifndef PIONEER_Q4_PIPELINE_FIXTURE_ONLY
int main()
{
	unsigned cases=0;
	for(unsigned trial=0;trial<8;trial++) {
		Q4PipelineFixture data(trial);Q4FpuPipelineMachine m;m.delay[0]=100003;m.delay[1]=7;m.Stage(data.prepared,data.input);PipelineIssue e{m};
		for(unsigned pass=0;pass<2;pass++) {
			PioneerQ38Q4FpuBlock::Run(e);assert(m.Results()==data.expected && m.Clean());cases++;
		}
		assert(m.rawLoads==64 && m.metadataLoads==64 && m.zeroLoads==10 && m.inputLoads==16 && m.moves==256 && m.transposes==64 && m.matrixOps==256);
		for(unsigned phase=0;phase<4;phase++)assert(m.visits[phase]==64);
		for(unsigned which=0;which<2;which++)assert(!m.unpackBank[which] && !m.matrixBank[which]);
	}
	printf("PASS OFFBOARD complete compressed-Q4 pipeline:%u K256 jobs across8 two-job trials,all32 outputs exact vs original-decode/BF16-RNE and rounded4-phase reference;actual L1 descriptors/physical Dst/SFPTRANSP/MOVD2A/bank handoff;NO host learned products,NoC/GDDR/native driver/hardware/model-quality/speed proof\n",cases);
}
#endif
