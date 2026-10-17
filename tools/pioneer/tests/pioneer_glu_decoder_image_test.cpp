/* SPDX-License-Identifier: MIT */
#define PIONEER_PROFILE_DECODER_MACHINE_ONLY
#include "pioneer_profile_decoder_image_test.cpp"
#define PIONEER_COORDINATED_GROUP_MACHINE_PROVIDED
#define PIONEER_COORDINATED_GLU_MACHINE_ONLY
#define Machine DecoderSfpuMachine
#define Image DecoderSfpuImage
#define F DecoderSfpuFloat
#define B DecoderSfpuBits
#include "pioneer_coordinated_glu_image_test.cpp"
#undef Machine
#undef Image
#undef F
#undef B
struct GluDecoder:ProfileDecoder {
	std::unique_ptr<GluMachine> pendingGlu[6];
	unsigned gluIssued=0,gluCompleted=0,gluBatches=0,badGluProof=0,dropGluInput=0,badPacked=0,badScale=0;
	GluDecoder(const char* path,const DecoderSfpuImage& image,bool nz,bool greedy=false,bool paired=false):ProfileDecoder(path,image,nz,true,greedy,paired) {}
	void CompleteGlu() {
		assert(gluIssued==63 && !gluCompleted); gluBatches++;
		for (unsigned p=0;p<6;p++) {
			auto& m=*pendingGlu[p]; m.Run(); m.VerifyGlu();
			for (unsigned a=0x91000;a<0x91440;a+=4) peerMemory[p][a]=m.mem[a/4];
			for (unsigned a=0x91500;a<0x91580;a+=4) peerMemory[p][a]=m.mem[a/4];
			for (unsigned i=0;i<16;i++) peerMemory[p][0x71340+i*4]=i<4?m.mem[0x91500/4+i]:i<8?m.mem[0x91520/4+i-4]
				:i==8?0x7200c:i==9?512:i==10?0x314e5441:i==11?sequence[p]:0;
			if (p==5 && badGluProof) peerMemory[p][0x91500+(badGluProof-1)*4]^=1;
			if (p==5 && badPacked) peerMemory[p][0x91000+1020]^=0x4000;
			if (p==5 && badScale) peerMemory[p][0x91400+60]=0x7f800000;
			pendingGlu[p].reset(); gluCompleted|=1U<<p;
		}
	}
	uint32_t PeerRead(unsigned w,uint32_t a) override {
		unsigned p=w-1;
		if (p<6 && descriptor[p][7]==0x314c4741) {
			if (Range(a,0x71300,16)) {
				if (gluIssued==63 && !gluCompleted) CompleteGlu();
				unsigned i=(a-0x71300)/4; return i==0?sequence[p]:i==1?sequence[p]-((gluCompleted&(1U<<p))?0:1):i==2?0:0x43524431;
			}
			if (Range(a,0x71340,64) || Range(a,0x91000,1088) || Range(a,0x91500,128)) { assert(peerMemory[p].count(a)); return peerMemory[p].at(a); }
		}
		if (p<6 && Range(a,0x90000,4096)) { assert(peerMemory[p].count(a)); return peerMemory[p].at(a); }
		return ParallelDecoder::PeerRead(w,a);
	}
	void PeerWrite(unsigned w,uint32_t a,uint32_t value) override {
		unsigned p=w-1;
		if (p<6 && Range(a,0x90000,4096)) { if (dropGluInput && p==5 && a==0x90ffc) value^=1; peerMemory[p][a]=value; return; }
		if (a==0x71300 && descriptor[p][7]==0x314c4741) {
			assert(p<6 && value==sequence[p]+1 && !pendingGlu[p]); sequence[p]=value; issued++;
			if (gluCompleted==63) gluCompleted=gluIssued=0;
			pendingGlu[p].reset(new GluMachine(groupImage,peerMemory[p].at(0x74008),p)); auto& m=*pendingGlu[p];
			for (unsigned i=0;i<8;i++) m.mem[0x70500/4+i]=descriptor[p][i];
			for (unsigned i=0;i<4;i++) m.mem[0x74000/4+i]=peerMemory[p].at(0x74000+i*4);
			for (unsigned i=0;i<1024;i++) m.mem[0x90000/4+i]=peerMemory[p].at(0x90000+i*4);
			gluIssued|=1U<<p; return;
		}
		ParallelDecoder::PeerWrite(w,a,value);
	}
};
#ifndef PIONEER_GLU_DECODER_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true; DecoderSfpuImage image(argv[2],false,true,true);
	{ GluDecoder m(argv[1],image,false); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); assert(m.gluBatches==28); }
	{ GluDecoder m(argv[1],image,true); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); m.Next(); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); assert(m.gluBatches==56); }
	for (unsigned word=1;word<=32;word++) { GluDecoder m(argv[1],image,false); m.badGluProof=word; assert(!m.RunDecoder() && !m.hostWrites); }
	for (unsigned fault=0;fault<3;fault++) { GluDecoder m(argv[1],image,false);
		if (!fault) m.dropGluInput=1; else if (fault==1) m.badPacked=1; else m.badScale=1;
		assert(!m.RunDecoder() && !m.hostWrites); }
	puts("PASS OFFLINE actual whole master/8 attention/6 GLU scalar+SFPU images,all6 GLUs issued BEFORE waits,2 positions/exact full logits/KV/Q8/119 sequences/counters,per-word/operand/scale/staging faults;matrix peers independent model,NOT native qualification");
}
#endif
