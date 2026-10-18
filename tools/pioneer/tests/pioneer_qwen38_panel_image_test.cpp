/* SPDX-License-Identifier: MIT */
#define PIONEER_RUNTIME_MACHINE_ONLY
#include "pioneer_runtime_matvec_image_test.cpp"
#include "../PioneerQwen38Panel.h"
#include "../PioneerQwen38Gguf.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PanelProbeImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38AffineProbeImage.h"
#include <map>
#include <string>
struct PanelMachine:RuntimeMachine {
	std::vector<uint8_t> bridge,unpack;
	std::map<uint32_t,uint32_t> memory;
	PioneerQ38Panel expected={};
	unsigned panelWords=0,copied=0,minStack=0x70000;
	bool dropCopy=false,affineImage=false;
	PanelMachine(const char* directory,unsigned type,unsigned panel,unsigned trial,const uint8_t* sourceTile=nullptr,bool affine=false):RuntimeMachine(1024,32,0,0x04000000,false,true,true),affineImage(affine) {
		std::string root(directory);
		auto main=Read((root+"/text.bin").c_str()),boot=Read((root+"/boot.bin").c_str());
		bridge=Read((root+"/bridge.bin").c_str()); unpack=Read((root+"/unpack.bin").c_str());
		assert(main.size()==4096 && boot.size()==4 && bridge.size()==(affine?136U:100U) && unpack.size()<=8192);
		l1[0]=Word(boot,0);
		for (unsigned w=0;w<1024;w++) {
			// Exactly ONE jump differs from the prior integer matrix page.
			assert(Word(main,w*4)==(affine && w==0x3c8/4?0x49d0006f:w==0x2ac/4?0x5550006f:BlackholeRuntimeInt8MatvecCodeWord(w)));
			l1[1+w]=Word(main,w*4);
			assert((affine?PioneerQ38AffineMainWord(w):PioneerQ38PanelMainWord(w))==Word(main,w*4));
		}
		if (!affine) {
			for (unsigned w=0;w<1024;w++) assert(PioneerQ38PanelBridgeWord(w)==(w<bridge.size()/4?Word(bridge,w*4):0));
			for (unsigned w=0;w<2048;w++) assert(PioneerQ38PanelUnpackWord(w)==(w<unpack.size()/4?Word(unpack,w*4):0));
		} else {
			for (unsigned w=0;w<1024;w++) assert(PioneerQ38AffineBridgeWord(w)==(w<bridge.size()/4?Word(bridge,w*4):0));
			for (unsigned w=0;w<2048;w++) assert(PioneerQ38AffineUnpackWord(w)==(w<unpack.size()/4?Word(unpack,w*4):0));
		}
		for (unsigned w=0;w<bridge.size()/4;w++) l1[Index(0x71000+w*4)]=Word(bridge,w*4);
		for (unsigned w=0;w<unpack.size()/4;w++) l1[Index(0x72000+w*4)]=Word(unpack,w*4);
		inputs=outputs=16; l1[Index(0x70500)]=0; l1[Index(0x70504)]=l1[Index(0x70508)]=1;
		uint8_t tile[3392]={}; unsigned bytes=PioneerQ38PanelTileBytes(type),block=PioneerQ38BlockBytes(type);
		for (unsigned row=0;row<16;row++) {
			for (unsigned b=0;b<block;b++) tile[b*16+row]=uint8_t(row*17+b*29+trial*59);
			unsigned d=type==14?208:0; uint16_t half=row%3?uint16_t(0x3c00+row*13):uint16_t(1+row);
			tile[d*16+row]=uint8_t(half); tile[(d+1)*16+row]=uint8_t(half>>8);
			if (type==12) { tile[2*16+row]=0; tile[3*16+row]=0x38; }
			if (type==8) tile[(2+panel*16)*16+row]=128; // full-range -128 NOT clipped/refused
		}
		if (sourceTile) memcpy(tile,sourceTile,bytes);
		assert(PioneerQ38UnpackPanel(type,tile,bytes,panel,&expected));
		memory[0x74000]=0x50383355; memory[0x74004]=type; memory[0x74008]=bytes; memory[0x7400c]=panel;
		for (unsigned w=0;w<bytes/4;w++) memory[0x90000+w*4]=tile[w*4]|(uint32_t(tile[w*4+1])<<8)|(uint32_t(tile[w*4+2])<<16)|(uint32_t(tile[w*4+3])<<24);
		for (unsigned w=0;w<160;w++) memory[0x91000+w*4]=0xdeadbeef;
		for (unsigned k=0;k<16;k++) {
			vector[k]=PioneerInt8Operand(int((k*29+trial*17)%256)-128);
			if (!(k&1)) buffers[k/2]=0;
			buffers[k/2]|=uint32_t(vector[k])<<(16*(k&1));
		}
	}
	static bool Range(uint32_t a,uint32_t first,unsigned bytes) { return a>=first && a-first<bytes; }
	unsigned Index(uint32_t a) override {
		assert(!(a&3));
		if (Range(a,0x71000,0x3000)) return 2048+(a-0x71000)/4;
		return RuntimeMachine::Index(a);
	}
	bool CodeAddress(uint32_t a) const override {
		return a==0x6000 || Range(a,0x70000,0x500) || Range(a,0x70540,0x40) || Range(a,0x70600,0x60)
			|| Range(a,0x71000,unsigned(bridge.size())) || Range(a,0x72000,unsigned(unpack.size()));
	}
	uint32_t Load(uint32_t a) override {
		assert(!(a&3) && !Range(a,0xffb20000,0x4000)); // ZERO NoC/PCI/DMA access
		if (Range(a,0x71000,unsigned(bridge.size())) || Range(a,0x72000,unsigned(unpack.size()))) return l1[Index(a)];
		if (Range(a,0x6e000,8192) || Range(a,0x74000,affineImage?20:16) || Range(a,0x90000,3392) || Range(a,0x91000,640)) {
			assert(memory.count(a)); return memory.at(a);
		}
		return RuntimeMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t v) override {
		assert(!(a&3) && !Range(a,0xffb20000,0x4000));
		if (Range(a,0x6e000,8192)) { minStack=std::min(minStack,unsigned(a)); memory[a]=v; return; }
		if (Range(a,0x91000,640)) { panelWords++; memory[a]=v; return; }
		if (Range(a,0x70710,512)) {
			assert(pc==0x71034 && drained && copied<128); copied++;
			if (!dropCopy) l1[Index(a)]=v;
			return;
		}
		if (a==0x17ffc8 && v==0x46383350) {
			assert((!matrixInstructions && !panelWords) || (affineImage && matrixInstructions==4 && panelWords==160)); memory[a]=v; return;
		}
		RuntimeMachine::Store(a,v);
	}
	void Store16(uint32_t,uint16_t) override { assert(false && "unqualified half-store in integer-only unpacker"); }
	virtual void StepPanel() { Step(); }
	bool RunPanel() {
		while (pc!=ParkPc() && pc!=0x71060) StepPanel();
		assert(!requests && !responses && minStack>=0x6e000);
		if (pc==0x71060) return false;
		assert(l1[Index(0x17ffc0)]==FinalPhase() && l1[Index(0x17ffc4)]==1 && l1[Index(0x17ffc8)]==0x4d415431);
		return true;
	}
	bool CorrectPanel() const {
		if (panelWords!=160 || copied!=128 || matrixInstructions!=4) return false;
		uint32_t actual[160]; for (unsigned w=0;w<160;w++) actual[w]=memory.at(0x91000+w*4);
		if (memcmp(actual,&expected,sizeof(expected))) return false;
		for (unsigned row=0;row<16;row++) {
			int sum=0;
			for (unsigned k=0;k<16;k++) {
				unsigned element=k*16+row; uint16_t operand=uint16_t(expected.operand[element/2]>>((element%2)*16));
				int weight=int(operand&1023)*((operand&0x8000)?-1:1);
				int activation=int(vector[k]&1023)*((vector[k]&0x8000)?-1:1); sum+=weight*activation;
			}
			if (int32_t(buffers[4096+row])!=sum) return false;
		}
		for (unsigned row=16;row<4096;row++) if (buffers[4096+row]!=0x7fc00000) return false;
		return true;
	}
};
#ifndef PIONEER_Q38_PANEL_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2 || argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned p=0;p<(type==8?2U:16U);p++) for (unsigned trial=0;trial<2;trial++) {
		PanelMachine m(argv[1],type,p,trial); auto before=m.memory;
		assert(m.RunPanel() && m.CorrectPanel());
		for (const auto& word:before) if (PanelMachine::Range(word.first,0x90000,3392) || PanelMachine::Range(word.first,0x74000,16)) assert(m.memory.at(word.first)==word.second);
	}
	for (unsigned type:{8U,12U,14U}) {
		for (unsigned field=0;field<3;field++) for (unsigned bit=0;bit<32;bit++) {
			PanelMachine m(argv[1],type,0,1); m.memory[0x74000+field*4]^=1U<<bit;
			assert(!m.RunPanel() && !m.panelWords && !m.matrixInstructions);
		}
		for (unsigned bit=4;bit<32;bit++) {
			PanelMachine m(argv[1],type,0,1); m.memory[0x7400c]=1U<<bit;
			assert(!m.RunPanel() && !m.panelWords && !m.matrixInstructions);
		}
		for (unsigned fault=0;fault<3;fault++) {
			PanelMachine m(argv[1],type,0,1);
			m.dropMatrix=fault==0; m.dropUnpack=fault==1; m.dropCopy=fault==2;
			assert(m.RunPanel() && !m.CorrectPanel());
		}
		// Scale NaN/infinity refuses before publishing a panel or starting
		// any matrix instruction; no dirty partial output is accepted.
		for (unsigned row=0;row<16;row++) {
			PanelMachine m(argv[1],type,0,1); unsigned at=(type==14?208:0)*16+row;
			for (unsigned byte=0;byte<2;byte++) {
				uint32_t address=0x90000+at+byte*16,shift=(address&3)*8;
				m.memory[address&~3U]=(m.memory.at(address&~3U)&~(255U<<shift))|((byte?0x7cU:1U)<<shift);
			}
			assert(!m.RunPanel() && !m.panelWords && !m.matrixInstructions);
		}
	}
	unsigned real=0;
	if (argc==3) {
		FILE* file=fopen(argv[2],"rb"); assert(file); PioneerQwen38Gguf model; assert(model.Open(file));
		for (unsigned tensor=0;tensor<model.Tensors().size();tensor++) {
			const auto& t=model.Tensors()[tensor]; if (!t.type) continue;
			unsigned block=PioneerQ38BlockBytes(t.type),rowBytes=unsigned(t.bytes/t.outputs);
			for (unsigned end=0;end<2;end++) {
				uint8_t raw[210]={},tile[3392]={}; unsigned first=end?unsigned(t.outputs)-16:0;
				for (unsigned row=0;row<16;row++) {
					assert(model.ReadPacked(tensor,uint64_t(first+row)*rowBytes+(end?rowBytes-block:0),block,raw,block));
					for (unsigned byte=0;byte<block;byte++) tile[byte*16+row]=raw[byte];
				}
				for (unsigned panel:{0U,t.type==8?1U:15U}) {
					PanelMachine m(argv[1],t.type,panel,1,tile); assert(m.RunPanel() && m.CorrectPanel()); real++;
				}
			}
		}
		assert(real==1992 && model.Stable()); fclose(file);
	}
	printf("PASS OFFLINE actual RV32IM unpack+four-fidelity Tensix integer panels,ALL34 panel shapes/full-range -128,%u real-checkpoint panels,exact640B metadata/immutable image bytes,strict memory/zero NoC/descriptor+fault refusals;NOT native or model inference\n",real);
}
#endif
