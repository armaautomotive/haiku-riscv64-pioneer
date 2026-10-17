/* SPDX-License-Identifier: MIT */
// Actual master + group scalar/SFPU bytes;independent sparse matrix peers.
// The dispatcher is qualified separately. No native/card authority here.
#define PIONEER_DECODER_MACHINE_ONLY
#include "pioneer_decoder_image_test.cpp"
#define PIONEER_ATTENTION_MACHINE_PROVIDED
#define PIONEER_COORDINATED_ATTENTION_MACHINE_ONLY
#define Machine DecoderSfpuMachine
#define Image DecoderSfpuImage
#define F DecoderSfpuFloat
#define B DecoderSfpuBits
#include "pioneer_coordinated_attention_image_test.cpp"
#undef Machine
#undef Image
#undef F
#undef B
#include <memory>
struct ParallelDecoder:DecoderMachine {
	const DecoderSfpuImage& groupImage;
	std::map<uint32_t,uint32_t> peerMemory[8];
	std::unique_ptr<GroupMachine> pendingGroups[8];
	unsigned issuedGroups=0,completedGroups=0,parallelBatches=0;
	unsigned badGroupProof=0,corruptGroupOutput=0,dropGroupInput=0,failGroup=0;
	bool delayFirst=false,delayed=false;
	ParallelDecoder(const char* path,const DecoderSfpuImage& image,bool nz,bool profile=false,bool glu=false):DecoderMachine(path,image,nz,true,profile,glu),groupImage(image) {}
	void CompleteGroups() {
		assert(issuedGroups==255 && !completedGroups); parallelBatches++;
		for (unsigned p=0;p<8;p++) {
			auto& m=*pendingGroups[p]; m.dram.swap(kv);
			m.Run(failGroup==p+1); m.dram.swap(kv);
			if (failGroup==p+1) { peerMemory[p][0x17ffc8]=0x464e5441; return; }
			m.Verify();
			for (unsigned a:{0x74000U,0x74004U,0x74008U,0x7400cU}) peerMemory[p][a]=m.mem[a/4];
			for (unsigned a=0x90600;a<0x90660;a+=4) peerMemory[p][a]=m.mem[a/4];
			for (unsigned i=0;i<256;i++) peerMemory[p][0xb2000+i*4]=m.mem[0xb2000/4+i];
			for (unsigned i=0;i<16;i++) peerMemory[p][0x71340+i*4]=i<4?m.mem[0x90600/4+i]:i<8?m.mem[0x90620/4+i-4]
				:i==8?0x7200c:i==9?positions:i==10?0x314e5441:i==11?sequence[p]:0;
			if (badGroupProof && p==7) peerMemory[p][0x90600+(badGroupProof-1)*4]^=1;
			if (corruptGroupOutput && p==7) peerMemory[p][0xb2000+1020]=0x7fc00000;
			pendingGroups[p].reset(); completedGroups|=1U<<p;
		}
	}
	uint32_t PeerRead(unsigned w,uint32_t a) override {
		unsigned p=w-1;
		if (p<8 && descriptor[p][7]==0x31475441) {
			if (Range(a,0x71300,16)) {
				unsigned i=(a-0x71300)/4;
				if (issuedGroups==255 && !completedGroups) {
					if (delayFirst && !delayed && !i) delayed=true;
					else if (!delayFirst || !i) CompleteGroups();
				}
				return i==0?sequence[p]:i==1?sequence[p]-((completedGroups&(1U<<p))?0:1):i==2?0:0x43524431;
			}
			if (a==0x17ffc8) return peerMemory[p].count(a)?peerMemory[p][a]:0;
			if (Range(a,0x71340,64) || Range(a,0x90600,96) || Range(a,0xb2000,1024)) { assert(peerMemory[p].count(a)); return peerMemory[p].at(a); }
		}
		if (p<8 && (Range(a,0x74000,16) || Range(a,0x90000,1536))) { assert(peerMemory[p].count(a)); return peerMemory[p].at(a); }
		return DecoderMachine::PeerRead(w,a);
	}
	void PeerWrite(unsigned w,uint32_t a,uint32_t value) override {
		unsigned p=w-1;
		if (p<8 && (Range(a,0x74000,16) || Range(a,0x90000,1536))) {
			if (dropGroupInput && p==7 && a==0x905fc) value^=1;
			peerMemory[p][a]=value; return;
		}
		if (a==0x71300 && descriptor[p][7]==0x31475441) {
			assert(p<8 && value==sequence[p]+1 && !pendingGroups[p]); sequence[p]=value; issued++;
			if (completedGroups==255) { completedGroups=issuedGroups=0; delayed=false; }
			unsigned layer=peerMemory[p].at(0x74008);
			pendingGroups[p].reset(new GroupMachine(groupImage,positions,layer,p)); auto& m=*pendingGroups[p];
			for (unsigned i=0;i<8;i++) m.mem[0x70500/4+i]=descriptor[p][i];
			for (unsigned i=0;i<4;i++) m.mem[0x74000/4+i]=peerMemory[p].at(0x74000+i*4);
			for (unsigned i=0;i<384;i++) m.mem[0x90000/4+i]=peerMemory[p].at(0x90000+i*4);
			if (failGroup==p+1) m.failCommand=1;
			issuedGroups|=1U<<p; return;
		}
		DecoderMachine::PeerWrite(w,a,value);
	}
	void Submit() override {
		bool write=command[7]==0x212; unsigned xy=write?command[5]:command[2];
		if (xy==786 || xy==1555) { DecoderMachine::Submit(); return; }
		comparing=false;
		transfers++; if (transfers==failTransfer) { pending=true; return; }
		unsigned worker=0; for (unsigned w=1;w<120;w++) if (xy==(PioneerWorkerX(w)|(PioneerWorkerY(w)<<6))) worker=w;
		unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
		assert(worker && bytes && bytes<=16384 && !(bytes&3) && !command[1] && !command[4] && !command[6] && !command[9]
			&& (write?command[2]:command[5])==129 && (write || !command[7]));
		for (unsigned i=0;i<bytes;i+=4) { if (write) PeerWrite(worker,remote+i,memory.at(local+i)); else memory[local+i]=PeerRead(worker,remote+i); }
		unsigned at=write?4:0; counts[at]++; counts[at+1]+=(bytes+63)/64; counts[2]++; counts[6]=counts[2]; counts[at+3]++;
		if (transfers==badCounter) counts[at+3]++;
	}
};
#ifndef PIONEER_PARALLEL_DECODER_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true; DecoderSfpuImage image(argv[2],false,true);
	{ ParallelDecoder m(argv[1],image,false); assert(m.RunDecoder()); m.Verify(); assert(m.parallelBatches==28); }
	{ ParallelDecoder m(argv[1],image,true); m.delayFirst=true; assert(m.RunDecoder()); m.Verify(); m.Next(); assert(m.RunDecoder()); m.Verify(); assert(m.parallelBatches==56); }
	for (unsigned word=1;word<=24;word++) { ParallelDecoder m(argv[1],image,false); m.badGroupProof=word; assert(!m.RunDecoder() && !m.hostWrites); }
	for (unsigned fault=0;fault<3;fault++) { ParallelDecoder m(argv[1],image,false);
		if (!fault) m.corruptGroupOutput=1; else if (fault==1) m.dropGroupInput=1; else m.failGroup=1;
		assert(!m.RunDecoder() && !m.hostWrites); }
	puts("PASS OFFLINE actual whole master + all8 actual scalar/SFPU KV groups,all8 dispatched BEFORE any wait,2 positions/exact KV/full logits/counters/119 sequences,delayed completion and per-word/fault refusal;matrix peers independently modeled,NOT native concurrency/performance qualification");
}
#endif
