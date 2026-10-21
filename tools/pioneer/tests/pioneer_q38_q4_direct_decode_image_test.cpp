/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
// Exact new immutable helper window only. Original opcode/replay/hazard
// interpreter is unchanged;not a general code-range/unsupported-ISA waiver.
struct DirectQ4Peer:DirectSrcBImage {
	std::vector<uint8_t> decoder;
	DirectQ4Peer(const char* path,unsigned t,unsigned k,unsigned g):DirectSrcBImage(path,t,k,g){
		decoder=Read((std::string(path)+"/fetch.bin").c_str());assert(decoder.size()==256);
		for(unsigned w=0;w<decoder.size()/4;w++)l1[Index(0x70000+w*4)]=Word(decoder,w*4);
	}
	bool CodeAddress(uint32_t at)const override{return Range(at,0x70000,unsigned(decoder.size()))||DirectSrcBImage::CodeAddress(at);}
	uint32_t Load(uint32_t at)override{return Range(at,0x70000,unsigned(decoder.size()))?l1[Index(at)]:DirectSrcBImage::Load(at);}
};
int main(int argc,char** argv)
{
	assert(argc==3||(argc==4&&!strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
	for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
	for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
		if(argc==4&&(k!=5120||groups!=1||!work))continue;
		printf("Q4_DIRECT_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
		DirectSrcBImage old(argv[1],type,k,groups);DirectQ4Peer next(argv[2],type,k,groups);
		assert(old.Boot()&&next.Boot());old.memory[0x74038]=next.memory[0x74038]=work;
		for(unsigned job=1;job<=2;job++) {
			if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
			old.Poison();next.Poison();old.PoisonTail();next.PoisonTail();
			unsigned oldOps=old.vectorOps,nextOps=next.vectorOps,before=old.steps,after=next.steps;
			unsigned oldRecords=old.replayRecords,nextRecords=next.replayRecords,oldPlays=old.replayExpansions,nextPlays=next.replayExpansions;
			assert(old.Job(job)&&next.Job(job));
			assert(old.requests==next.requests&&old.responses==next.responses&&old.matrixInstructions==next.matrixInstructions
				&&old.rawReads==next.rawReads&&old.rawUnpacks==next.rawUnpacks&&old.drains==next.drains
				&&old.sumWrites==next.sumWrites&&old.sumReads==next.sumReads&&old.dotProbeReads==next.dotProbeReads
				&&old.affineRecords==next.affineRecords&&old.affineExpansions==next.affineExpansions
				&&old.paddingWrites==next.paddingWrites&&old.baseWrites==next.baseWrites&&old.copyWrites==next.copyWrites
				&&old.directReads==next.directReads&&!next.recordRemaining&&next.phaseQueue.empty()&&next.pendingAccumulator.empty());
			unsigned panels=work&&type==12?groups*(k/16):0;
			assert(old.replayRecords-oldRecords==next.replayRecords-nextRecords+panels);
			assert(old.replayExpansions-oldPlays==next.replayExpansions-nextPlays+panels*7);
			assert(old.vectorOps-oldOps==next.vectorOps-nextOps+panels*23/2);
			for(unsigned w=0;w<64;w++)assert(old.memory.at(0x83f00+w*4)==next.memory.at(0x83f00+w*4));
			for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
			if(work&&type==12)printf("OFFLINE_Q4_DIRECT RV%u/%u SFPU%u/%u replay_removed%u;NO_NATIVE_RATE_CLAIM\n",old.steps-before,next.steps-after,old.vectorOps-oldOps,next.vectorOps-nextOps,panels*8);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2)&&next.Job(3,2));
	}
	for(unsigned fault:{1U,5U,40U,208U}) {
		DirectQ4Peer bad(argv[2],12,17408,3);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.faultAt=fault;
		assert(!bad.Job(1)&&bad.pending&&bad.requests==fault&&bad.responses==fault-1&&!bad.memory.at(0x74144));
		unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
	}
	puts(argc==4?"PASS PARTIAL direct Q4 pilots;FULL SUITE SKIPPED":"PASS OFFLINE compiled direct Q4 peer ALL54 shapes/two changed inputs/STOP;original FP32+independent FP64/all64proofs,identical unpack/read/drain/matrix/NoC/source sequence,only Work Q4 decode/replay issue changed;NoC failures inert;NO native/driver/throughput qualification");
}
