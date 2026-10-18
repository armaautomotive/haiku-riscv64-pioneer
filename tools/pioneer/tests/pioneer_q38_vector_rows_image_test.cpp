/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_HAS_ROWS_MACHINE
#define PIONEER_Q38_SHARED_ROWS_MACHINE_ONLY
#include "pioneer_qwen38_rows_image_test.cpp"
#endif
#include <memory>
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
	uint16_t physical[1024][16]={}; uint32_t lr[8][32]={};
	bool wrote[1024][16]={},sources[2]={},rawPending=false,publish=false,enabled=false;
	unsigned unpackBank=0,mathBank=0,rawAddress=0,rawRow=0,adc0=0,adc1=255;
	unsigned rawReads=0,vectorOps=0,panelMoves=0,rawUnpacks=0;
	bool corruptMove=false;
	VectorRows(const char* path,unsigned type,unsigned k,unsigned groups):Base(path,type,k,groups) { if (!Persistent) memory[0x74000]=0x31563851; }
	VectorRows(const char* path,unsigned type,unsigned k,unsigned groups,unsigned worker):Base(path,type,k,groups,worker) { static_assert(Persistent,"worker routing ONLY in the persistent model"); }
	bool InlineTensix() const override { return true; }
	static unsigned Adj(unsigned row) { return ((row&0x1f8)<<1)|(row&0x207); }
	void Instruction(uint32_t op) override {
		unsigned code=op>>24;
		if (code==0x42 && !(op&0x800000)) {
			assert(!rawPending && !pending && !sources[unpackBank] && !ownerA
				&& config[1]==0xa03c8000 && !config[49] && config[64]==0x10001e && config[65]==16
				&& config[72]==14 && config[73]==0x13 && thread[5]==0 && !adc0 && adc1==255
				&& (op==0x42000081 || op==0x420000c1));
			rawAddress=(config[76]+1)*16; rawRow=((config[84]&65535)/16)-4;
			assert(rawRow==0 || rawRow==16);
			assert(rawAddress>=0x90000 && rawAddress-0x90000+256<=tile.size());
			if (!rawRow) { memset(physical,0,sizeof(physical)); memset(wrote,0,sizeof(wrote)); panelMoves=0; }
			rawPending=true; publish=op&64; rawUnpacks++; drained=false; return;
		}
		if (op==0x5e23fc00) { adc0=adcX0[0]=0; adc1=adcX1[0]=255; return; }
		if (op==0x5120000a || op==0x5420000f) { assert(!rawPending); return; }
		if (op==0x4500017e && rawPending) {
			assert(backendWait && !pending);
			for (unsigned i=0;i<256;i++) {
				unsigned address=rawAddress+i; uint32_t word=memory.at(address&~3U);
				unsigned byte=(word>>(8*(address&3)))&255;
				physical[rawRow+i/16][i%16]=uint16_t((byte<<5)|(byte?16:0));
				wrote[rawRow+i/16][i%16]=true; rawReads++;
			}
			if (publish) { sources[unpackBank]=true; unpackBank^=1; ownerA=sources[mathBank]; }
			rawPending=false;
		}
		if ((code>=0x70 && code<=0x8d) || code==8) {
			assert(!rawPending && !pending && ownerA && sources[mathBank] && thread[2]==1
				&& config[1]==0xa0220000 && config[220]==0 && thread[5]==4);
			vectorOps++;
			if (op==0x8a000002) { enabled=true; return; }
			assert(enabled);
			if (code==0x70 || code==0x72) {
				unsigned mode=(op>>16)&15,reg=(op>>20)&15,addr=op&1023;
				assert(reg<2 && !(op&0xfc00) && !(addr&1));
				assert(code==0x70?(mode==5 && addr<32):(mode==7 && !reg && addr>=64 && addr<80));
				for (unsigned lane=0;lane<32;lane++) {
					unsigned row=(addr&~3U)+lane/8,col=(lane&7)*2+((addr&2)!=0);
					if (code==0x70) { assert(wrote[row][col]); uint16_t v=physical[row][col]; lr[reg][lane]=(uint32_t(v>>15)<<31)|((v>>5)&255); }
					else { unsigned hi=Adj(row),lo=hi+8; assert(hi>=128 && hi<160 && !wrote[hi][col] && !wrote[lo][col]);
						physical[hi][col]=uint16_t(lr[0][lane]>>16); physical[lo][col]=uint16_t(lr[0][lane]); wrote[hi][col]=wrote[lo][col]=true; }
				}
				drained=false; return;
			}
			if (code==8) {
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
				assert(vc<8 && vd<8); auto& v=lr[vd][lane];
				switch (code) {
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
		if (op==0x26c00020) { sources[mathBank]=false; mathBank^=1; assert(unpackBank==mathBank); }
	}
	void Store(uint32_t address,uint32_t value) override {
		if (Range(address,0xffef0000,224*4)) { assert(!rawPending && drained); config[(address-0xffef0000)/4]=value; configWrites++; return; }
		assert(!Range(address,0x70710,512)); // NO decoded operand staging/copy
		Base::Store(address,value);
	}
	uint32_t Load(uint32_t address) override {
		if (Range(address,0xffef0000,224*4)) { assert(!rawPending && drained); return config[(address-0xffef0000)/4]; }
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
