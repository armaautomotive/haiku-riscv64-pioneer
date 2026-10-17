/* SPDX-License-Identifier: MIT */
#define PIONEER_TABLE_DECODER_MACHINE_ONLY
#include "pioneer_table_decoder_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderBatch.h"
struct BatchDecoder:TableDecoder {
	unsigned requested,expectedSteps,selected[2]={},selectedBits[2]={};
	unsigned ropeReads=0,embeddingReads=0,failInput=0,corruptInput=0,inputPackets=0;
	BatchDecoder(const char* root,const DecoderSfpuImage& image,bool nz,unsigned steps,bool stop=false):TableDecoder(root,image,nz,true),requested(steps),expectedSteps(stop?1:steps) {
		memory[TT_DECODER_JOB_BASE+4]=10; memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,reserved)]=steps;
		std::vector<float> first(1024); for (unsigned i=0;i<1024;i++) first[i]=Float(memory.at(TT_DECODER_INPUT+i*4));
		selected[0]=Best(); selectedBits[0]=nz?Bits(expected[selected[0]]):0;
		if (stop) {
			assert(selected[0]<151643); unsigned ids[]={selected[0],151643,151645},word0=0; uint32_t word1=0;
			assert(PioneerDecoderPackEog(ids,3,word0,word1));
			memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,reserved)+8]=word0;
			memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,reserved)+12]=word1;
		}
		if (expectedSteps==2) {
			std::vector<float> embedding(1024,0); if (nz) embedding[(selected[0]*13+49)%1024]=selected[0]%3==0?-.25f:.25f;
			positions=2; SetRope(1); if (nz) Reference(&embedding);
			selected[1]=Best(); selectedBits[1]=nz?Bits(expected[selected[1]]):0;
			positions=1; SetRope(0);
		}
		for (unsigned i=0;i<1024;i++) memory[TT_DECODER_INPUT+i*4]=Bits(first[i]);
	}
	unsigned Best() const { unsigned best=0; if (nonzero) for (unsigned i=1;i<expected.size();i++) if (expected[i]>expected[best]) best=i; return best; }
	void SetRope(unsigned position) {
		float theta=float(position),step=powf(1000000.0f,-2.0f/128);
		for (unsigned i=0;i<64;i++) { memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope)+i*8]=Bits(cosf(theta));
			memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope)+i*8+4]=Bits(sinf(theta)); theta*=step; }
	}
	void Store(uint32_t a,uint32_t value) override {
		if (a==TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,tokens)) { assert(value==positions+1 && value<=2); positions=value; memory[a]=value; return; }
		if (Range(a,0x146c00,256)) { memory[a]=value; return; }
		TableDecoder::Store(a,value);
	}
	void Submit() override {
		unsigned source=command[0],bytes=command[8];
		if (!command[7] && command[2]==786 && command[3]==0x80000) {
			assert(!command[1] && !command[4] && command[5]==129 && !command[6] && !command[9] && !(source&31));
			inputPackets++; transfers++; if (inputPackets==failInput) { pending=true; return; }
			if (bytes==512) {
				assert(source==0x31000000+(positions-1)*512); ropeReads++;
				float theta=float(positions-1),step=powf(1000000.0f,-2.0f/128);
				for (unsigned i=0;i<64;i++) { memory[0x80000+i*8]=Bits(cosf(theta)); memory[0x80004+i*8]=Bits(sinf(theta)); theta*=step; }
			} else {
				assert(bytes==576 && positions==1); unsigned token=selected[0],tile=(token>>4)*16,block=(embeddingReads>>1);
				assert(source==WeightBase(7)+(token>>4)*18432+block*576 && block<32); embeddingReads++;
				uint32_t words[144]={};
				for (unsigned row=0;row<16;row++) {
					unsigned at=((tile+row)*13+49)%1024;
					words[row]=nonzero?Bits(.25f):0;
					if (nonzero && at/32==block) { unsigned byte=64+(at%32)*16+row; words[byte/4]|=((tile+row)%3==0?129U:1U)<<((byte&3)*8); }
				}
				for (unsigned i=0;i<144;i++) memory[0x80000+i*4]=words[i];
			}
			if (inputPackets==corruptInput) memory[0x80000+bytes-4]^=1;
			comparing=false; counts[0]++; counts[1]+=bytes/64; counts[2]++; counts[6]=counts[2]; counts[3]++;
			return;
		}
		TableDecoder::Submit();
	}
	void VerifyBatch() {
		assert(positions==expectedSteps && issued==9219*positions && lookups==issued && hostWrites==1 && hostReads==1 && originalWords==64 && returnedWords==64);
		assert(ropeReads==positions*2 && embeddingReads==(positions-1)*64 && bankReads==113*positions);
		unsigned count=0; assert(PioneerDecoderBatchPacket(wide.data(),1,1,requested,memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,reserved)+8),memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,reserved)+12),count) && count==positions);
		for (unsigned i=0;i<positions;i++) assert(wide[8+i]==selected[i] && wide[24+i]==selectedBits[i]);
		for (unsigned w=0;w<119;w++) assert(sequence[w]==PioneerDecoderSequenceAdvance(w,true,true,true)*positions
			&& memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,sequence)+w*4)==sequence[w]+1);
		if (nonzero) for (unsigned layer=0;layer<28;layer++) for (unsigned t=0;t<positions;t++) for (unsigned g=0;g<8;g++) for (unsigned i=0;i<64;i++) {
			unsigned p=t*1024+g*128+i*2; assert(kv.at(0x82400000+layer*0x100000+g*0x20000+t*256+i*4)==(expectedK[layer][p]|(uint32_t(expectedK[layer][p+1])<<16)));
			assert(kv.at(0x82410000+layer*0x100000+g*0x20000+t*256+i*4)==(expectedV[layer][p]|(uint32_t(expectedV[layer][p+1])<<16))); }
		uint32_t summary[24],before[8],after[8]; for (unsigned i=0;i<24;i++) summary[i]=memory.at(0x143000+i*4);
		for (unsigned i=0;i<8;i++) { before[i]=summary[8+i]; after[i]=summary[16+i]; }
		assert(PioneerDecoderBatchSummary(summary,positions,1,before,after,getenv("PIONEER_GREEDY_FUSED_GATHER")!=nullptr));
		tt_decoder_stage_profile profile={}; for (unsigned i=0;i<32;i++) reinterpret_cast<uint32_t*>(&profile)[i]=memory.at(TT_DECODER_STAGE_L1+i*4);
		assert(PioneerDecoderStageValid(profile,1,positions,5,positions));
		printf("PASS OFFLINE actual batch positions%u/%u,119-peer sequences/whole KV/exact token bits/one256B host pair/profile/counters,steps%u;NOT native\n",positions,requested,steps); fflush(stdout);
	}
};
struct BatchDebugDecoder:BatchDecoder {
	BatchDebugDecoder(const char* root,const DecoderSfpuImage& image,bool nz,bool small):BatchDecoder(root,image,nz,1) { memory[TT_DECODER_JOB_BASE+4]=small?9:8; }
	void VerifyDebug() { TableDecoder::Verify(); VerifyProfile(); assert(lookups==9219 && ropeReads==2 && !embeddingReads); }
};
#ifndef PIONEER_BATCH_DECODER_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true;
	for (const char* key:{"PIONEER_COORDINATED_PULL_INPUT","PIONEER_ATTENTION_PARALLEL_ROPE","PIONEER_GREEDY_PAIRED_FFN","PIONEER_GREEDY_DESCRIPTOR_TABLE","PIONEER_GREEDY_BATCH"}) setenv(key,"1",1);
	DecoderSfpuImage image(argv[2],false,true,true);
	if (!getenv("PIONEER_BATCH_FAULTS_ONLY")) {
	for (bool nonzero:{false,true}) for (bool small:{false,true}) { BatchDebugDecoder m(argv[1],image,nonzero,small); assert(m.RunDecoder()); m.VerifyDebug(); }
	for (bool nonzero:{false,true}) for (unsigned steps:{1U,2U}) { BatchDecoder m(argv[1],image,nonzero,steps); assert(m.RunDecoder()); m.VerifyBatch(); }
	for (bool nonzero:{false,true}) { BatchDecoder m(argv[1],image,nonzero,2,true); assert(m.RunDecoder()); m.VerifyBatch(); }
	}
	for (unsigned fault:{1U,2U,3U,4U}) { BatchDecoder m(argv[1],image,true,2); if (fault<3) m.failInput=fault; else m.corruptInput=fault-2;
		assert(!m.RunDecoder() && !m.hostWrites); }
	puts("PASS OFFLINE actual two-position decoder/card-side tied embedding/immutable RoPE and local sampling,independent CPU sparse model,NO host call between positions,early EOS/input faults;NOT wired/native qualification");
}
#endif
