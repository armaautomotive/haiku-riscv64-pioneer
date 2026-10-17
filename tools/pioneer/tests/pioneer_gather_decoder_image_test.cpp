/* SPDX-License-Identifier: MIT */
#define PIONEER_BATCH_DECODER_MACHINE_ONLY
#include "pioneer_batch_decoder_image_test.cpp"
struct GatherDecoder:BatchDecoder {
	unsigned gathers=0,badCombinedProof=0,badRestore=0,lastPrefix=0,lastWorker=0;
	bool idleProof[119]={};
	GatherDecoder(const char* root,const DecoderSfpuImage& image,bool nz,unsigned steps,bool stop=false):BatchDecoder(root,image,nz,steps,stop) {
		for (unsigned i=0;i<16;i++) memory[0x83fc0+i*4]=0x123cafe;
	}
	void Store(uint32_t a,uint32_t value) override {
		if (badRestore && operation==10 && lastWorker==1 && a==lastPrefix+(badRestore-1)*4) value^=1;
		BatchDecoder::Store(a,value);
	}
	uint32_t PeerRead(unsigned w,uint32_t a) override {
		if (a==0x71304) idleProof[w-1]=true;
		return BatchDecoder::PeerRead(w,a);
	}
	void PeerWrite(unsigned w,uint32_t a,uint32_t value) override {
		if (a==0x71300) idleProof[w-1]=false;
		BatchDecoder::PeerWrite(w,a,value);
	}
	void Submit() override {
		if (!command[7] && command[0]==0x83fc0) {
			unsigned w=1; while (w<=119 && command[2]!=(PioneerWorkerX(w)|(PioneerWorkerY(w)<<6))) w++;
			assert(w<=119 && command[5]==129 && !command[1] && !command[4] && !command[6] && !command[9]);
			operation=memory.at(0x140004); k=memory.at(0x14000c); n=memory.at(0x140010); limit=119;
			currentPhase=Phase(memory.at(0x140014)); unsigned rows=descriptor[w-1][2]*16;
			for (unsigned p=0;p<std::min(n/32,119U);p++) assert(idleProof[p]);
			unsigned first=(descriptor[w-1][0]-memory.at(0x140014))/(k+k/8),destination=0x84000+first*4;
			if (operation==6 || operation==9 || operation==11) destination=0x84000;
			if (operation==10) { PioneerPairedFfnSpan span; assert(PioneerPairedFfnWorker(w,span)); destination=(span.tensor?0x84000:0x87000)+span.first*4; }
			assert(rows && rows<=1312 && command[3]+64==destination && command[8]==rows*4+64 && destination+rows*4<=0x8a000);
			transfers++; gathers++; lastPrefix=command[3]; lastWorker=w;
			for (unsigned i=0;i<16;i++) memory[lastPrefix+i*4]=PeerRead(w,0x71340+i*4)^(i+1==badCombinedProof?1:0);
			for (unsigned i=0;i<rows;i++) memory[destination+i*4]=PeerRead(w,0x84000+i*4);
			counts[0]++; counts[1]+=command[8]/64; counts[2]++; counts[3]++; counts[6]=counts[2]; comparing=false;
			return;
		}
		BatchDecoder::Submit();
	}
};
#ifndef PIONEER_GATHER_DECODER_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true;
	for (const char* key:{"PIONEER_COORDINATED_PULL_INPUT","PIONEER_ATTENTION_PARALLEL_ROPE","PIONEER_GREEDY_PAIRED_FFN","PIONEER_GREEDY_DESCRIPTOR_TABLE","PIONEER_GREEDY_BATCH","PIONEER_GREEDY_FUSED_GATHER"}) setenv(key,"1",1);
	DecoderSfpuImage image(argv[2],false,true,true);
	for (bool nz:{false,true}) for (unsigned mode:{0U,1U,2U,3U}) {
		GatherDecoder m(argv[1],image,nz,mode==3?2:1);
		if (mode<2) m.memory[TT_DECODER_JOB_BASE+4]=mode==0?8:9;
		bool okay=m.RunDecoder();
		if (!okay || m.gathers!=8827*m.positions) { printf("GATHER TRACE mode%u nz%u gathers%u issued%u phase%u ra%08x prefix%08x\n",mode,nz,m.gathers,m.issued,m.currentPhase,m.regs[1],m.lastPrefix); fflush(stdout); }
		assert(okay && m.gathers==8827*m.positions);
		if (mode<2) { m.TableDecoder::Verify(); m.VerifyProfile(); } else m.VerifyBatch();
	}
	for (bool nz:{false,true}) { GatherDecoder m(argv[1],image,nz,2,true); assert(m.RunDecoder()); m.VerifyBatch(); }
	for (unsigned word=1;word<=16;word++) for (bool restore:{false,true}) {
		GatherDecoder m(argv[1],image,true,2); if (restore) m.badRestore=word; else m.badCombinedProof=word;
		assert(!m.RunDecoder() && !m.hostWrites && m.gathers && (restore || m.gathers==1));
	}
	puts("PASS OFFLINE actual combined-gather decoder,full/small/one+two-position zero+sparse,exact logits/KV/119 sequences/unchanged flits,ALL16 bad proofs+prefix restoration faults before host output;NOT wired/native");
}
#endif
