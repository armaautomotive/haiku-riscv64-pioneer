/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
// Isolated opcode model: the original compiled-image/replay whitelist is
// UNCHANGED. Only these two exact register ops, inside a fresh Work Q8/Q6
// panel with all original source/config/queue proofs, get this extension.
struct SignedMagnitudePeer:DirectSrcBImage {
	std::vector<uint8_t> decoder;unsigned absolutes=0,signs=0;
	SignedMagnitudePeer(const char* path,unsigned t,unsigned k,unsigned g):DirectSrcBImage(path,t,k,g){
		decoder=Read((std::string(path)+"/fetch.bin").c_str());assert(!decoder.empty()&&decoder.size()<=0x700&&!(decoder.size()&3));
		for(unsigned w=0;w<decoder.size()/4;w++)l1[Index(0x70000+w*4)]=Word(decoder,w*4);
	}
	bool CodeAddress(uint32_t at)const override{return Range(at,0x70000,unsigned(decoder.size()))||DirectSrcBImage::CodeAddress(at);}
	uint32_t Load(uint32_t at)override{return Range(at,0x70000,unsigned(decoder.size()))?l1[Index(at)]:DirectSrcBImage::Load(at);}
	void Instruction(uint32_t op)override {
		if(op!=0x7d000020&&op!=0x89000200){DirectSrcBImage::Instruction(op);return;}
		assert(Active()&&(type==8||type==14)&&!affineRecord);
		if(recordRemaining){
			assert(recordNext<32);replay[recordNext]=op;replayDefined[recordNext++]=!dropReplayRecord;
			recordRemaining--;if(!recordExecute)return;
		}
		assert(enabled&&!rawPending&&!pending&&phaseQueue.empty()&&pendingAccumulator.empty()
			&&ownerA&&sources[mathBank]&&!affineEligible&&thread[2]==1&&thread[3]==1&&thread[5]==4
			&&PioneerQ38VectorFixedMatrixEquivalent(config[0],config[1],thread[2],thread[3])&&config[220]==0);
		for(unsigned lane=0;lane<32;lane++) {
			if(op==0x7d000020){
				int64_t v=int32_t(lr[0][lane]);assert(v>=(type==8?-128:-32)&&v<=(type==8?127:31));
				lr[2][lane]=uint32_t(v<0?-v:v);
			}else {
				uint32_t oldSign=lr[0][lane]&0x80000000U,mag=lr[2][lane];
				assert(mag<=(type==8?128U:32U));lr[0][lane]=oldSign|mag;
			}
		}
		if(op==0x7d000020)absolutes++;else signs++;
		vectorOps++;drained=false;
	}
};
int main(int argc,char** argv)
{
	assert(argc==3||(argc==4&&!strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
	for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
	for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
		if(argc==4&&(k!=5120||groups!=1||!work))continue;
		printf("SIGNED_MAG2_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
		DirectSrcBImage old(argv[1],type,k,groups);SignedMagnitudePeer next(argv[2],type,k,groups);
		assert(old.Boot()&&next.Boot());old.memory[0x74038]=next.memory[0x74038]=work;
		for(unsigned job=1;job<=2;job++) {
			if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
			old.Poison();next.Poison();old.PoisonTail();next.PoisonTail();
			unsigned oldOps=old.vectorOps,nextOps=next.vectorOps,before=old.steps,after=next.steps;
			assert(old.Job(job)&&next.Job(job));
			assert(old.requests==next.requests&&old.responses==next.responses&&old.matrixInstructions==next.matrixInstructions
				&&old.rawReads==next.rawReads&&old.rawUnpacks==next.rawUnpacks&&old.drains==next.drains
				&&old.sumWrites==next.sumWrites&&old.sumReads==next.sumReads&&old.dotProbeReads==next.dotProbeReads
				&&old.replayRecords==next.replayRecords&&old.replayExpansions==next.replayExpansions
				&&old.paddingWrites==next.paddingWrites&&old.baseWrites==next.baseWrites&&old.copyWrites==next.copyWrites
				&&old.directReads==next.directReads&&!next.recordRemaining&&next.phaseQueue.empty()&&next.pendingAccumulator.empty());
			unsigned decodes=work&&type!=12?groups*(k/16)*8:0;
			assert(old.vectorOps-oldOps==next.vectorOps-nextOps+decodes*6&&next.absolutes==job*decodes&&next.signs==job*decodes);
			for(unsigned w=0;w<64;w++)assert(old.memory.at(0x83f00+w*4)==next.memory.at(0x83f00+w*4));
			for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if(work&&type!=12)printf("OFFLINE_SIGNED_MAG2 RV%u/%u SFPU%u/%u;NO_NATIVE_RATE_CLAIM\n",old.steps-before,next.steps-after,old.vectorOps-oldOps,next.vectorOps-nextOps);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2)&&next.Job(3,2));
	}
	for(unsigned type:{8U,14U})for(unsigned fault:{1U,5U,40U,208U}) {
		SignedMagnitudePeer bad(argv[2],type,17408,3);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.faultAt=fault;
		assert(!bad.Job(1)&&bad.pending&&bad.requests==fault&&bad.responses==fault-1&&!bad.memory.at(0x74144));
		unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
	}
	puts(argc==4?"PASS PARTIAL signed magnitude2 pilots;FULL SUITE SKIPPED":"PASS OFFLINE compiled signed magnitude2 ALL54 shapes/two changed inputs/STOP;original FP32+independent FP64/all64proofs,identical unpack/read/drain/matrix/NoC/source sequence;ONLY Work Q8/Q6 signed decoding shortened;NoC failures inert;NO native/driver/throughput qualification");
}
