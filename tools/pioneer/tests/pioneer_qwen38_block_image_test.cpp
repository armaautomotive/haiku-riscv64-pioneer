/* SPDX-License-Identifier: MIT */
#define PIONEER_RUNTIME_MACHINE_ONLY
#include "pioneer_runtime_matvec_image_test.cpp"
#include "../PioneerQwen38Panel.h"
#include "../PioneerQwen38Gguf.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BlockProbeImage.h"
#include <map>
#include <string>
struct BlockMachine:RuntimeMachine {
	std::vector<uint8_t> code,tile;
#ifdef PIONEER_Q38_TEST_INT_ACCUMULATOR
	std::vector<uint8_t> affineCode;
#endif
#ifdef PIONEER_Q38_TEST_SHARED_WORKER_STORAGE
	::PioneerQ38SharedTestMap memory;
#else
	std::map<uint32_t,uint32_t> memory;
#endif
	uint32_t fp[32]={}; unsigned type,count,floatOps=0,minStack=0x70000;
	float scale;
	BlockMachine(const char* root,unsigned kind,unsigned trial,float activationScale,const uint8_t* source=nullptr,bool stream=false)
		:RuntimeMachine(1024,32,0,0x04000000,false,true,true),type(kind),count(kind==8?32:256),scale(activationScale) {
		std::string path(root); code=Read((path+"/text.bin").c_str()); auto boot=Read((path+"/boot.bin").c_str());
		assert(boot.size()==4 && !code.empty() && code.size()<=8192 && !(code.size()&3));
		assert(Word(boot,0)==TT_Q38_BLOCK_BOOT);
		if (!stream) for (unsigned w=0;w<2048;w++) assert(PioneerQ38BlockWord(w)==(w<code.size()/4?Word(code,w*4):0));
		l1[0]=Word(boot,0); for (unsigned w=0;w<code.size()/4;w++) l1[Index(0x72000+w*4)]=Word(code,w*4);
#ifdef PIONEER_Q38_TEST_INT_ACCUMULATOR
		// A cooperative test also constructs the UNCHANGED root, which has
		// no affine helper. The integer peer constructor requires its pin.
		if (FILE* f=fopen((path+"/affine.bin").c_str(),"rb")) {
			fclose(f); affineCode=Read((path+"/affine.bin").c_str());
			assert(!affineCode.empty() && affineCode.size()<=0x1500 && !(affineCode.size()&3));
			for (unsigned w=0;w<affineCode.size()/4;w++) l1[Index(0x70b00+w*4)]=Word(affineCode,w*4);
		}
#endif
		tile.assign(PioneerQ38PanelTileBytes(type),0); unsigned bytes=PioneerQ38BlockBytes(type);
		for (unsigned row=0;row<16;row++) {
			for (unsigned b=0;b<bytes;b++) tile[b*16+row]=uint8_t(b*37+row*59+trial*13);
			unsigned d=type==14?208:0; uint16_t half=row%3?uint16_t(0x3c00+row*13):uint16_t(1+row);
			tile[d*16+row]=uint8_t(half); tile[(d+1)*16+row]=uint8_t(half>>8);
			if (type==12) { tile[32+row]=0; tile[48+row]=0x38; }
			if (type==8) tile[32+row]=128;
		}
		if (source) memcpy(tile.data(),source,tile.size());
		for (unsigned w=0;w<tile.size()/4;w++) memory[0x90000+w*4]=Word(tile,w*4);
		memory[0x74000]=0x31423851; memory[0x74004]=type; memory[0x74008]=unsigned(tile.size());
		memory[0x7400c]=0; memory[0x74010]=Bits(scale);
		for (unsigned w=0;w<160;w++) memory[0x91000+w*4]=0xdeadbeef;
		inputs=count; outputs=16; vector.resize(count);
		for (unsigned k=0;k<count;k++) {
			vector[k]=PioneerInt8Operand(int((k*29+trial*17)%256)-128);
			if (!(k&1)) buffers[k/2]=0; buffers[k/2]|=uint32_t(vector[k])<<((k%2)*16);
		}
	}
	static bool Range(uint32_t a,uint32_t first,unsigned bytes) { return a>=first && a-first<bytes; }
	unsigned Index(uint32_t a) override {
		assert(!(a&3)); if (Range(a,0x72000,8192)) return 2048+(a-0x72000)/4;
#ifdef PIONEER_Q38_TEST_INT_ACCUMULATOR
		if (Range(a,0x71000,4096)) return 4096+(a-0x71000)/4; // disjoint from main/status/operand indices
#endif
		return RuntimeMachine::Index(a);
	}
	bool CodeAddress(uint32_t a) const override { return a==0x6000 || Range(a,0x72000,unsigned(code.size()))
#ifdef PIONEER_Q38_TEST_INT_ACCUMULATOR
		|| Range(a,0x70b00,unsigned(affineCode.size()))
#endif
		; }
	unsigned StepLimit() const override { return 10000000; }
	uint32_t Load(uint32_t a) override {
		assert(!(a&3) && !Range(a,0xffb20000,0x4000)); // NO NoC/PCI/DMA access
#ifdef PIONEER_Q38_TEST_INT_ACCUMULATOR
		if (Range(a,0x70b00,unsigned(affineCode.size()))) return l1[Index(a)];
#endif
		if (Range(a,0x72000,unsigned(code.size()))) return l1[Index(a)];
		if (Range(a,0x6e000,8192) || Range(a,0x74000,20) || Range(a,0x90000,unsigned(tile.size())) || Range(a,0x91000,640)) {
			assert(memory.count(a)); return memory.at(a);
		}
		if (Range(a,0x80000,0x4000)) assert(Range(a,0x80000,count*2));
		return RuntimeMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		assert(!(a&3) && !Range(a,0xffb20000,0x4000));
		if (Range(a,0x6e000,8192)) { minStack=std::min(minStack,unsigned(a)); memory[a]=value; return; }
		if (Range(a,0x91000,640)) { memory[a]=value; return; }
		if (Range(a,0x70700,0x320)) {
			assert(drained);
			bool header=a<0x70710 || (a>=0x70910 && a<0x70920);
			if (header) assert(!value);
			else for (unsigned lane=0;lane<2;lane++) { uint16_t v=uint16_t(value>>(lane*16)); assert(!v || PioneerInt8OperandValid(v)); }
			l1[Index(a)]=value; return;
		}
		if (Range(a,0x84000,0x4000)) assert(Range(a,0x84000,64));
		RuntimeMachine::Store(a,value);
	}
	void Store16(uint32_t,uint16_t) override { assert(false && "unqualified half-store"); }
	virtual void StepBlock() {
		uint32_t i=l1[Index(pc)],op=i&127;
		if (op!=7 && op!=0x27 && op!=0x53) { Step(); return; }
		assert(CodeAddress(pc) && ++steps<StepLimit()); floatOps++;
		unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
		if (op==7) { assert(f==2); fp[d]=Load(regs[s]+Sign(i>>20,12)); }
		else if (op==0x27) { assert(f==2); Store(regs[s]+Sign((hi<<5)|d,12),fp[t]); }
		else {
			float a=Float(fp[s]),b=Float(fp[t]);
			assert(!(fp[s]&0x7fffffff) || (fp[s]&0x7f800000));
			assert(!(fp[t]&0x7fffffff) || (fp[t]&0x7f800000));
			switch (hi) {
				case 0: assert(f==7 || !f); fp[d]=Bits(a+b); break;
				case 8: assert(f==7 || !f); fp[d]=Bits(a*b); break;
				case 0x50: assert(f<=2); if (d) regs[d]=f==2?a==b:f==1?a<b:a<=b; break;
				case 0x68: assert(t<=1 && (f==7 || !f)); fp[d]=Bits(t?float(regs[s]):float(int32_t(regs[s]))); break;
				case 0x70: assert(!t && !f); if (d) regs[d]=fp[s]; break;
				case 0x78: assert(!t && !f); fp[d]=regs[s]; break;
				default: fprintf(stderr,"UNQUALIFIED block FP instruction%08x pc=%08x\n",i,pc); assert(false);
			}
		}
		regs[0]=0; pc+=4;
	}
	bool RunBlock() {
		while (pc!=0x72038 && pc!=0x72058) StepBlock();
		assert(minStack>=0x6e000 && !requests && !responses);
		if (pc==0x72058) return false;
		assert(l1[Index(0x17ffc0)]==0x7200c && l1[Index(0x17ffc8)]==0x31423851);
		return l1[Index(0x17ffc4)]==count/16;
	}
	bool CorrectBlock() const {
		if (matrixInstructions!=4*(count/16) || !floatOps) return false;
		float expected[16]={};
		for (unsigned p=0;p<count/16;p++) {
			PioneerQ38Panel panel; assert(PioneerQ38UnpackPanel(type,tile.data(),unsigned(tile.size()),p,&panel));
			int inputSum=0; for (unsigned k=p*16;k<(p+1)*16;k++) inputSum+=int(vector[k]&1023)*((vector[k]&0x8000)?-1:1);
			for (unsigned row=0;row<16;row++) {
				int dot=0;
				for (unsigned k=0;k<16;k++) {
					unsigned e=k*16+row; uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16)),a=vector[p*16+k];
					dot+=int(w&1023)*((w&0x8000)?-1:1)*int(a&1023)*((a&0x8000)?-1:1);
				}
				float product=(Float(panel.coefficient[row])*scale)*float(dot),correction=(Float(panel.minimum[row])*scale)*float(inputSum);
				expected[row]+=product+correction;
			}
		}
		for (unsigned row=0;row<16;row++) {
			if (buffers[4096+row]!=Bits(expected[row])) return false;
			uint8_t raw[210]={}; unsigned bytes=PioneerQ38BlockBytes(type);
			for (unsigned b=0;b<bytes;b++) raw[b]=tile[b*16+row];
			float values[256]={}; assert(type==8?PioneerDecodeQ8_0(raw,values):type==12?PioneerDecodeQ4K(raw,values):PioneerDecodeQ6K(raw,values));
			double reference=0,absolute=0;
			for (unsigned k=0;k<count;k++) { double product=double(values[k])*int(vector[k]&1023)*((vector[k]&0x8000)?-1:1)*scale; reference+=product; absolute+=fabs(product); }
			if (fabs(double(expected[row])-reference)>1e-6+absolute*1e-6) return false;
		}
		for (unsigned row=16;row<4096;row++) if (buffers[4096+row]!=0x7fc00000) return false;
		return true;
	}
};
#ifndef PIONEER_Q38_BLOCK_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2 || argc==3);
	for (unsigned type:{8U,12U,14U}) for (float scale:{0.0f,-0.0f,.012345f,-.03125f,1.0f,0x1p-32f,0x1p16f}) {
		BlockMachine m(argv[1],type,1,scale); auto before=m.memory; assert(m.RunBlock() && m.CorrectBlock());
		for (const auto& word:before) if (BlockMachine::Range(word.first,0x90000,3392) || BlockMachine::Range(word.first,0x74000,20)) assert(m.memory.at(word.first)==word.second);
	}
	for (unsigned type:{8U,12U,14U}) for (unsigned fault=0;fault<4;fault++) {
		BlockMachine m(argv[1],type,1,.012345f); m.dropMatrix=fault==0; m.dropUnpack=fault==1; m.dropClear=fault==2;
		if (fault==3) m.memory[0x90000+16*16]^=0x01010101;
		assert(!m.RunBlock() || !m.CorrectBlock());
	}
	for (uint32_t bits:{0x7f800000U,0xff800000U,0x7fc00001U,1U,0x2f7fffffU,0x47800001U}) {
		BlockMachine m(argv[1],12,1,Float(bits)); assert(!m.RunBlock() && !m.matrixInstructions);
	}
	for (unsigned type:{8U,12U,14U}) {
		for (unsigned fault=0;fault<4;fault++) {
			BlockMachine m(argv[1],type,1,.012345f);
			m.memory[0x74000+fault*4]^=1;
			assert(!m.RunBlock() && !m.matrixInstructions);
		}
		for (unsigned word=0;word<(type==8?16U:128U);word++) {
			BlockMachine m(argv[1],type,1,.012345f); m.buffers[word]=0;
			assert(!m.RunBlock() && !m.matrixInstructions);
		}
	}
	unsigned real=0;
	if (argc==3) {
		FILE* f=fopen(argv[2],"rb"); assert(f); PioneerQwen38Gguf model; assert(model.Open(f));
		for (unsigned t=0;t<model.Tensors().size();t++) {
			const auto& tensor=model.Tensors()[t]; if (!tensor.type) continue;
			unsigned block=PioneerQ38BlockBytes(tensor.type),rowBytes=unsigned(tensor.bytes/tensor.outputs);
			for (unsigned end=0;end<2;end++) {
				uint8_t tile[3392]={},raw[210]={}; unsigned first=end?unsigned(tensor.outputs)-16:0;
				for (unsigned row=0;row<16;row++) {
					assert(model.ReadPacked(t,uint64_t(first+row)*rowBytes+(end?rowBytes-block:0),block,raw,block));
					for (unsigned b=0;b<block;b++) tile[b*16+row]=raw[b];
				}
				BlockMachine m(argv[1],tensor.type,1,end?-.03125f:.012345f,tile); assert(m.RunBlock() && m.CorrectBlock()); real++;
			}
		}
		assert(real==996 && model.Stable()); fclose(f);
	}
	printf("PASS OFFLINE actual complete Q8/Q4/Q6 block matrices+ALL2/16 affine accumulation,separate FP32 and dequantized-double references,%u real blocks,signed scale limits/immutable inputs/zero NoC/fault sensitivity;NOT fullK/resident/native/inference/speed\n",real);
}
#endif
