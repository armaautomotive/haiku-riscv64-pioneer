/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_DELTA_VECTOR_MODEL_ONLY
#define PIONEER_Q38_DELTA_VECTOR_EXISTING_BITS
#include "pioneer_q38_delta_vector_test.cpp"
#include "../PioneerQ38DeltaVectorLayout.h"
namespace L=PioneerQ38DeltaVectorLayout;
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
static uint32_t TestStatePage(unsigned page){return 0x120000+page*4096;}
#endif
struct DeltaVectorImage:ScalarMachine {
	DeltaVectorMachine unit;
	uint32_t cfg[224]={},threads[68]={},ack=0,x0=0,x1=0;
	unsigned imports=0,pendingWords=0,pendingSource=0,pendingRow=0,nextWrites=0,scatterWrites=0,
		statePrefillWrites=0,resultWrites=0,csrReads=0;
	std::vector<uint32_t> initialState;
	unsigned failImport=0,configWrites=0;
	bool wait=false,dropMathDrain=false,dropOutputDrain=false;
	#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	bool IsTestStatePage(uint32_t at)const{
		for(unsigned page=0;page<L::StatePages;page++){
			const uint32_t base=memory.at(uint32_t(L::StatePageTable)+page*4);
			if(at>=base && at-base<L::StatePageBytes)return true;
		}
		return false;
	}
	#endif
	struct UnsafeRead {};
	DeltaVectorImage(const char* path,unsigned trial):ScalarMachine(path,64,0) {
		unit.localImage=true; memory.clear();
		memory[0x74000]=0x56383351; memory[0x74004]=128; memory[0x74008]=64; memory[0x7400c]=0;
		initialState.reserve(8192);
		for (unsigned i=0;i<8192;i++) {
			uint32_t value=Bits(trial?float(sin(i*.031+trial*.17)*.003):0);initialState.push_back(value);
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			unsigned page=i/1024,word=i%1024;uint32_t address=TestStatePage(page);
			memory[uint32_t(L::StatePageTable)+page*4]=address;memory[address+word*4]=value;
#else
			memory[0x90000+i*4]=value;
#endif
			memory[0xa0000+i*4]=0xdeadbeef;
		}
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		memory[uint32_t(L::StatePageMode)]=0;
#endif
		for (unsigned i=0;i<128;i++) {
			memory[uint32_t(L::Query)+i*4]=Bits(trial?float(sin(i*.123+trial*.317)*.01):0);
			memory[uint32_t(L::Key)+i*4]=Bits(trial?float(cos(i*.153+trial*.217)*.03):0);
		}
		for (unsigned i=0;i<64;i++) { memory[uint32_t(L::Value)+i*4]=Bits(trial?float(sin(i*.013+trial*.117)*2.1):0); memory[uint32_t(L::Output)+i*4]=0xdeadbeef; }
		memory[uint32_t(L::Decay)]=Bits(trial==2?0:trial==3?1:.83f); memory[uint32_t(L::Beta)]=Bits(trial==2?1:trial==3?0:.37f);
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
	}
	unsigned Index(uint32_t at) override {
		if (at==0x6000) return 0;
		assert(Range(at,L::Code,unsigned(code.size())) && !(at&3)); return 1+(at-L::Code)/4;
	}
	bool CodeAddress(uint32_t at) const override { return at==0x6000 || Range(at,L::Code,unsigned(code.size())); }
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
			assert((!imports && source==uint32_t(L::Value) && !row && words==64)
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
				|| (imports && imports<9 && source==(memory.at(uint32_t(L::StatePageMode))==2
					?uint32_t(L::Prior)+(imports-1)*4096:memory.at(uint32_t(L::StatePageTable)+(imports-1)*4))
					&& row==(imports-1)*64 && words==1024));
#else
				|| (imports && imports<9 && source==0x90000+(imports-1)*4096 && row==(imports-1)*64 && words==1024));
#endif
			imports++; pendingSource=source; pendingRow=row; pendingWords=words; unit.drained=false; return;
		}
		assert(!pendingWords && threads[2]==1 && threads[3]==1 && threads[5]==4 && cfg[0]==0x4231 && cfg[1]==0xa0200000);
		if (code==0x70 && ((op>>20)&15)) assert(imports==1 && (((op>>20)&15)==6 || ((op>>20)&15)==7) && !(op&0x3fc));
		if (code==0x72 && ((op>>20)&15)) assert(
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			(memory.at(uint32_t(L::StatePageMode))==2?nextWrites==8192:scatterWrites==8192)
#else
			nextWrites==8192
#endif
			&& (((op>>20)&15)==4 || ((op>>20)&15)==5) && !(op&0x3fc));
		unit.Op(op);
	}
	uint32_t Load(uint32_t at) override {
		assert(!(at&3));
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		if (Range(at,uint32_t(L::StatePageTable),L::StatePages*4+4)) return memory.at(at);
#endif
		if (Range(at,uint32_t(L::Code),unsigned(code.size()))) return l1[Index(at)];
		if (at==0xffe000fc) return ack;
		if (Range(at,0xffef0000,224*4)) return cfg[(at-0xffef0000)/4];
		if (Range(at,0xffbd8000,32768)) {
			if (!unit.drained || pendingWords) throw UnsafeRead{};
			assert(!cfg[3] && imports==9); csrReads++; return unit.state[(at-0xffbd8000)/4];
		}
		assert(Range(at,0x74000,16) || Range(at,0x6e000,8192) || Range(at,0x90000,32768)
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			|| Range(at,uint32_t(L::StatePageTable),32) || Range(at,0x90000,L::L1Bytes-0x90000)
#endif
			|| Range(at,uint32_t(L::Query),1288) || Range(at,0x17ffc0,12));
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
		assert(Range(at,0x6e000,8192) || Range(at,0xa0000,32768) || Range(at,uint32_t(L::Output),256) || Range(at,0x17ffc0,12)
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			|| Range(at,uint32_t(L::StatePageTable),L::StatePages*4+4)
			|| Range(at,0x90000,L::L1Bytes-0x90000)
#endif
		);
		if (Range(at,0x6e000,8192)) minStack=std::min(minStack,unsigned(at));
		if (Range(at,0xa0000,32768)) { assert(unit.drained && imports==9 &&
#if defined(PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT) && defined(PIONEER_Q38_GDN_SCATTERED_RESIDENT_IMAGE_TEST)
			(memory.at(uint32_t(L::StatePageMode))==2?csrReads==nextWrites+1:scatterWrites==8192)
#else
			csrReads==nextWrites+1
#endif
			); nextWrites++; }
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		if (IsTestStatePage(at)) {
			if(imports<9){assert(!memory.at(uint32_t(L::StatePageMode)) && !imports && unit.drained);statePrefillWrites++;}
			else {assert(unit.drained && imports==9 && csrReads==scatterWrites+1);scatterWrites++;}
		}
#endif
		if (Range(at,uint32_t(L::Output),256)) { assert(unit.drained &&
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			(memory.at(uint32_t(L::StatePageMode))==2?nextWrites==8192:scatterWrites==8192)
#else
			nextWrites==8192
#endif
		); resultWrites++; }
		memory[at]=value;
	}
	bool RunVector() {
		while (pc!=uint32_t(L::Park) && pc!=uint32_t(L::FaultPark)) StepScalar();
		if (pc==uint32_t(L::FaultPark)) { assert(memory.at(0x17ffc8)==0x46564451); return false; }
		assert(memory.at(0x17ffc8)==0x31564451); return memory.at(0x17ffc4)==64;
	}
	void Verify() {
		float next[8192],delta[64],out[64]={};
		float decay=Float(memory.at(uint32_t(L::Decay))),beta=Float(memory.at(uint32_t(L::Beta)));
		for (unsigned j=0;j<64;j++) {
			float predicted=0; for (unsigned i=0;i<128;i++) predicted+=(Float(initialState[i*64+j])*decay)*Float(memory.at(uint32_t(L::Key)+i*4));
			delta[j]=(Float(memory.at(uint32_t(L::Value)+j*4))-predicted)*beta;
		}
		for (unsigned i=0;i<128;i++) for (unsigned j=0;j<64;j++) {
			next[i*64+j]=Float(initialState[i*64+j])*decay+Float(memory.at(uint32_t(L::Key)+i*4))*delta[j];
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			uint32_t actual;
			if(memory.at(uint32_t(L::StatePageMode))==2)actual=memory.at(uint32_t(L::Next)+(i*64+j)*4);
			else {uint32_t page=memory.at(uint32_t(L::StatePageTable)+(i*64+j)/1024*4);
				actual=memory.at(page+((i*64+j)%1024)*4);}
			if (actual!=Bits(next[i*64+j])) {
				fprintf(stderr,"state mismatch i=%u j=%u got=%08x expected=%08x initial=%08x\n",
					 i,j,actual,Bits(next[i*64+j]),Bits(initialState[i*64+j]));
				assert(false);
			}
#else
			if(memory.at(0xa0000+(i*64+j)*4)!=Bits(next[i*64+j])){
				fprintf(stderr,"contiguous mismatch i=%u j=%u got=%08x expected=%08x initial=%08x key=%08x value=%08x decay=%08x beta=%08x\n",
					i,j,memory.at(0xa0000+(i*64+j)*4),Bits(next[i*64+j]),initialState[i*64+j],
					memory.at(uint32_t(L::Key)+i*4),memory.at(uint32_t(L::Value)+j*4),memory.at(uint32_t(L::Decay)),memory.at(uint32_t(L::Beta)));
				assert(false);
			}
#endif
		}
		for (unsigned j=0;j<64;j++) {
			for (unsigned i=0;i<128;i++) out[j]+=next[i*64+j]*Float(memory.at(uint32_t(L::Query)+i*4));
#ifdef PIONEER_Q38_DELTA_VECTOR_POST_SCALE
			out[j]*=1.f/sqrtf(128.f);
#endif
			assert(memory.at(uint32_t(L::Output)+j*4)==Bits(out[j]));
		}
		assert(resultWrites==64 && imports==9 && csrReads==8192+64 && unit.drained);
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
#ifdef PIONEER_Q38_GDN_SCATTERED_RESIDENT_IMAGE_TEST
		assert((memory.at(uint32_t(L::StatePageMode))==2
			?nextWrites==8192 && !scatterWrites
			:nextWrites==8192 && scatterWrites==8192)
			&& statePrefillWrites==(memory.at(uint32_t(L::StatePageMode))?0:8192));
#else
		assert(memory.at(uint32_t(L::StatePageMode))==2
			?nextWrites==8192 && !scatterWrites
			:!nextWrites && scatterWrites==8192);
#endif
#else
		assert(nextWrites==8192 && !scatterWrites);
#endif
	}
};
#ifndef PIONEER_Q38_DELTA_VECTOR_IMAGE_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);
#ifdef PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
	for (uint32_t at:{0x72038U,0x7203cU}) {
		DeltaVectorImage m(argv[1],1); auto before=m.memory; m.pc=at; m.StepScalar();
		assert(m.pc==0x72038 && m.memory==before); // actual BOTH branches,not a mocked whitelist
	}
#endif
	for (unsigned trial=0;trial<4;trial++) {
		DeltaVectorImage m(argv[1],trial); auto before=m.memory; assert(m.RunVector()); m.Verify();
		for (const auto& word:before) if (
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			DeltaVectorImage::Range(word.first,uint32_t(L::Query),1288)
#else
			DeltaVectorImage::Range(word.first,0x90000,32768) || DeltaVectorImage::Range(word.first,uint32_t(L::Query),1288)
#endif
		) assert(m.memory.at(word.first)==word.second);
		printf("PASS ACTUAL local vector GDN trial%u steps%u SFPUops%u ALL state/output bits exact;NO native speed claim\n",trial,m.steps,m.unit.ops);
	}
#ifdef PIONEER_Q38_DELTA_VECTOR_POST_SCALE
	for (unsigned head=0;head<4;head++) {
		std::array<uint32_t,8192> prior={};
		for (unsigned token=0;token<16;token++) {
			DeltaVectorImage m(argv[1],1+(token+head)%3);
			for (unsigned i=0;i<prior.size();i++) m.memory[0x90000+i*4]=prior[i];
			assert(m.RunVector()); m.Verify();
			for (unsigned i=0;i<prior.size();i++) prior[i]=m.memory.at(0xa0000+i*4);
		}
	}
	puts("PASS ACTUAL post-dot-scale vector:4 independent half-heads x16 successive state transitions,ALL8256 words/step exact;NOT native/full-model quality");
#endif
	for (unsigned field=0;field<4;field++) for (unsigned bit=0;bit<32;bit++) {
		DeltaVectorImage m(argv[1],1); m.memory[0x74000+field*4]^=1U<<bit;
		assert(!m.RunVector() && !m.configWrites && !m.unit.ops && !m.nextWrites && !m.resultWrites);
	}
	for (uint32_t at:{
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		TestStatePage(0),TestStatePage(7)+L::StatePageBytes-4,
#else
		0x90000U,0x97ffcU,
#endif
		uint32_t(L::Query),uint32_t(L::Query+0x1fc),uint32_t(L::Key),uint32_t(L::Key+0x1fc),uint32_t(L::Value),uint32_t(L::Value+0xfc),uint32_t(L::Decay),uint32_t(L::Beta)}) {
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
		assert(caught && !m.resultWrites &&
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			m.scatterWrites==(output?8192U:0U)
#else
			m.nextWrites==(output?8192U:0U)
#endif
		);
	}
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	std::array<uint32_t,8192> scatteredPrior{};
	for (unsigned token=0;token<16;token++) {
		DeltaVectorImage m(argv[1],1+token%3);
		if (token) {
			for (unsigned i=0;i<m.initialState.size();i++) {
				const uint32_t page=m.memory.at(uint32_t(L::StatePageTable)+(i/1024)*4);
				m.initialState[i]=scatteredPrior[i];
				m.memory[page+(i%1024)*4]=scatteredPrior[i];
			}
		}
		assert(m.RunVector());m.Verify();
		for (unsigned i=0;i<scatteredPrior.size();i++) {
			const uint32_t page=m.memory.at(uint32_t(L::StatePageTable)+(i/1024)*4);
			scatteredPrior[i]=m.memory.at(page+(i%1024)*4);
		}
	}
	puts("PASS scattered-state resident sequence: 16 consecutive exact recurrent updates through one stable eight-page mapping;NOT model-level quality or native inference");
	for (unsigned bad=0;bad<4;bad++) {
		DeltaVectorImage m(argv[1],1);const uint32_t at=uint32_t(L::StatePageTable)+(bad==0?0:bad==1?4:bad==2?8:12);
		m.memory[at]=bad==0?TestStatePage(1):bad==1?TestStatePage(1)+1:bad==2?0x180000:0x80000;
		assert(!m.RunVector() && !m.configWrites && !m.unit.ops && !m.scatterWrites && !m.resultWrites);
	}
	puts("PASS scattered L1 state experiment: eight noncontiguous owned-page inputs, in-place Tensix Dst update and exact page writeback; malformed/duplicate/unaligned/out-of-range/reserved pages inert; OFFLINE ONLY,NO runtime owner or token commit");
#endif
	puts("PASS OFFLINE ACTUAL emitted vector GDN:fixed L1/CSR whitelist,eight10-bit-counter state imports,ALL512 rows defined,ALL descriptor bits+nonfinite inputs inert,first/middle/last missing imports and false final drains reject without publication;NO NoC/DRAM/DMA/PCI/native image selection/GGML residency");
}
#endif
