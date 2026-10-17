/* SPDX-License-Identifier: MIT */
// Execute the linked layer/master AND unchanged persistent peer bytecode.
// Offline only: no driver,PCI,reset or hardware timing is modeled here.
#define PIONEER_COORDINATOR_MACHINE_ONLY
#include "pioneer_coordinator_image_test.cpp"
#include "PioneerScalarInstruction.h"
#include "../PioneerGraphMath.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerLayerProof.h"
struct LayerMachine:CoordinatorMachine {
	uint32_t fp[32]={};
	unsigned kind,initialLimit,layerN,initialNonce;
	unsigned packedStores=0,dropPacked=0,gateStores=0,dropGate=0;
	unsigned finalOriginal=0,finalReturned=0;
	std::vector<float> expected;
	static uint32_t Base(unsigned phase) { return 0x04000000+phase*0x800000; }
	static int Weight(unsigned phase,unsigned row,unsigned i) { return int((phase*7+row*3+i*13+i/32)%15)-7; }
	static float Scale(unsigned phase,unsigned row,unsigned block) { return (1+(phase*3+row+block)%7)*.0009765625f; }
	static int Decode(uint32_t word,unsigned i) { unsigned h=uint16_t(word>>(16*(i&1))); return int(h&1023)*((h&0x8000)?-1:1); }
	LayerMachine(const char* directory,unsigned op,unsigned peers,unsigned count=3072,bool debug=false):CoordinatorMachine(directory,true),kind(op),initialLimit(peers),layerN(op==4?1024:count),initialNonce(17) {
		auto boot=Read((std::string(directory)+"/boot.bin").c_str());
		assert(Word(boot,0)==TT_LAYER_BOOT && code.size()==TT_LAYER_IMAGE_WORDS*4);
		for (unsigned i=0;i<TT_LAYER_CODE_WORDS;i++) assert(BlackholeLayerCodeWord(i)==(i<code.size()/4?Word(code,i*4):0));
		operation=op; limit=peers; n=layerN; nonce=initialNonce;
		for (unsigned i=0;i<4096;i++) memory[0x80000+i*4]=0;
		for (unsigned i=0;i<8192;i++) memory[0x84000+i*4]=0x7fc00000;
		for (unsigned i=0;i<128;i++) memory[0x140000+i*4]=0;
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
		memory[0x140000]=0x31445243; memory[0x140004]=op; memory[0x140008]=peers;
		memory[0x14000c]=op==4?1024:0; memory[0x140010]=layerN; memory[0x140014]=op==4?Base(0):0;
		memory[0x140018]=nonce; memory[0x14001c]=0x80000000; memory[0x140020]=1; memory[0x140024]=16384;
		memory[0x140028]=op==4?Base(1):0; memory[0x14002c]=op==4?Base(2):0; memory[0x140030]=debug;
		for (unsigned w=1;w<=std::min(peers,96U);w++) {
			memory[0x140100+(w-1)*4]=1;
			if (op==4) { peer[w].reset(new CoordinatedMachine(workerImage,1024,32,w)); peer[w]->BootCoord(); }
		}
		for (auto& value:host) value=0xffffffff;
		if (op==4) {
			for (unsigned i=0;i<1024;i++) memory[0x80000+i/2*4]|=uint32_t(PioneerInt8Operand(int(i*13%255)-127))<<(16*(i&1));
			for (unsigned b=0;b<32;b++) memory[0x80800+b*4]=Bits(.0009765625f*(1+b%5));
			std::vector<float> gate=ReferenceMatrix(0,1024,3072,0x80800),up=ReferenceMatrix(1,1024,3072,0x80800);
			std::vector<uint32_t> original;
			for (unsigned i=0;i<4096;i++) original.push_back(memory.at(0x80000+i*4));
			for (unsigned b=0;b<96;b++) {
				float values[32]; uint32_t operands[16],scale;
				for (unsigned i=0;i<32;i++) values[i]=(debug?PioneerGraphSilu(gate[b*32+i]):PioneerGraphSiluFast(gate[b*32+i]))*up[b*32+i];
				assert(PioneerGraphQuantize32(values,operands,scale));
				for (unsigned i=0;i<16;i++) memory[0x80000+(b*16+i)*4]=operands[i];
				memory[0x81800+b*4]=scale;
			}
			expected=ReferenceMatrix(2,3072,1024,0x81800);
			for (unsigned i=0;i<4096;i++) memory[0x80000+i*4]=original[i];
		} else {
			for (unsigned i=0;i<layerN;i++) {
				float gate=float(sin(i*.713)*12.7),up=float(cos(i*.219)*3.1);
				memory[0x87000+i*4]=Bits(gate); memory[0x84000+i*4]=Bits(up);
				expected.push_back((debug?PioneerGraphSilu(gate):PioneerGraphSiluFast(gate))*up);
			}
		}
	}
	std::vector<float> ReferenceMatrix(unsigned phase,unsigned inputs,unsigned outputs,uint32_t scales) {
		std::vector<float> result(outputs);
		for (unsigned row=0;row<outputs;row++) {
			float sum=0;
			for (unsigned b=0;b<inputs/32;b++) {
				int dot=0; for (unsigned i=0;i<32;i++) dot+=Weight(phase,row,b*32+i)*Decode(memory.at(0x80000+(b*16+i/2)*4),i);
				sum=PioneerBlackholeFma(float(dot),Scale(phase,row,b)*Float(memory.at(scales+b*4)),sum);
			}
			result[row]=sum;
		}
		return result;
	}
	unsigned StepLimit() const override { return 100000000; }
	void PeerWrite(unsigned w,uint32_t a,uint32_t value) override {
		if (a==0x71300) {
			auto& m=*peer[w]; unsigned inputs=m.l1[m.Index(0x70504)]*16,outputs=m.l1[m.Index(0x70508)]*16;
			uint32_t base=m.l1[m.Index(0x70500)]; unsigned phase=(base-Base(0))/0x800000;
			assert(phase<=2 && inputs==(phase==2?3072:1024) && outputs && !(outputs&31));
			unsigned first=(base-Base(phase))/(inputs+inputs/8);
			assert(base==Base(phase)+first*(inputs+inputs/8) && first+outputs<=(phase==2?1024U:3072U));
			m.inputs=inputs; m.outputs=outputs; m.base=base; m.vector.resize(inputs);
			m.weights.assign(PioneerQ8CompactWeightBytes(inputs,outputs)/4,0);
			for (unsigned row=0;row<outputs;row++) for (unsigned b=0;b<inputs/32;b++) {
				m.weights[PioneerQ8NativeF32ScaleByte(inputs,b,row)/4]=Bits(Scale(phase,first+row,b));
				for (unsigned i=0;i<32;i++) {
					uint8_t q=0; assert(PioneerInt8UnpackEncode(Weight(phase,first+row,b*32+i),q));
					uint64_t at=PioneerQ8NativeF32ValueByte(inputs,b*32+i,row); m.weights[at/4]|=uint32_t(q)<<(8*(at&3));
				}
			}
		}
		CoordinatorMachine::PeerWrite(w,a,value);
	}
	void Submit() override {
		bool write=command[7]==0x212; unsigned xy=write?command[5]:command[2];
		if (xy!=1555) { CoordinatorMachine::Submit(); return; }
		transfers++; if (transfers==failTransfer) { pending=true; return; }
		const uint32_t expectedCommand[]={write?0x84000U:0x80000000U,write?0U:1U,write?129U:1555U,
			write?0x80000000U:0x144000U,write?1U:0U,write?1555U:129U,0,write?0x212U:0U,layerN*4,0};
		assert(!memcmp(command,expectedCommand,sizeof(command)));
		if (write) {
			hostWrites++;
			if (!dropPosted) for (unsigned i=0;i<layerN-(partialPosted?1:0);i++) host[i]=memory.at(0x84000+i*4)^(i==corruptHost?1:0);
			if (pairedCorrupt) { host[0]^=1; host[1]^=1; }
		} else { hostReads++; for (unsigned i=0;i<layerN;i++) memory[0x144000+i*4]=host[i]; }
		unsigned at=write?4:0;
		counts[at]++; counts[at+1]+=layerN/16; counts[2]++; counts[6]=counts[2]; counts[at+3]++;
		if (transfers==badCounter) counts[at+3]++;
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0x80000,16384)) return memory.at(a);
		if (hostReads && Range(a,0x84000,layerN*4)) finalOriginal++;
		if (hostReads && Range(a,0x144000,layerN*4)) finalReturned++;
		return CoordinatorMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0x80000,16384)) { if (++packedStores!=dropPacked) memory[a]=value; return; }
		if (Range(a,0x84000,3072*4)) { memory[a]=value; return; }
		if (Range(a,0x87000,3072*4)) { if (++gateStores!=dropGate) memory[a]=value; return; }
		if (Range(a,0x140000,732)) { memory[a]=value; return; }
		CoordinatorMachine::Store(a,value);
	}
	bool RunLayer() {
		while (pc!=0x64038 && pc!=0x64058) PioneerScalarStep(*this,fp);
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==0x46445243 && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==0x6400c && memory.at(0x17ffc8)==0x31445243);
		return memory.at(0x17ffc4)!=0;
	}
	void Verify() {
		assert(!pending && hostWrites==1 && hostReads==1 && finalOriginal==layerN && finalReturned==layerN);
		for (unsigned i=0;i<layerN;i++) assert(host[i]==Bits(expected[i]));
		for (unsigned i=layerN;i<4096;i++) assert(host[i]==0xffffffff);
		unsigned pg=std::min(initialLimit,96U),pd=std::min(initialLimit,32U),p=kind==4?2*pg+pd:0;
		assert(memory.at(0x17ffc4)==(kind==4?pg:1));
		assert(memory.at(0x143010)) ;
		assert(memory.at(0x143014)==layerN && memory.at(0x143018)==initialNonce && memory.at(0x14301c)==kind);
		unsigned r=memory.at(0x143000),rf=memory.at(0x143004),w=memory.at(0x143008),wf=memory.at(0x14300c);
		assert(r>=4*p+1 && !((r-4*p-1)&1) && rf==(kind==4?r-p+511:layerN/16));
		assert(w==2*p+1 && wf==2*p+layerN/16 && issued==p);
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		for (unsigned i=0;i<8;i++) assert(uint32_t(memory.at(0x143040+i*4)-memory.at(0x143020+i*4))==delta[i]);
		uint32_t summary[24],before[8],after[8];
		for (unsigned i=0;i<24;i++) summary[i]=memory.at(0x143000+i*4);
		for (unsigned i=0;i<8;i++) { before[i]=summary[8+i]; after[i]=summary[16+i]; }
		assert(PioneerLayerSummary(summary,kind,initialLimit,layerN,initialNonce,before,after));
		for (unsigned i=0;i<24;i++) { summary[i]^=1; assert(!PioneerLayerSummary(summary,kind,initialLimit,layerN,initialNonce,before,after)); summary[i]^=1; }
		if (kind==4) for (unsigned peerIndex=1;peerIndex<=pg;peerIndex++) {
			auto& m=*peer[peerIndex]; assert(m.pc==CoordWait && m.l1[m.Index(0x71304)]==(peerIndex<=pd?3U:2U));
		}
		printf("OFFLINE layer op%u peers%u rows%u steps%u stack%08x exact final outputs,one owned-host write/read PASS\n",kind,initialLimit,layerN,steps,minStack); fflush(stdout);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned n:{32U,64U,1024U,3072U}) for (bool debug:{false,true}) { LayerMachine m(argv[1],5,119,n,debug); assert(m.RunLayer()); m.Verify(); }
	for (unsigned peers:{32U,64U,119U}) { LayerMachine m(argv[1],4,peers); assert(m.RunLayer()); m.Verify(); }
	for (unsigned fault=0;fault<11;fault++) {
		LayerMachine m(argv[1],4,119);
		if (fault==0) m.dropGate=1; if (fault==1) m.dropGate=3072;
		if (fault==2) m.dropPacked=1; if (fault==3) m.dropPacked=65; // first changed down-projection operand,not an identical repeated scale copy
		if (fault==4) m.dropDoorbell=true; if (fault==5) m.dropDescriptor=0;
		if (fault==6) m.corruptProof=11; if (fault==7) m.peer[1]->failSourceRead=1;
		if (fault==8) m.memory[0x140100]=0x7ffffffc;
		if (fault==9) m.memory[0x140028]=0x82400000;
		if (fault==10) m.memory[0x14002c]=0x82400000-4096;
		bool passed=m.RunLayer();
		if (passed || m.hostWrites || m.hostReads) fprintf(stderr,"Unexpected layer fault acceptance%u\n",fault);
		assert(!passed && !m.hostWrites && !m.hostReads);
	}
	for (unsigned fault=0;fault<6;fault++) {
		LayerMachine m(argv[1],5,119,32); m.dropPosted=fault==0; m.partialPosted=fault==1; m.pairedCorrupt=fault==2;
		m.corruptHost=fault==3?0:fault==4?16:fault==5?31:~0U; assert(!m.RunLayer());
	}
	for (unsigned fault=0;fault<15;fault++) {
		LayerMachine m(argv[1],5,119,32);
		if (fault==0) m.memory[0x140000]^=1; if (fault==1) m.memory[0x140004]=6;
		if (fault==2) m.memory[0x140008]=0; if (fault==3) m.memory[0x140008]=120;
		if (fault==4) m.memory[0x140010]=0; if (fault==5) m.memory[0x140010]=3104;
		if (fault==6) m.memory[0x140010]=33; if (fault==7) m.memory[0x140018]=0;
		if (fault==8) m.memory[0x14001c]=1; if (fault==9) m.memory[0x140020]=0;
		if (fault==10) m.memory[0x140020]=0x10000; if (fault==11) m.memory[0x140024]=32768;
		if (fault==12) m.memory[0x140030]=2; if (fault==13) m.memory[0x14000c]=1024;
		if (fault==14) m.memory[0x140014]=0x04000000;
		assert(!m.RunLayer() && !m.transfers && !m.issued);
	}
	puts("PASS OFFLINE fused FFN/GLU linked RV32IMF + unchanged peer bytes,exact final math,physical summaries,host corruption/shape refusal; NOT native speed or driver exposure");
}
