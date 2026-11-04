/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_q8_fpu_pipeline_machine.h"
struct PipelineIssue {
	Q8FpuPipelineMachine& machine;
	void Op(unsigned op){machine.Op(op);}void Config(unsigned i,unsigned v){machine.Config(i,v);}void Drain(){machine.Drain();}
};
struct Q8PipelineFixture {
	std::array<uint8_t,32*272> original;std::array<uint8_t,9216> prepared;std::array<uint16_t,256> input;
	std::array<uint32_t,32> expected{};unsigned rows=32;
	explicit Q8PipelineFixture(unsigned trial) {
		uint32_t seed=0x712f3b49+trial;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
		for(auto& b:original)b=uint8_t(random());
		for(unsigned row=0;row<32;row++)for(unsigned g=0;g<8;g++) {
			unsigned at=row*272+g*34;uint8_t& high=original[at+1];if((high&0x7c)==0x7c)high^=4;
			if(trial==1){original[at]=uint8_t(row+g+1);high=uint8_t(row&1?0x80:0);}
			if(trial==2){original[at]=0;high=uint8_t(row&1?0x80:0);}
			if(trial==3){original[at]=0;high=0x3c;for(unsigned i=0;i<32;i++)original[at+2+i]=uint8_t(g*32+i+row*17);}
		}
		for(unsigned c=0;c<256;c++)input[c]=uint16_t(0x3e80+(c*19+trial*11)%384+((c&3)==2?0x8000:0));
		Refresh();
	}
	void Weights(unsigned row,float* output)const {
		assert(row<32 && (rows==16 || rows==32));
		if(row>=rows){for(unsigned i=0;i<256;i++)output[i]=0;return;}
		for(unsigned g=0;g<8;g++)assert(PioneerDecodeQ8_0(original.data()+row*272+g*34,output+g*32));
	}
	void Refresh() {
		expected.fill(0);assert(PioneerQ38Q8FpuBlockData::PrepareRows(original.data(),rows*272,rows,prepared.data(),prepared.size()));
		for(unsigned row=0;row<32;row++) {
			float weights[256];Weights(row,weights);
			for(unsigned first=0;first<256;first+=16)for(unsigned phase=0;phase<4;phase++) {
				uint16_t a[16],b[16];for(unsigned i=0;i<16;i++){a[i]=uint16_t(MixedRound(PioneerFmaBits(weights[first+i]))>>16);b[i]=input[first+i];}
				assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],expected[row]));
			}
		}
	}
};
#ifndef PIONEER_Q8_PIPELINE_FIXTURE_ONLY
int main() {
	unsigned cases=0;
	for(unsigned trial=0;trial<8;trial++) {
		Q8PipelineFixture data(trial);if(trial==4){data.rows=16;data.Refresh();} // absent rows retain poison
		Q8FpuPipelineMachine m;m.delay[0]=100003;m.delay[1]=7;m.Stage(data.prepared,data.input);PipelineIssue e{m};
		for(unsigned pass=0;pass<2;pass++) {
			PioneerQ38Q8FpuStreamCompute::Begin(e);for(unsigned g=0;g<8;g++)assert(PioneerQ38Q8FpuStreamCompute::Group(e,0,g));
			assert(m.Results()==data.expected && m.Clean());cases++;
		}
		assert(m.rawLoads==64 && m.metadataLoads==32 && m.zeroLoads==10 && m.inputLoads==16 && m.moves==256 && m.transposes==64 && m.matrixOps==256);
		assert(m.rawInstructions==m.rawLoads && m.metadataInstructions==m.metadataLoads);
		for(unsigned phase=0;phase<4;phase++)assert(m.visits[phase]==64);
		for(unsigned which=0;which<2;which++)assert(!m.unpackBank[which] && !m.matrixBank[which]);
	}
	printf("PASS OFFBOARD complete original-TC8 pipeline:%u K256 jobs/eight two-job trials,all32 exact BF16-RNE/four-phase outputs,including-128/signed-zero/16-row padded tail;actual L1/physical Dst/SFPTRANSP/MOVD2A/bank handoff;NO host learned products/NoC/GDDR/native driver/hardware/quality/speed proof\n",cases);
}
#endif
