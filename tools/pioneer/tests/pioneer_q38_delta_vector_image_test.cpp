/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_DELTA_VECTOR_MODEL_ONLY
#define PIONEER_Q38_DELTA_VECTOR_EXISTING_BITS
#include "pioneer_q38_delta_vector_test.cpp"
struct DeltaVectorImage:ScalarMachine {
	DeltaVectorMachine unit;
	uint32_t cfg[224]={},threads[68]={},ack=0,x0=0,x1=0;
	unsigned imports=0,pendingWords=0,pendingSource=0,pendingRow=0,nextWrites=0,resultWrites=0,csrReads=0;
	unsigned failImport=0,configWrites=0;
	bool wait=false,dropMathDrain=false,dropOutputDrain=false;
	struct UnsafeRead {};
	DeltaVectorImage(const char* path,unsigned trial):ScalarMachine(path,64,0) {
		unit.localImage=true; memory.clear();
		memory[0x74000]=0x56383351; memory[0x74004]=128; memory[0x74008]=64; memory[0x7400c]=0;
		for (unsigned i=0;i<8192;i++) { memory[0x90000+i*4]=Bits(trial?float(sin(i*.031+trial*.17)*.003):0); memory[0xa0000+i*4]=0xdeadbeef; }
		for (unsigned i=0;i<128;i++) {
			memory[0xb0000+i*4]=Bits(trial?float(sin(i*.123+trial*.317)*.01):0);
			memory[0xb0200+i*4]=Bits(trial?float(cos(i*.153+trial*.217)*.03):0);
		}
		for (unsigned i=0;i<64;i++) { memory[0xb0400+i*4]=Bits(trial?float(sin(i*.013+trial*.117)*2.1):0); memory[0xb0800+i*4]=0xdeadbeef; }
		memory[0xb0500]=Bits(trial==2?0:trial==3?1:.83f); memory[0xb0504]=Bits(trial==2?1:trial==3?0:.37f);
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
	}
	void Instruction(uint32_t op) override {
		unsigned code=op>>24;
		if (code==0xb2) { assert(!pendingWords); unsigned i=(op>>16)&255; assert(i<68); threads[i]=op&65535; return; }
		if (op==0xa2101816) { assert(!wait && !ack); wait=true; return; }
		if (op==0x4500017e) {
			assert(wait); wait=false;
			if (pendingWords) {
				if (imports!=failImport) {
					for (unsigned i=0;i<pendingWords;i++) unit.state[pendingRow*16+i]=memory.at(pendingSource+i*4);
					for (unsigned r=pendingRow;r<pendingRow+pendingWords/16;r++) unit.defined[r]=true;
				}
				pendingWords=0;
			}
			bool dropped=dropMathDrain && imports==9 && unit.stores==8192 && !nextWrites;
			dropped |= dropOutputDrain && unit.stores==8192+64;
			if (!dropped) unit.Drain(); ack=1; return;
		}
		if ((op&0xfff00000)==0x5e200000) { assert(!pendingWords); x0=op&1023; x1=(op>>10)&1023; return; }
		if (op==0x5120000a || op==0x5420000f) { assert(!pendingWords); return; }
		if (op==0x42000081) {
			assert(!pendingWords && !threads[5] && cfg[0]==0x4231 && cfg[1]==0xa0200000 && !cfg[3] && !cfg[49]
				&& cfg[64]==0x100010 && cfg[65]==16 && !cfg[72] && cfg[73]==0x13 && !x0);
			unsigned source=(cfg[76]+1)*16,row=(cfg[84]&65535)/16-4,words=x1+1;
			assert((cfg[84]&0xffff0000)==0x400000 && row<512 && !((words)%16) && words<=8192-row*16);
			assert((!imports && source==0xb0400 && !row && words==64)
				|| (imports && imports<9 && source==0x90000+(imports-1)*4096 && row==(imports-1)*64 && words==1024));
			imports++; pendingSource=source; pendingRow=row; pendingWords=words; unit.drained=false; return;
		}
		assert(!pendingWords && threads[2]==1 && threads[3]==1 && threads[5]==4 && cfg[0]==0x4231 && cfg[1]==0xa0200000);
		if (code==0x70 && ((op>>20)&15)) assert(imports==1 && (((op>>20)&15)==6 || ((op>>20)&15)==7) && !(op&0x3fc));
		if (code==0x72 && ((op>>20)&15)) assert(nextWrites==8192 && (((op>>20)&15)==4 || ((op>>20)&15)==5) && !(op&0x3fc));
		unit.Op(op);
	}
	uint32_t Load(uint32_t at) override {
		assert(!(at&3));
		if (Range(at,0x64000,unsigned(code.size()))) return l1[Index(at)];
		if (at==0xffe000fc) return ack;
		if (Range(at,0xffef0000,224*4)) return cfg[(at-0xffef0000)/4];
		if (Range(at,0xffbd8000,32768)) {
			if (!unit.drained || pendingWords) throw UnsafeRead{};
			assert(!cfg[3] && imports==9); csrReads++; return unit.state[(at-0xffbd8000)/4];
		}
		assert(Range(at,0x74000,16) || Range(at,0x6e000,8192) || Range(at,0x90000,32768)
			|| Range(at,0xb0000,1288) || Range(at,0x17ffc0,12));
		return memory.at(at);
	}
	void Store(uint32_t at,uint32_t value) override {
		assert(!(at&3));
		if (at==0xffe000fc) { assert(!value); ack=0; return; }
		if (at==0xffe40000) { Instruction(value); return; }
		if (Range(at,0xffef0000,224*4)) {
			assert(unit.drained && !pendingWords); unsigned i=(at-0xffef0000)/4;
			const unsigned allowed[]={0,1,2,3,6,7,220,20,24,28,49,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
			bool found=false; for (auto a:allowed) found|=i==a; assert(found); cfg[i]=value; configWrites++; return;
		}
		assert(Range(at,0x6e000,8192) || Range(at,0xa0000,32768) || Range(at,0xb0800,256) || Range(at,0x17ffc0,12));
		if (Range(at,0x6e000,8192)) minStack=std::min(minStack,unsigned(at));
		if (Range(at,0xa0000,32768)) { assert(unit.drained && imports==9 && csrReads==nextWrites+1); nextWrites++; }
		if (Range(at,0xb0800,256)) { assert(unit.drained && nextWrites==8192); resultWrites++; }
		memory[at]=value;
	}
	bool RunVector() {
		while (pc!=0x64038 && pc!=0x64058) StepScalar();
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==0x46564451); return false; }
		assert(memory.at(0x17ffc8)==0x31564451); return memory.at(0x17ffc4)==64;
	}
	void Verify() {
		float next[8192],delta[64],out[64]={};
		float decay=Float(memory.at(0xb0500)),beta=Float(memory.at(0xb0504));
		for (unsigned j=0;j<64;j++) {
			float predicted=0; for (unsigned i=0;i<128;i++) predicted+=(Float(memory.at(0x90000+(i*64+j)*4))*decay)*Float(memory.at(0xb0200+i*4));
			delta[j]=(Float(memory.at(0xb0400+j*4))-predicted)*beta;
		}
		for (unsigned i=0;i<128;i++) for (unsigned j=0;j<64;j++) {
			next[i*64+j]=Float(memory.at(0x90000+(i*64+j)*4))*decay+Float(memory.at(0xb0200+i*4))*delta[j];
			assert(memory.at(0xa0000+(i*64+j)*4)==Bits(next[i*64+j]));
		}
		for (unsigned j=0;j<64;j++) {
			for (unsigned i=0;i<128;i++) out[j]+=next[i*64+j]*Float(memory.at(0xb0000+i*4));
			assert(memory.at(0xb0800+j*4)==Bits(out[j]));
		}
		assert(nextWrites==8192 && resultWrites==64 && imports==9 && csrReads==8192+64 && unit.drained);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned trial=0;trial<4;trial++) {
		DeltaVectorImage m(argv[1],trial); auto before=m.memory; assert(m.RunVector()); m.Verify();
		for (const auto& word:before) if (DeltaVectorImage::Range(word.first,0x90000,32768) || DeltaVectorImage::Range(word.first,0xb0000,1288)) assert(m.memory.at(word.first)==word.second);
		printf("PASS ACTUAL local vector GDN trial%u steps%u SFPUops%u ALL state/output bits exact;NO native speed claim\n",trial,m.steps,m.unit.ops);
	}
	for (unsigned field=0;field<4;field++) for (unsigned bit=0;bit<32;bit++) {
		DeltaVectorImage m(argv[1],1); m.memory[0x74000+field*4]^=1U<<bit;
		assert(!m.RunVector() && !m.configWrites && !m.unit.ops && !m.nextWrites && !m.resultWrites);
	}
	for (uint32_t at:{0x90000U,0x97ffcU,0xb0000U,0xb01fcU,0xb0200U,0xb03fcU,0xb0400U,0xb04fcU,0xb0500U,0xb0504U}) {
		for (uint32_t bad:{1U,0x80000001U,0x7f800000U,0x7fc00001U}) {
			DeltaVectorImage m(argv[1],1); m.memory[at]=bad;
			assert(!m.RunVector() && !m.configWrites && !m.unit.ops && !m.nextWrites && !m.resultWrites);
		}
	}
	for (unsigned import:{1U,5U,9U}) {
		DeltaVectorImage m(argv[1],1); m.failImport=import; bool caught=false;
		try { (void)m.RunVector(); } catch (const DeltaVectorMachine::OwnershipFault&) { caught=true; }
		assert(caught && !m.nextWrites && !m.resultWrites);
	}
	for (bool output:{false,true}) {
		DeltaVectorImage m(argv[1],1); m.dropMathDrain=!output; m.dropOutputDrain=output; bool caught=false;
		try { (void)m.RunVector(); } catch (const DeltaVectorImage::UnsafeRead&) { caught=true; }
		assert(caught && !m.resultWrites && m.nextWrites==(output?8192U:0U));
	}
	puts("PASS OFFLINE ACTUAL emitted vector GDN:fixed L1/CSR whitelist,eight10-bit-counter state imports,ALL512 rows defined,ALL descriptor bits+nonfinite inputs inert,first/middle/last missing imports and false final drains reject without publication;NO NoC/DRAM/DMA/PCI/native image selection/GGML residency");
}
