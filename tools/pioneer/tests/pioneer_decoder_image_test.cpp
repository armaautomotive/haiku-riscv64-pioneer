/* SPDX-License-Identifier: MIT */
// Execute ACTUAL whole-decoder master/scalar/attention/SFPU bytes. Math
// peers are independently modeled ZERO fixtures here;their unchanged real
// bytecode is qualified by separate matrix/FFN/wide tests. NOT native proof.
#define PIONEER_COORDINATOR_MACHINE_ONLY
#include "pioneer_coordinator_image_test.cpp"
#include "PioneerScalarInstruction.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDecoderCandidateImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeParallelDecoderCandidateImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeProfileDecoderCandidateImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGluDecoderCandidateImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGreedyDecoderCandidateImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGreedyUnrolledDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGreedyRmsDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGreedyCountersDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePairedDecoderCandidateImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePullInputDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePullDiagnosticDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTableDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeSmallRowsDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRopeDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeBatchDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeGatherDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePackedDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeOptimizedDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeSpeedDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeHotQuantDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeFixedRouteDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePairedQkvDecoderImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderGreedy.h"
#define Machine DecoderSfpuMachine
#define Image DecoderSfpuImage
#define Read DecoderSfpuRead
#define Word DecoderSfpuWord
#define Sign DecoderSfpuSign
#define F DecoderSfpuFloat
#define B DecoderSfpuBits
#define Flush DecoderSfpuFlush
#define main decoder_unused_attention_main
#include "pioneer_attention_image_test.cpp"
#undef Machine
#undef Image
#undef Read
#undef Word
#undef Sign
#undef F
#undef B
#undef Flush
#undef main
struct DecoderMachine:CoordinatorMachine {
	DecoderSfpuMachine sfpu;
	bool nonzero,parallel,fusedGlu,sampling,paired,diagnostic;
	unsigned positions=1;
	std::vector<float> peerOutput[119],expected;
	std::vector<uint16_t> expectedK[28],expectedV[28];
	std::vector<float> expectedK32[28];
	unsigned kvDiagnostics=0;
	uint32_t fp[32]={},descriptor[119][8]={},sequence[119]={},tiles[119]={};
	std::map<uint32_t,uint32_t> kv;
	std::vector<uint32_t> wide=std::vector<uint32_t>(262144,0xffffffff);
	bool comparing=false;
	unsigned dropCopy=0,currentPhase=0;
	unsigned originalWords=0,returnedWords=0,compareWords=0,bankReads=0,bankWrites=0;
	static uint32_t WeightBase(unsigned phase) { return phase==7?0x40000000:0x04000000+phase*0x1000000; }
	static unsigned Phase(uint32_t base) { return base>=0x40000000?7:(base-0x04000000)/0x1000000; }
	static float WeightScale(unsigned phase) { return float(phase+1)*.03125f; }
	// Independent host IEEE conversion,including subnormals. The older
	// matrix-operand PioneerFloat16 helper intentionally FLUSHES them and
	// is NOT the pinned GGML KV/Q8 conversion reference.
	static uint16_t HalfBits(float value) { _Float16 h=_Float16(value); uint16_t bits; memcpy(&bits,&h,2); return bits; }
	static float HalfValue(uint16_t bits) { _Float16 h; memcpy(&h,&bits,2); return float(h); }
	static std::vector<float> Norm(const std::vector<float>& input,unsigned width) {
		std::vector<float> result(input.size());
		for (unsigned head=0;head<input.size();head+=width) { double sum=0; for (unsigned i=0;i<width;i++) sum+=double(input[head+i]*input[head+i]);
			float mean=float(sum/width),scale=1.0f/sqrtf(mean+1e-6f); for (unsigned i=0;i<width;i++) result[head+i]=input[head+i]*scale; }
		return result;
	}
	static std::vector<float> ReferenceMatrix(const std::vector<float>& input,unsigned n,unsigned phase) {
		unsigned k=unsigned(input.size()); std::vector<int> q(k); std::vector<float> scales(k/32),result(n);
		for (unsigned b=0;b<k/32;b++) { float maximum=0; for (unsigned i=0;i<32;i++) maximum=fmaxf(maximum,fabsf(input[b*32+i]));
			float d=maximum/127.0f,id=d?1.0f/d:0; scales[b]=HalfValue(HalfBits(d));
			for (unsigned i=0;i<32;i++) q[b*32+i]=int(roundf(input[b*32+i]*id)); }
		for (unsigned r=0;r<n;r++) { unsigned at=(r*13+phase*7)%k; int weight=r%3==0?-1:1;
			result[r]=fmaf(float(q[at]*weight),WeightScale(phase)*scales[at/32],0); }
		return result;
	}
	std::vector<float> Rope(std::vector<float> x) {
		for (unsigned head=0;head<x.size();head+=128) for (unsigned i=0;i<64;i++) {
			float a=x[head+i],b=x[head+64+i],c=Float(memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope)+i*8)),s=Float(memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope)+i*8+4));
			x[head+i]=fmaf(a,c,-(b*s)); x[head+64+i]=fmaf(a,s,b*c); }
		return x;
	}
	void Reference(const std::vector<float>* initial=nullptr) {
		std::vector<float> hidden(1024); for (unsigned i=0;i<1024;i++) { hidden[i]=initial?initial->at(i):float(sin(i*.173+(positions-1)*.317)*.1+.125); memory[TT_DECODER_INPUT+i*4]=Bits(hidden[i]); }
		for (unsigned layer=0;layer<28;layer++) {
			auto residual=hidden,normalized=Norm(hidden,1024),v=ReferenceMatrix(normalized,1024,1),k=Rope(Norm(ReferenceMatrix(normalized,1024,2),128));
			auto q=Rope(Norm(ReferenceMatrix(normalized,2048,0),128));
			expectedK32[layer]=k;
			expectedK[layer].resize(positions*1024); expectedV[layer].resize(positions*1024);
			for (unsigned i=0;i<1024;i++) { expectedK[layer][(positions-1)*1024+i]=HalfBits(k[i]); expectedV[layer][(positions-1)*1024+i]=HalfBits(v[i]); }
			std::vector<float> attention(2048);
			for (unsigned g=0;g<8;g++) { std::vector<float> tq(q.begin()+g*256,q.begin()+(g+1)*256),out(256); std::vector<uint16_t> tk(positions*128),tv(positions*128);
				for (unsigned t=0;t<positions;t++) for (unsigned d=0;d<128;d++) { tk[t*128+d]=expectedK[layer][t*1024+g*128+d]; tv[t*128+d]=expectedV[layer][t*1024+g*128+d]; }
				::Reference(tq,tk,tv,positions,out); std::copy(out.begin(),out.end(),attention.begin()+g*256); }
			hidden=ReferenceMatrix(attention,1024,3); for (unsigned i=0;i<1024;i++) hidden[i]+=residual[i];
			residual=hidden; normalized=Norm(hidden,1024); auto gate=ReferenceMatrix(normalized,3072,4),up=ReferenceMatrix(normalized,3072,5);
			for (unsigned i=0;i<3072;i++) up[i]=(gate[i]/(1.0f+expf(-gate[i])))*up[i];
			hidden=ReferenceMatrix(up,1024,6); for (unsigned i=0;i<1024;i++) hidden[i]+=residual[i];
		}
		expected=ReferenceMatrix(Norm(hidden,1024),151936,7);
	}
	DecoderMachine(const char* root,const DecoderSfpuImage& attention,bool changed=false,bool concurrent=false,bool profile=false,bool glu=false,bool greedy=false,bool together=false):CoordinatorMachine(root,true,true),sfpu(attention,1),nonzero(changed),parallel(concurrent),fusedGlu(glu),sampling(greedy),paired(together),diagnostic(getenv("PIONEER_PULL_DIAGNOSTIC")!=nullptr) {
		bool unrolled=greedy && code.size()==TT_GREEDY_UNROLLED_DECODER_IMAGE_WORDS*4;
		bool fastRms=greedy && code.size()==TT_GREEDY_RMS_DECODER_IMAGE_WORDS*4;
		bool cumulative=greedy && code.size()==TT_GREEDY_COUNTERS_DECODER_IMAGE_WORDS*4;
		assert(code.size()==unsigned(getenv("PIONEER_GREEDY_PAIRED_QKV")?TT_PAIRED_QKV_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_FIXED_ROUTES")?TT_FIXED_ROUTE_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_HOT_QUANT")?TT_HOT_QUANT_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_SPEED_MASTER")?TT_SPEED_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_OPTIMIZED")?TT_OPTIMIZED_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_PACKED_Q8")?TT_PACKED_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_FUSED_GATHER")?TT_GATHER_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_BATCH")?TT_BATCH_DECODER_IMAGE_WORDS:attention.parallelRope?TT_ROPE_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_SMALL_ROWS")?TT_SMALL_ROWS_DECODER_IMAGE_WORDS:getenv("PIONEER_GREEDY_DESCRIPTOR_TABLE")?TT_TABLE_DECODER_IMAGE_WORDS:diagnostic?TT_PULL_DIAGNOSTIC_DECODER_IMAGE_WORDS:attention.inputPull?TT_PULL_INPUT_DECODER_IMAGE_WORDS:paired?TT_PAIRED_DECODER_IMAGE_WORDS:cumulative?TT_GREEDY_COUNTERS_DECODER_IMAGE_WORDS:fastRms?TT_GREEDY_RMS_DECODER_IMAGE_WORDS:unrolled?TT_GREEDY_UNROLLED_DECODER_IMAGE_WORDS:greedy?TT_GREEDY_DECODER_IMAGE_WORDS:glu?TT_GLU_DECODER_IMAGE_WORDS:profile?TT_PROFILE_DECODER_IMAGE_WORDS:parallel?TT_PARALLEL_DECODER_IMAGE_WORDS:TT_DECODER_IMAGE_WORDS)*4 && l1[0]==TT_DECODER_BOOT);
		for (unsigned i=0;i<TT_DECODER_CODE_WORDS;i++) assert((getenv("PIONEER_GREEDY_PAIRED_QKV")?BlackholePairedQkvDecoderCodeWord(i):getenv("PIONEER_GREEDY_FIXED_ROUTES")?BlackholeFixedRouteDecoderCodeWord(i):getenv("PIONEER_GREEDY_HOT_QUANT")?BlackholeHotQuantDecoderCodeWord(i):getenv("PIONEER_GREEDY_SPEED_MASTER")?BlackholeSpeedDecoderCodeWord(i):getenv("PIONEER_GREEDY_OPTIMIZED")?BlackholeOptimizedDecoderCodeWord(i):getenv("PIONEER_GREEDY_PACKED_Q8")?BlackholePackedDecoderCodeWord(i):getenv("PIONEER_GREEDY_FUSED_GATHER")?BlackholeGatherDecoderCodeWord(i):getenv("PIONEER_GREEDY_BATCH")?BlackholeBatchDecoderCodeWord(i):attention.parallelRope?BlackholeRopeDecoderCodeWord(i):getenv("PIONEER_GREEDY_SMALL_ROWS")?BlackholeSmallRowsDecoderCodeWord(i):getenv("PIONEER_GREEDY_DESCRIPTOR_TABLE")?BlackholeTableDecoderCodeWord(i):diagnostic?BlackholePullDiagnosticDecoderCodeWord(i):attention.inputPull?BlackholePullInputDecoderCodeWord(i):paired?BlackholePairedDecoderCodeWord(i):cumulative?BlackholeGreedyCountersDecoderCodeWord(i):fastRms?BlackholeGreedyRmsDecoderCodeWord(i):unrolled?BlackholeGreedyUnrolledDecoderCodeWord(i):greedy?BlackholeGreedyDecoderCodeWord(i):glu?BlackholeGluDecoderCodeWord(i):profile?BlackholeProfileDecoderCodeWord(i):parallel?BlackholeParallelDecoderCodeWord(i):BlackholeDecoderCodeWord(i))==(i<code.size()/4?Word(code,i*4):0));
		for (unsigned a=0x80000;a<0x8c000;a+=4) memory[a]=0;
		for (unsigned a=0x90000;a<0xb2400;a+=4) memory[a]=0;
		for (unsigned a=0x140000;a<0x148000;a+=4) memory[a]=0;
		for (unsigned a=0x74000;a<0x74010;a+=4) memory[a]=0;
		for (unsigned a=0x17ffc0;a<0x17ffcc;a+=4) memory[a]=0;
		PioneerDecoderJob j={}; j.magic=0x31444344; j.operation=8; j.version=getenv("PIONEER_GREEDY_BATCH")?10:attention.parallelRope?9:getenv("PIONEER_GREEDY_SMALL_ROWS")?8:getenv("PIONEER_GREEDY_DESCRIPTOR_TABLE")?7:attention.inputPull?6:paired?5:greedy?4:glu?3:parallel?2:1; j.tokens=j.nonce=1;
		j.kvBase=0x82400000; j.finalNorm=0x30000000; j.vocabulary=WeightBase(7); j.hostLow=0x80000000; j.hostHigh=1; j.hostBytes=0x100000; j.workers=119;
		for (auto& l:j.layers) { auto* words=reinterpret_cast<uint32_t*>(&l); for (unsigned i=0;i<7;i++) words[i]=WeightBase(i); for (unsigned i=7;i<11;i++) words[i]=0x30000000; }
		if (getenv("PIONEER_GREEDY_BATCH")) { j.operation=10; j.reserved[0]=2; j.reserved[1]=0x31000000; j.reserved[2]=151643; j.reserved[3]=5; }
		for (auto& value:j.sequence) value=1;
		for (unsigned i=0;i<64;i++) j.rope[2*i]=0x3f800000;
		assert(PioneerDecoderJobValid(j,getenv("PIONEER_GREEDY_BATCH")?10:attention.parallelRope?9:getenv("PIONEER_GREEDY_SMALL_ROWS")?8:getenv("PIONEER_GREEDY_DESCRIPTOR_TABLE")?7:attention.inputPull?6:paired?5:greedy?4:glu?3:parallel?2:1));
		for (unsigned i=0;i<sizeof(j)/4;i++) memory[TT_DECODER_JOB_BASE+i*4]=reinterpret_cast<uint32_t*>(&j)[i];
		if (nonzero) Reference();
	}
	void Next() {
		assert(nonzero && positions==1 && !pending); positions++;
		memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,tokens)]=positions;
		memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,nonce)]=positions;
		float theta=float(positions-1),step=powf(1000000.0f,-2.0f/128);
		for (unsigned i=0;i<64;i++) { memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope)+i*8]=Bits(cosf(theta));
			memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope)+i*8+4]=Bits(sinf(theta)); theta*=step; }
		for (auto& value:wide) value=0xffffffff;
		comparing=false; Reference(); pc=0x6000; for (unsigned i=0;i<3;i++) memory[0x17ffc0+i*4]=0;
	}
	unsigned StepLimit() const override { return 2000000000U; }
	uint32_t PeerRead(unsigned w,uint32_t a) override {
		assert(w && w<=119); unsigned p=w-1;
		if (Range(a,0x70500,32)) return descriptor[p][(a-0x70500)/4];
		if (Range(a,0x71300,16)) { unsigned i=(a-0x71300)/4; return i<2?sequence[p]:i==2?0:0x43524431; }
		if (Range(a,0x71340,64)) {
			unsigned i=(a-0x71340)/4,reads=tiles[p]>>5;
			if (i<4) return 0xfffffff8;
			if (i<8) return 0xfffffff8+(i==5?reads*144+256:reads+1);
			return i==8?0x703ec:i==9?tiles[p]:i==10?0x4d415431:i==11?sequence[p]:0;
		}
		assert(Range(a,0x84000,descriptor[p][2]*16*4)); return nonzero?Bits(peerOutput[p].at((a-0x84000)/4)):0;
	}
	void PeerWrite(unsigned w,uint32_t a,uint32_t value) override {
		assert(w && w<=119); unsigned p=w-1;
		if (Range(a,0x70500,32)) { descriptor[p][(a-0x70500)/4]=value; return; }
		assert(a==0x71300 && value==sequence[p]+1 && value<0x7fffffff && descriptor[p][3]==1
			&& descriptor[p][4]==(PioneerWorkerX(w)|(PioneerWorkerY(w)<<6)));
		sequence[p]=value; tiles[p]=descriptor[p][1]*descriptor[p][2]; issued++;
		unsigned inputs=descriptor[p][1]*16;
		if (!nonzero) { for (unsigned i=0;i<inputs/2;i++) assert(memory.at(0x80000+i*4)==0x40004000);
			for (unsigned i=0;i<inputs/32;i++) assert(memory.at(0x83000+i*4)==0); }
		else {
			unsigned phase=Phase(descriptor[p][0]),first=(descriptor[p][0]-WeightBase(phase))/(inputs+inputs/8),rows=descriptor[p][2]*16;
			peerOutput[p].resize(rows);
			for (unsigned r=0;r<rows;r++) { unsigned row=first+r,at=(row*13+phase*7)%inputs; uint16_t a=uint16_t(memory.at(0x80000+at/2*4)>>(16*(at&1)));
				assert(PioneerInt8OperandValid(a)); int q=int(a&1023)*((a&0x8000)?-1:1),weight=row%3==0?-1:1;
				peerOutput[p][r]=fmaf(float(q*weight),WeightScale(phase)*Float(memory.at(0x83000+at/32*4)),0); }
		}
	}
	void Submit() override {
		bool write=command[7]==0x212; unsigned xy=write?command[5]:command[2];
		if (xy!=786 && xy!=1555) {
			operation=memory.at(0x140004); k=memory.at(0x14000c); n=memory.at(0x140010); limit=119;
			currentPhase=Phase(memory.at(0x140014));
			workers=std::min(n/32,119U); comparing=false; CoordinatorMachine::Submit(); return;
		}
		transfers++; if (transfers==failTransfer) { pending=true; return; }
		unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8],words=bytes/4;
		assert(bytes && bytes<=16384 && !(bytes&63) && !command[6] && !command[9] && (write?command[2]:command[5])==129
			&& !(write?command[1]:command[4]));
		if (xy==1555) {
			assert((write?command[4]:command[1])==1 && remote>=0x80000000 && local==(write?0x84000U:0x144000U));
			unsigned start=(remote-0x80000000)/4; assert(start+words<=151936);
			if (write) { if (!dropPosted) for (unsigned i=0;i<words-(partialPosted?1:0);i++) wide[start+i]=memory.at(local+i*4);
				if (pairedCorrupt) { wide[start]^=1; wide[start+1]^=1; } hostWrites++; }
			else { for (unsigned i=0;i<words;i++) memory[local+i*4]=wide[start+i]; hostReads++; comparing=true; compareWords=words; }
		} else {
			assert(!command[1] && !command[4]); comparing=false;
			if (remote<0x82400000) {
				assert(!write && remote==0x30000000 && local==TT_DECODER_NORM && (bytes==512 || bytes==4096));
				for (unsigned i=0;i<words;i++) memory[local+i*4]=0x3f800000;
			} else {
				assert(remote>=0x82400000 && remote+bytes<=0x84000000);
				if (write) { assert(bytes==256 && Range(local,0x142000,4096)); for (unsigned i=0;i<words;i++) kv[remote+i*4]=memory.at(local+i*4);
					if (nonzero && positions==2 && (remote-0x82400000)%0x20000<0x10000) {
						unsigned layer=(remote-0x82400000)/0x100000,g=((remote-0x82400000)%0x100000)/0x20000;
						for (unsigned i=0;i<words;i++) { unsigned p=g*128+i*2; uint32_t want=expectedK[layer][1024+p]|(uint32_t(expectedK[layer][1024+p+1])<<16);
							if (want!=memory.at(local+i*4) && kvDiagnostics++<4) { printf("KV TRACE layer%u group%u pair%u actual/expected%08x/%08x F32%08x/%08x,%08x/%08x\n",layer,g,i,memory.at(local+i*4),want,
								memory.at(TT_DECODER_K+p*4),Bits(expectedK32[layer][p]),memory.at(TT_DECODER_K+(p+1)*4),Bits(expectedK32[layer][p+1])); fflush(stdout); }
						}
					}
				}
				else { assert(Range(local,0x92000,65536) || Range(local,0xa2000,65536)); for (unsigned i=0;i<words;i++) memory[local+i*4]=kv.at(remote+i*4); }
			}
			if (write) bankWrites++; else bankReads++;
		}
		unsigned at=write?4:0; counts[at]++; counts[at+1]+=bytes/64; counts[2]++; counts[6]=counts[2]; counts[at+3]++;
		if (transfers==badCounter) counts[at+3]++;
	}
	uint32_t Load(uint32_t a) override {
		if (a==0xffe000fc || Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,12*64)) return sfpu.Load(a);
		if (comparing) { if (Range(a,0x84000,compareWords*4)) originalWords++; if (Range(a,0x144000,compareWords*4)) returnedWords++; }
		if (Range(a,0x80000,0x4000*3) || Range(a,0x90000,0x22400) || Range(a,0x74000,16)) return memory.at(a);
		return CoordinatorMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if ((dropCopy==1 && a==TT_DECODER_HIDDEN) || (dropCopy==2 && currentPhase==4 && a==0x87000)) return;
		if (a==0xffe40000 || a==0xffe000fc || Range(a,0xffef0000,224*4) || Range(a,0xffbd8000,8*64)) { sfpu.Store(a,value); return; }
		if (Range(a,TT_DECODER_JOB_BASE,4096)) {
			assert(Range(a,TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,sequence),119*4)); memory[a]=value; return;
		}
		if (Range(a,0x80000,0xc000) || Range(a,0x90000,0x22400) || Range(a,0x74000,16) || Range(a,0x140000,0x6000)) { memory[a]=value; return; }
		CoordinatorMachine::Store(a,value);
	}
	bool RunDecoder() {
		while (pc!=0x64038 && pc!=0x64058) PioneerScalarStep(*this,fp);
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==TT_COORDINATOR_FAULT && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==TT_LAYER_DONE && memory.at(0x17ffc8)==TT_COORDINATOR_MARKER); return memory.at(0x17ffc4)!=0;
	}
	void Verify() {
		bool small=memory.at(TT_DECODER_JOB_BASE+4)==9;
		printf("Decoder fixture counters:pending%u issued%u hostW/R%u/%u compared%u/%u bankR/W%u/%u count%u\n",pending,issued,hostWrites,hostReads,originalWords,returnedWords,bankReads,bankWrites,memory.at(0x17ffc4)); fflush(stdout);
		assert(!pending && issued==(paired?(getenv("PIONEER_GREEDY_SMALL_ROWS")?14343U:9219U):fusedGlu?11263U:parallel?11095U:10871U)*positions && hostWrites==(small?1U:119U)*positions && hostReads==hostWrites && originalWords==(small?32U:151936U)*positions && returnedWords==originalWords
			&& bankReads==(parallel?113U:113+28*16)*positions && bankWrites==(parallel?0U:28*16)*positions && memory.at(0x17ffc4)==119);
		for (unsigned i=0;i<119;i++) assert(sequence[i]==PioneerDecoderSequenceAdvance(i,parallel,fusedGlu,paired,getenv("PIONEER_GREEDY_SMALL_ROWS")!=nullptr)*positions
			&& memory.at(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,sequence)+i*4)==sequence[i]+1);
		if (small) { unsigned best=0; if (nonzero) for (unsigned i=1;i<151936;i++) if (expected[i]>expected[best]) best=i;
			assert(PioneerDecoderGreedyValid(wide.data(),positions,positions) && wide[3]==best && wide[4]==(nonzero?Bits(expected[best]):0)); }
		else for (unsigned i=diagnostic?16:0;i<151936;i++) assert(wide[i]==(nonzero?Bits(expected[i]):0));
		if (diagnostic) { assert(!small && wide[0]==Bits(12345) && wide[9]==Bits(27));
			assert(wide[1]==wide[2] && wide[3]==wide[4] && wide[5]==wide[6] && wide[7]==wide[8] && !wide[10] && !wide[11]); }
		if (nonzero) for (unsigned layer=0;layer<28;layer++) for (unsigned t=0;t<positions;t++) for (unsigned g=0;g<8;g++) for (unsigned i=0;i<64;i++) {
			unsigned p=t*1024+g*128+i*2;
			assert(kv.at(0x82400000+layer*0x100000+g*0x20000+t*256+i*4)==(expectedK[layer][p]|(uint32_t(expectedK[layer][p+1])<<16)));
			assert(kv.at(0x82410000+layer*0x100000+g*0x20000+t*256+i*4)==(expectedV[layer][p]|(uint32_t(expectedV[layer][p+1])<<16))); }
		for (unsigned i=small?32:151936;i<wide.size();i++) assert(wide[i]==0xffffffff);
		uint32_t s[24],before[8],after[8]; for (unsigned i=0;i<24;i++) s[i]=memory.at(0x143000+i*4);
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]; after[i]=s[16+i]; }
		assert(PioneerDecoderSummary(s,positions,positions,before,after,parallel,fusedGlu,small,paired,sfpu.image.inputPull,diagnostic,getenv("PIONEER_GREEDY_DESCRIPTOR_TABLE")!=nullptr,getenv("PIONEER_GREEDY_SMALL_ROWS")!=nullptr,getenv("PIONEER_GREEDY_BATCH")!=nullptr,getenv("PIONEER_GREEDY_FUSED_GATHER")!=nullptr));
		printf("PASS OFFLINE ACTUAL master/scalar/attention/SFPU whole28-layer decoder,independently modeled %s math peers,197 projections/28 FFN/224 heads,one final vocabulary transfer,all counters/119 sequences,steps%u stack%08x;NOT full-model/native qualification\n",nonzero?"SPARSE NONZERO":"ZERO",steps,minStack); fflush(stdout);
	}
};
#ifndef PIONEER_DECODER_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true; DecoderSfpuImage attention(argv[2],true);
	{ DecoderMachine m(argv[1],attention); assert(m.RunDecoder()); m.Verify(); }
	{ DecoderMachine m(argv[1],attention,true); assert(m.RunDecoder()); m.Verify(); m.Next(); assert(m.RunDecoder()); m.Verify(); }
	for (unsigned fault=0;fault<5;fault++) {
		DecoderMachine m(argv[1],attention);
		if (fault==0) m.memory[TT_DECODER_JOB_BASE]^=1; if (fault==1) m.memory[TT_DECODER_JOB_BASE+12]=257;
		if (fault==2) m.memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,sequence)]=0;
		if (fault==3) m.memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope)]=0x7fc00000;
		if (fault==4) m.memory[TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,layers)]=0x82400000;
		assert(!m.RunDecoder() && !m.transfers && !m.issued && !m.hostWrites);
	}
	for (bool counter:{false,true}) { DecoderMachine m(argv[1],attention); if (counter) m.badCounter=1; else m.failTransfer=1;
		assert(!m.RunDecoder() && !m.issued && !m.hostWrites); }
	for (unsigned fault=0;fault<3;fault++) { DecoderMachine m(argv[1],attention); m.dropPosted=fault==0; m.partialPosted=fault==1; m.pairedCorrupt=fault==2;
		assert(!m.RunDecoder() && m.hostWrites==1 && m.hostReads==1 && !m.memory.at(0x17ffc4)); }
	for (unsigned fault:{1U,2U}) { DecoderMachine m(argv[1],attention,true); m.dropCopy=fault; assert(!m.RunDecoder() && !m.hostWrites);
		if (fault==1) assert(!m.transfers && !m.issued); }
}
#endif
