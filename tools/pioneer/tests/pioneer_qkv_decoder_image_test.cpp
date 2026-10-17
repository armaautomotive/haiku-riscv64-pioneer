/* SPDX-License-Identifier: MIT */
// OFFLINE actual private11 root and frozen attention/SFPU group bytes;
// independent zero/sparse matrix peers. No driver or native timing claim.
#define PIONEER_GATHER_DECODER_MACHINE_ONLY
#include "pioneer_gather_decoder_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQkvDecoderProof.h"
struct QkvDecoder:GatherDecoder {
	unsigned qkvBatches=0,qkvCoverage[3][2048]={};
	QkvDecoder(const char* root,const DecoderSfpuImage& image,bool nz,unsigned positions,bool stop=false):GatherDecoder(root,image,nz,positions,stop) {
		memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,version)]=11; frozen.version=11;
	}
	void Store(uint32_t a,uint32_t value) override {
		if (badRestore && operation==12 && lastWorker==1 && a==lastPrefix+(badRestore-1)*4) value^=1;
		GatherDecoder::Store(a,value);
	}
	void PeerWrite(unsigned w,uint32_t a,uint32_t value) override {
		if (a==0x71300 && memory.at(0x140004)==12) {
			PioneerDecoderDescriptor entry; PioneerPairedQkvSpan span;
			assert(PioneerQkvDescriptorEntry(frozen,w,sequence[w-1]%PioneerQkvDescriptorCount(w),entry) && PioneerPairedQkvWorker(w,span));
			assert(!entry.words[7] && entry.words[1]==64 && entry.words[2]*16==span.rows);
			unsigned phase=Phase(entry.words[0]),first=(entry.words[0]-WeightBase(phase))/1152;
			assert(phase==span.tensor && first==span.first);
			for (unsigned row=first;row<first+span.rows;row++) qkvCoverage[phase][row]++;
			if (w==1) qkvBatches++;
		}
		GatherDecoder::PeerWrite(w,a,value);
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
			if (operation==12) { PioneerPairedQkvSpan span; assert(PioneerPairedQkvWorker(w,span)); destination=PioneerPairedQkvDestination(span); }
			assert(rows && rows<=1312 && command[3]+64==destination && command[8]==rows*4+64 && destination+rows*4<=0x8a000);
			transfers++; gathers++; lastPrefix=command[3]; lastWorker=w;
			for (unsigned i=0;i<16;i++) memory[lastPrefix+i*4]=PeerRead(w,0x71340+i*4)^(i+1==badCombinedProof?1:0);
			for (unsigned i=0;i<rows;i++) memory[destination+i*4]=PeerRead(w,0x84000+i*4);
			counts[0]++; counts[1]+=command[8]/64; counts[2]++; counts[3]++; counts[6]=counts[2]; comparing=false;
			return;
		}
		BatchDecoder::Submit();
	}
	void VerifyQkv() {
		unsigned mode=memory.at(TT_DECODER_JOB_BASE+4),count=mode==10?expectedSteps:1,nonce=memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,nonce));
		assert(!pending && issued==8967*positions && lookups==issued && gathers==8575*positions
			&& qkvBatches==28*positions && pairBatches==28*positions && parallelBatches==28*positions && gluBatches==28*positions);
		for (unsigned t=0;t<3;t++) for (unsigned row=0;row<2048;row++) assert(qkvCoverage[t][row]==((!t || row<1024)?28*positions:0));
		for (auto& tensor:pairCoverage) for (unsigned hits:tensor) assert(hits==28*positions);
		for (unsigned w=0;w<119;w++) assert(sequence[w]==PioneerPairedQkvSequenceAdvance(w)*positions
			&& memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,sequence)+w*4)==sequence[w]+1);
		assert(bankReads==113*positions && !bankWrites && ropeReads==positions*2 && embeddingReads==(mode==10?(positions-1)*64:0));
		assert(hostWrites==(mode==8?119U:1U)*(mode==10?1:positions) && hostReads==hostWrites
			&& originalWords==(mode==8?151936U:mode==9?32U:64U)*(mode==10?1:positions) && returnedWords==originalWords);
		if (mode==8) { for (unsigned i=0;i<151936;i++) assert(wide[i]==(nonzero?Bits(expected[i]):0)); }
		else if (mode==9) { unsigned best=Best(); assert(PioneerDecoderGreedyValid(wide.data(),nonce,positions) && wide[3]==best && wide[4]==(nonzero?Bits(expected[best]):0)); }
		else { unsigned completed=0; assert(PioneerDecoderBatchPacket(wide.data(),nonce,1,requested,memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,reserved)+8),memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,reserved)+12),completed) && completed==count);
			for (unsigned i=0;i<count;i++) assert(wide[8+i]==selected[i] && wide[24+i]==selectedBits[i]); }
		for (unsigned i=mode==8?151936:mode==9?32:64;i<wide.size();i++) assert(wide[i]==0xffffffff);
		if (nonzero) for (unsigned layer=0;layer<28;layer++) for (unsigned t=0;t<positions;t++) for (unsigned g=0;g<8;g++) for (unsigned i=0;i<64;i++) {
			unsigned p=t*1024+g*128+i*2; assert(kv.at(0x82400000+layer*0x100000+g*0x20000+t*256+i*4)==(expectedK[layer][p]|(uint32_t(expectedK[layer][p+1])<<16)));
			assert(kv.at(0x82410000+layer*0x100000+g*0x20000+t*256+i*4)==(expectedV[layer][p]|(uint32_t(expectedV[layer][p+1])<<16))); }
		uint32_t s[24],before[8],after[8]; for (unsigned i=0;i<24;i++) s[i]=memory.at(0x143000+i*4);
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]; after[i]=s[16+i]; }
		printf("QKV actual mode%u positions%u count%u counters %u/%u/%u/%u steps%u stack%08x\n",mode,positions,count,s[0],s[1],s[2],s[3],steps,minStack); fflush(stdout);
		assert(PioneerQkvDecoderSummary(s,count,nonce,before,after,mode));
		for (unsigned word=0;word<24;word++) for (unsigned bit=0;bit<32;bit++) { auto old=s[word]; s[word]^=1U<<bit;
			assert(!PioneerQkvDecoderSummary(s,count,nonce,before,after,mode)); s[word]=old; }
		tt_decoder_stage_profile p={}; for (unsigned i=0;i<32;i++) reinterpret_cast<uint32_t*>(&p)[i]=memory.at(TT_DECODER_STAGE_L1+i*4);
		assert(PioneerQkvDecoderStageValid(p,nonce,positions,count) && minStack>=0x6e000);
	}
};
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true;
	for (const char* key:{"PIONEER_COORDINATED_PULL_INPUT","PIONEER_ATTENTION_PARALLEL_ROPE","PIONEER_GREEDY_PAIRED_FFN","PIONEER_GREEDY_DESCRIPTOR_TABLE","PIONEER_GREEDY_BATCH","PIONEER_GREEDY_FUSED_GATHER","PIONEER_GREEDY_PAIRED_QKV"}) setenv(key,"1",1);
	DecoderSfpuImage image(argv[2],false,true,true);
	for (bool nz:{false,true}) for (unsigned mode:{0U,1U,2U,3U}) {
		QkvDecoder m(argv[1],image,nz,mode==3?2:1);
		if (mode<2) m.memory[TT_DECODER_JOB_BASE+4]=mode==0?8:9;
		assert(m.RunDecoder()); m.VerifyQkv();
		if (nz && !mode) { m.Next(); assert(m.RunDecoder()); m.VerifyQkv(); }
	}
	for (bool nz:{false,true}) { QkvDecoder m(argv[1],image,nz,2,true); assert(m.RunDecoder()); m.VerifyQkv(); }
	for (unsigned word=1;word<=16;word++) for (bool restore:{false,true}) {
		QkvDecoder m(argv[1],image,true,2); if (restore) m.badRestore=word; else m.badCombinedProof=word;
		assert(!m.RunDecoder() && !m.hostWrites && m.gathers && (restore || m.gathers==1));
	}
	for (unsigned version:{0U,10U,12U,UINT32_MAX}) { QkvDecoder m(argv[1],image,false,1); m.memory[TT_DECODER_JOB_BASE+8]=version; assert(!m.RunDecoder() && !m.transfers); }
	puts("PASS OFFLINE actual private11 QKV decoder/full+small+one/two/early-EOS/zero+sparse+second-full position,ALL logits/token bits/KV/119 sequences/197-tensor row coverage/profile6/ALL32 bits of24-word summary/ALL16 proof+restore faults;independent math peers,NOT driver/native qualification");
}
