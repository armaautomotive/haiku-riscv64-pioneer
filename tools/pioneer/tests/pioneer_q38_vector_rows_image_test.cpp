/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_HAS_ROWS_MACHINE
#define PIONEER_Q38_SHARED_ROWS_MACHINE_ONLY
#include "pioneer_qwen38_rows_image_test.cpp"
#endif
#include <memory>
#include "../PioneerBlackholeFma.h"
#include "../PioneerQ38VectorFixedFormat.h"
// ACTUAL compiled RV32/FPU image plus independent exact integer SFPU subset.
// Not a silicon cycle model or permission to select a new native image.
template<class Base,bool Persistent=false> struct VectorRows:Base {
	using Base::memory; using Base::config; using Base::thread; using Base::ownerA;
	using Base::pending; using Base::drained; using Base::tile; using Base::tiles; using Base::backendWait;
	using Base::adcX0; using Base::adcX1; using Base::pc; using Base::configWrites;
	using Base::Range; using Base::buffers; using Base::groups; using Base::inputs;
	using Base::requests; using Base::responses; using Base::minStack; using Base::l1;
	using Base::Index; using Base::StepBlock; using Base::Operand16;
	using Base::moveInstructions; using Base::a;
	using Base::dstInt; using Base::outputReads;
	uint16_t physical[1024][16]={}; uint32_t lr[8][32]={};
	bool wrote[1024][16]={},sources[2]={},rawPending=false,publish=false,enabled=false;
	unsigned unpackBank=0,mathBank=0,rawAddress=0,rawRow=0,adc0=0,adc1=255;
	unsigned rawReads=0,vectorOps=0,panelMoves=0,rawUnpacks=0;
	unsigned setupConfiguration=25;
	bool corruptMove=false,corruptDotRange=false,negativeDotZero=false;
	bool rawFloat=false,affineEligible=false;
	VectorRows(const char* path,unsigned type,unsigned k,unsigned groups):Base(path,type,k,groups) { if (!Persistent) memory[0x74000]=0x31563851; }
	VectorRows(const char* path,unsigned type,unsigned k,unsigned groups,unsigned worker):Base(path,type,k,groups,worker) { static_assert(Persistent,"worker routing ONLY in the persistent model"); }
	bool InlineTensix() const override { return true; }
	uint32_t MatrixArithmeticConfig1() const override {
		// Read-only model of the pinned MVMUL fields,NOT a change to CSR
		// state:only the two proven ignored fields may differ under forced
		// A/B FP16 interpretation and disabled implied formats. All source
		// ownership,integer phases and input/output bits stay unmodified.
		return PioneerQ38VectorFixedMatrixEquivalent(config[0],config[1],thread[2],thread[3])?0xa0220000:config[1];
	}
	static unsigned Adj(unsigned row) { return ((row&0x1f8)<<1)|(row&0x207); }
	uint32_t FloatDst(unsigned row,unsigned col) const {
		unsigned hi=Adj(row),lo=hi+8; assert(wrote[hi][col] && wrote[lo][col]);
		uint16_t h=physical[hi][col]; unsigned ieee=(h&0x8000)|((h&255)<<7)|((h>>8)&127);
		return (ieee<<16)|physical[lo][col];
	}
	void StoreFloatDst(unsigned row,unsigned col,uint32_t value) {
		unsigned hi=Adj(row),lo=hi+8,h=value>>16;
		physical[hi][col]=uint16_t((h&0x8000)|((h&127)<<8)|((h>>7)&255));
		physical[lo][col]=uint16_t(value); wrote[hi][col]=wrote[lo][col]=true;
	}
	void Instruction(uint32_t op) override {
		unsigned code=op>>24;
		if (code==0x42 && !(op&0x800000)) {
			assert(!rawPending && !pending && !sources[unpackBank] && !ownerA
				&& !config[49] && config[73]==0x13 && thread[5]==0 && !adc0);
			rawFloat=config[72]==0;
			rawAddress=(config[76]+1)*16; rawRow=((config[84]&65535)/16)-4;
			if (rawFloat) {
				assert(affineEligible && config[1]==0xa0200000 && config[64]==0x100010 && config[65]==1
					&& adc1==15 && op==0x42000081 && (rawRow==96 || rawRow==100 || rawRow==104));
				assert(rawAddress==(rawRow==96?0x91200U:rawRow==100?0x91240U:rawAddress));
				if (rawRow==104) assert(!(rawAddress&63) && Range(rawAddress,0x84000,groups*64));
			} else {
				assert(config[1]==0xa03c8000 && config[64]==0x10001e && config[65]==16 && config[72]==14
					&& adc1==255 && (op==0x42000081 || op==0x420000c1));
				assert(rawRow==0 || rawRow==16);
				assert(rawAddress>=0x90000 && rawAddress-0x90000+256<=tile.size());
				if (!rawRow) { memset(physical,0,sizeof(physical)); memset(wrote,0,sizeof(wrote)); panelMoves=0; affineEligible=false; }
			}
			rawPending=true; publish=op&64; rawUnpacks++; drained=false; return;
		}
		if (op==0x5e23fc00) { adc0=adcX0[0]=0; adc1=adcX1[0]=255; return; }
		if (op==0x5e203c00) { adc0=adcX0[0]=0; adc1=adcX1[0]=15; return; }
		if (op==0x5120000a || op==0x5420000f) { assert(!rawPending); return; }
		if (op==0x4500017e && rawPending) {
			assert(backendWait && !pending);
			if (rawFloat) for (unsigned i=0;i<16;i++) { StoreFloatDst(rawRow,i,Base::Load(rawAddress+i*4)); rawReads+=4; }
			else for (unsigned i=0;i<256;i++) {
				unsigned address=rawAddress+i; uint32_t word=memory.at(address&~3U),byte=(word>>(8*(address&3)))&255;
				physical[rawRow+i/16][i%16]=uint16_t((byte<<5)|(byte?16:0)); wrote[rawRow+i/16][i%16]=true; rawReads++;
			}
			if (publish) { sources[unpackBank]=true; unpackBank^=1; ownerA=sources[mathBank]; }
			rawPending=false;
		}
		if (op==0x8f000000) { vectorOps++; drained=false; return; }
		if ((code>=0x70 && code<=0x8d) || code==8 || code==0x90) {
			assert(!rawPending && !pending && ((ownerA && sources[mathBank]) || affineEligible) && thread[2]==1
				&& (config[1]==0xa0220000 || PioneerQ38VectorFixedMatrixEquivalent(config[0],config[1],thread[2],thread[3]))
				&& config[220]==0 && thread[5]==4);
			vectorOps++;
			if (op==0x8a000002) { enabled=true; return; }
			assert(enabled);
			if (code==0x70 || code==0x72) {
				unsigned mode=(op>>16)&15,reg=(op>>20)&15,addr=op&1023;
				assert(reg<8 && !(op&0xfc00) && !(addr&1));
				for (unsigned lane=0;lane<32;lane++) {
					unsigned row=(addr&~3U)+lane/8,col=(lane&7)*2+((addr&2)!=0);
					if (code==0x70 && mode==5) { assert(ownerA && reg<2 && row<32 && wrote[row][col]); uint16_t v=physical[row][col]; lr[reg][lane]=(uint32_t(v>>15)<<31)|((v>>5)&255); }
					else if (code==0x70 && mode==4) { assert(affineEligible && !reg && row>=32 && row<36); int v=dstInt[row][col]; lr[0][lane]=(v<0 || (!v && negativeDotZero)?0x80000000U:0)|unsigned(v<0?-v:v); }
					else if (code==0x70 && mode==3) { assert(affineEligible && (reg==1 || reg==2 || reg==5) && row>=96 && row<108); lr[reg][lane]=FloatDst(row,col); }
					else if (code==0x72 && mode==3) { assert(affineEligible && !reg && row>=108 && row<112); StoreFloatDst(row,col,lr[reg][lane]); }
					else {
						assert(code==0x72 && mode==7); unsigned hi=Adj(row),lo=hi+8;
						if (!reg) assert(ownerA && row>=64 && row<80 && hi>=128 && hi<160 && !wrote[hi][col] && !wrote[lo][col]);
						else assert(affineEligible && reg==7 && row>=96 && row<108 && !lr[reg][lane]);
						physical[hi][col]=uint16_t(lr[reg][lane]>>16); physical[lo][col]=uint16_t(lr[reg][lane]); wrote[hi][col]=wrote[lo][col]=true;
					}
				}
				drained=false; return;
			}
			if (code==8) {
				assert(ownerA && sources[mathBank]);
				unsigned source=(op>>17)&63,row=op&1023;
				assert(!(op&0x0081dc00) && (op&0x2000) && source==panelMoves*4 && row==64+source && source<=12);
				for (unsigned k=0;k<4;k++) for (unsigned col=0;col<16;col++) {
					unsigned hi=Adj(row+k),lo=hi+8; assert(wrote[hi][col] && wrote[lo][col] && !physical[lo][col]);
					uint16_t v=physical[hi][col];
					unsigned src=((uint32_t(v)&0xffe0)<<3)|(v&31);
					uint16_t overlay=uint16_t(((src&0x40000)>>3)|((src&31)<<10)|((src>>8)&1023));
					a[source+k][col]=Operand16(overlay)+(corruptMove && !source && !k && !col?1:0);
				}
				panelMoves++; moveInstructions++; drained=false; return;
			}
			unsigned vc=(op>>8)&15,vd=(op>>4)&15,mod=op&15;
			for (unsigned lane=0;lane<32;lane++) {
				if (code==0x71) {
					unsigned reg=(op>>20)&15,mode=(op>>16)&15; assert(reg<8); auto& v=lr[reg][lane];
					if (!mode) v=(op&65535)<<16; else if (mode==10) v=(v&0xffff0000)|(op&65535);
					else { assert(mode==2); v=op&65535; } continue;
				}
				if (code==0x86 || code==0x85) {
					unsigned va=(op>>16)&15,vb=(op>>12)&15;
					assert(affineEligible && vd<8 && ((code==0x86 && vc==9 && mod==2) || (code==0x85 && va==10 && !mod)));
					auto value=[&](unsigned r) { assert(r<8 || r==9 || r==10); return r==9?0U:r==10?0x3f800000U:lr[r][lane]; };
					uint32_t c=value(vc); if (mod&2) c^=0x80000000;
					lr[vd][lane]=PioneerBlackholeFmaBits(value(va),value(vb),c); continue;
				}
				assert(vc<8 && vd<8); auto& v=lr[vd][lane];
				switch (code) {
				case 0x90: { assert(affineEligible && !mod && (vc==0 || vc==4)); uint32_t sm=lr[vc][lane],mag=sm&0x7fffffff;
					assert(mag<=(vc==0?262144U:2048U) && (mag || !sm)); v=Bits(float(int(mag)*((sm>>31)?-1:1))); break; }
				case 0x7c: assert(!mod); v=lr[vc][lane]; break;
				case 0x7e: { unsigned vb=(op>>12)&15; assert(mod==1 && vb<8); v=lr[vb][lane]&lr[vc][lane]; break; }
				case 0x8d: assert(!mod); v^=lr[vc][lane]; break;
				case 0x79: assert(mod==4 || mod==5); v=lr[vc][lane]+(mod==5?uint32_t(Sign(op>>12,12)):v); break;
				case 0x7a: { assert(mod==1 || mod==3); int shift=Sign(op>>12,12);
					if (shift>=0) v<<=unsigned(shift)&31;
					else if (mod==3) v=uint32_t(int32_t(v)>>(unsigned(-shift)&31)); else v>>=unsigned(-shift)&31; break; }
				default: fprintf(stderr,"UNMODELED VECTOR WORD %08x PC%08x\n",op,pc); assert(false);
				}
			}
			drained=false; return;
		}
		if (code==0x26) assert(panelMoves==4 && sources[mathBank] && !rawPending);
		Base::Instruction(op);
		if (op==0x26c00020) { sources[mathBank]=false; mathBank^=1; affineEligible=true; assert(unpackBank==mathBank); if (corruptDotRange) dstInt[32][0]=262145; }
	}
	void Store(uint32_t address,uint32_t value) override {
		if (Range(address,0xffef0000,224*4)) {
			assert(!rawPending && drained); unsigned index=(address-0xffef0000)/4;
			if (!index) { assert(setupConfiguration==25); setupConfiguration=0; }
			if (setupConfiguration<25) {
				// Independent exact trace,not read from the candidate table.
				const unsigned indices[]={0,1,2,3,6,7,220,20,24,28,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
				const uint32_t values[]={0x4231,0xa0220000,0,0x10000,0,0,0,0,0xffff,0x100,0x100,0x100000,0x100,
					0x100011,16,1,3,0x7070,0x400040,0x100010,0x100011,8,1,3,0x7091};
				assert(index==indices[setupConfiguration] && value==values[setupConfiguration]); setupConfiguration++;
			}
			config[index]=value; configWrites++; return;
		}
		assert(!Range(address,0x70710,512)); // NO decoded operand staging/copy
		Base::Store(address,value);
	}
	uint32_t Load(uint32_t address) override {
		if (Range(address,0xffef0000,224*4)) { assert(!rawPending && drained); return config[(address-0xffef0000)/4]; }
		if (Range(address,0xffbd9b00,64)) { assert(affineEligible && !config[3] && !rawPending && drained); outputReads++; return FloatDst(108,(address-0xffbd9b00)/4); }
		return Base::Load(address);
	}
	bool RunVector() {
		while (pc!=0x72038 && pc!=0x72058) StepBlock();
		if (pc==0x72058) { assert(l1[Index(0x17ffc8)]==0x46563851); return false; }
		assert(l1[Index(0x17ffc8)]==0x31563851 && minStack>=0x6e000 && !rawPending
			&& !sources[0] && !sources[1] && unpackBank==mathBank && drained);
		return l1[Index(0x17ffc4)]==groups*(inputs/16) && requests==tiles.size() && requests==responses && !pending;
	}
};
using VectorRowsMachine=VectorRows<RowsMachine>;
#ifndef PIONEER_Q38_VECTOR_ROWS_MODEL_ONLY
int main(int argc,char** argv)
{
	assert(argc>=2 && argc<=4);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		VectorRowsMachine m(argv[1],type,k,k==17408?3:1); m.delay=3;
		assert(m.RunVector() && m.CorrectRows());
		printf("OFFLINE VECTOR actual type%u K%u N%u steps%u rawBytes%u SFPUops%u exact FP32+double PASS\n",type,k,m.groups*16,m.steps,m.rawReads,m.vectorOps);
		if (argc==4) {
			RowsMachine scalar(argv[3],type,k,m.groups); scalar.delay=3;
			assert(scalar.RunRows() && scalar.CorrectRows());
			for (unsigned row=0;row<m.groups*16;row++) assert(m.buffers[4096+row]==scalar.buffers[4096+row]);
			printf("OFFLINE SAME synthetic workload scalar_RV_steps%u vector_RV_steps%u ratio%.6f;INSTRUCTION_COUNTS_NOT_NATIVE_SPEED\n",scalar.steps,m.steps,double(scalar.steps)/m.steps);
		}
	}
	for (unsigned type:{8U,12U,14U}) { VectorRowsMachine m(argv[1],type,5120,1); m.corruptMove=true; assert(m.RunVector() && !m.CorrectRows()); }
	for (unsigned type:{8U,12U,14U}) { VectorRowsMachine m(argv[1],type,5120,1); m.corruptDotRange=true; assert(!m.RunVector()); }
	for (unsigned type:{8U,12U,14U}) { VectorRowsMachine m(argv[1],type,5120,1); m.negativeDotZero=true; assert(m.RunVector() && m.CorrectRows()); }
	for (unsigned fault:{1U,20U,60U}) {
		VectorRowsMachine m(argv[1],12,5120,3); m.faultAt=fault;
		assert(!m.RunVector() && m.pending && m.requests==fault && m.responses==fault-1);
		unsigned requests=m.requests; for (unsigned i=0;i<1000;i++) m.StepBlock(); assert(m.requests==requests && m.pending);
	}
	if (argc>=3) {
		FILE* f=fopen(argv[2],"rb"); assert(f); PioneerQwen38Gguf model; assert(model.Open(f));
		for (const char* name:{"blk.0.attn_qkv.weight","blk.0.ffn_down.weight","output.weight"}) {
			const auto* t=model.Find(name); assert(t);
			for (unsigned end=0;end<2;end++) {
				VectorRowsMachine m(argv[1],t->type,unsigned(t->inputs),3);
				m.BindRows(model,unsigned(t-model.Tensors().data()),end?unsigned(t->outputs)-48:0);
				assert(m.RunVector() && m.CorrectRows());
				printf("OFFLINE VECTOR REAL %s K%u firstRow%u exact FP32+double PASS\n",name,m.inputs,end?unsigned(t->outputs)-48:0);
			}
		}
		assert(model.Stable()); fclose(f);
	}
	puts("PASS OFFLINE ACTUAL vector-row image:exact all-K Q8/Q4/Q6 math,raw Dst/vector/Matrix source-bank ownership,zero scalar operand writes,all4 fidelity phases,source bounds,UNKNOWN inert park;NOT native/persistent/coordinator/cycle/speed qualification");
}
#endif
