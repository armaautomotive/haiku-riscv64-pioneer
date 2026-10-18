/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PANEL_MACHINE_ONLY
#include "pioneer_qwen38_panel_image_test.cpp"
struct AffineMachine:PanelMachine {
	uint32_t fp[32]={}; unsigned floatOps=0;
	float scale;
	AffineMachine(const char* root,unsigned type,unsigned panel,unsigned trial,float activationScale,const uint8_t* tile=nullptr)
		:PanelMachine(root,type,panel,trial,tile,true),scale(activationScale) { memory[0x74010]=Bits(scale); }
	void StepPanel() override {
		uint32_t i=l1[Index(pc)],op=i&127;
		if (op!=7 && op!=0x27 && op!=0x53) { Step(); return; }
		assert(CodeAddress(pc) && Range(pc,0x72000,unsigned(unpack.size())) && ++steps<StepLimit()); floatOps++;
		unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
		if (op==7) { assert(f==2); fp[d]=Load(regs[s]+Sign(i>>20,12)); }
		else if (op==0x27) { assert(f==2); Store(regs[s]+Sign((hi<<5)|d,12),fp[t]); }
		else {
			float a=Float(fp[s]),b=Float(fp[t]);
			// Explicit probe range means every nonzero operand is NORMAL;
			// do not silently model unsupported scalar denormal preservation.
			assert(!(fp[s]&0x7fffffff) || (fp[s]&0x7f800000));
			assert(!(fp[t]&0x7fffffff) || (fp[t]&0x7f800000));
			switch (hi) {
				case 0: assert(f==7 || !f); fp[d]=Bits(a+b); break;
				case 8: assert(f==7 || !f); fp[d]=Bits(a*b); break;
				case 0x68: assert(t<=1 && (f==7 || !f)); fp[d]=Bits(t?float(regs[s]):float(int32_t(regs[s]))); break;
				case 0x70: assert(!t && !f); if (d) regs[d]=fp[s]; break;
				case 0x78: assert(!t && !f); fp[d]=regs[s]; break;
				default: fprintf(stderr,"UNQUALIFIED affine FP instruction%08x pc=%08x\n",i,pc); assert(false);
			}
		}
		regs[0]=0; pc+=4;
	}
	bool CorrectAffine() const {
		if (panelWords!=160 || copied!=128 || matrixInstructions!=4 || !floatOps) return false;
		uint32_t actual[160]; for (unsigned w=0;w<160;w++) actual[w]=memory.at(0x91000+w*4);
		if (memcmp(actual,&expected,sizeof(expected))) return false;
		int inputSum=0; for (unsigned k=0;k<16;k++) inputSum+=int(vector[k]&1023)*((vector[k]&0x8000)?-1:1);
		for (unsigned row=0;row<16;row++) {
			int sum=0;
			for (unsigned k=0;k<16;k++) {
				unsigned element=k*16+row; uint16_t w=uint16_t(expected.operand[element/2]>>((element%2)*16));
				sum+=int(w&1023)*((w&0x8000)?-1:1)*int(vector[k]&1023)*((vector[k]&0x8000)?-1:1);
			}
			float product=(Float(expected.coefficient[row])*scale)*float(sum);
			float correction=(Float(expected.minimum[row])*scale)*float(inputSum);
			if (buffers[4096+row]!=Bits(product+correction)) return false;
		}
		for (unsigned row=16;row<4096;row++) if (buffers[4096+row]!=0x7fc00000) return false;
		return true;
	}
};
int main(int argc,char** argv)
{
	assert(argc==2 || argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned panel=0;panel<(type==8?2U:16U);panel++)
		for (float scale:{0.0f,-0.0f,.012345f,-.03125f,1.0f,0x1p-32f,0x1p16f}) {
			AffineMachine m(argv[1],type,panel,1,scale); auto before=m.memory;
			assert(m.RunPanel() && m.CorrectAffine());
			for (const auto& word:before) if (AffineMachine::Range(word.first,0x74000,20) || AffineMachine::Range(word.first,0x90000,3392)) assert(m.memory.at(word.first)==word.second);
		}
	for (uint32_t bad:{0x7f800000U,0xff800000U,0x7fc00001U,1U,0x2f7fffffU,0x47800001U}) {
		AffineMachine m(argv[1],12,0,1,Float(bad)); assert(!m.RunPanel());
		assert(m.matrixInstructions==4 && m.panelWords==160); // no SUCCESS publication
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
				for (unsigned panel:{0U,tensor.type==8?1U:15U}) {
					AffineMachine m(argv[1],tensor.type,panel,1,end?-.03125f:.012345f,tile);
					assert(m.RunPanel() && m.CorrectAffine()); real++;
				}
			}
		}
		assert(real==1992 && model.Stable()); fclose(f);
	}
	printf("PASS OFFLINE actual mixed integer matrix+on-card affine epilogue,ALL34 panels/seven signed scale boundaries,%u real panels,BIT_EXACT local arithmetic/strict FP+memory/zero NoC;NOT resident/native/model inference or speed\n",real);
}
