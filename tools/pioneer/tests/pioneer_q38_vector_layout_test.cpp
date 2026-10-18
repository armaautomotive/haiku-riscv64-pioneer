/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../PioneerQ38VectorPanel.h"
#include "../PioneerQwen38Panel.h"
struct VectorLayout:NativeF32Q8Machine {
	uint16_t physical[1024][16]={}; bool written[1024][16]={}; unsigned words=0;
	VectorLayout():NativeF32Q8Machine(1024,32,0) {}
	static unsigned Adj32(unsigned row) { return ((row&0x1f8)<<1)|(row&0x207); } // config220=0
	void Raw(unsigned row,unsigned col,unsigned byte) {
		assert(row<32 && col<16 && byte<256); physical[row][col]=uint16_t((byte<<5)|(byte?16:0)); written[row][col]=true;
	}
	void LoadLanes(unsigned reg,unsigned address) {
		assert(reg<2 && address<32 && !(address&1));
		for (unsigned lane=0;lane<32;lane++) {
			unsigned row=(address&~3U)+lane/8,col=(lane%8)*2+((address&2)!=0);
			assert(written[row][col] && !(physical[row][col]&0x8000));
			lanes[reg][lane]=(physical[row][col]>>5)&255; // SFPLOAD INT8
		}
	}
	void Op(uint32_t word) { Instruction(word); words++; }
	void StoreLanes(unsigned address) {
		assert(address>=64 && address<80 && !(address&1));
		for (unsigned lane=0;lane<32;lane++) {
			unsigned row=(address&~3U)+lane/8,col=(lane%8)*2+((address&2)!=0),hi=Adj32(row),lo=hi+8;
			assert(hi>=128 && hi<160 && lo>=128 && lo<160 && !written[hi][col] && !written[lo][col]);
			physical[hi][col]=uint16_t(lanes[0][lane]>>16); physical[lo][col]=uint16_t(lanes[0][lane]);
			written[hi][col]=written[lo][col]=true; // SFPSTORE HI16 opaque32
		}
	}
	uint16_t Move(unsigned row,unsigned col) const {
		unsigned hi=Adj32(64+row),lo=hi+8;
		assert(written[hi][col] && written[lo][col] && !physical[lo][col]);
		uint16_t high=physical[hi][col]; uint32_t src=((uint32_t(high)&0xffe0)<<3)|(high&31);
		return uint16_t(((src&0x40000)>>3)|((src&31)<<10)|((src>>8)&1023));
	}
};
int main()
{
	(void)Read; (void)Word;
	unsigned panels=0,weights=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned trial=0;trial<256;trial++) {
		std::vector<uint8_t> tile(PioneerQ38PanelTileBytes(type));
		uint32_t seed=0x25b19e73^(trial*0x5a7149d3)^type;
		for (unsigned i=0;i<tile.size();i++) { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; tile[i]=uint8_t(seed); }
		for (unsigned row=0;row<16;row++) { // valid scales;raw quant bytes stay unconstrained
			unsigned at=(type==14?208:0)*16+row; tile[at]=0; tile[at+16]=0x3c;
			if (type==12) { tile[32+row]=0; tile[48+row]=0x38; }
		}
		for (unsigned panel=0;panel<(type==8?2U:16U);panel++) {
			PioneerQ38Panel expected; assert(PioneerQ38UnpackPanel(type,tile.data(),unsigned(tile.size()),panel,&expected));
			PioneerQ38VectorPanelSpan span; assert(PioneerQ38VectorPanel(type,panel,span));
			auto m=std::make_unique<VectorLayout>();
			for (unsigned k=0;k<16;k++) for (unsigned row=0;row<16;row++) {
				m->Raw(k,row,tile[span.low+k*16+row]);
				if (span.second) m->Raw(16+k,row,tile[span.high+k*16+row]);
			}
			for (unsigned block=0;block<16;block+=4) for (unsigned parity:{0U,2U}) {
				m->LoadLanes(0,block+parity); if (span.second) m->LoadLanes(1,16+block+parity);
				assert(PioneerQ38VectorOperand(*m,type,panel)); m->StoreLanes(64+block+parity);
			}
			for (unsigned k=0;k<16;k++) for (unsigned row=0;row<16;row++) {
				unsigned e=k*16+row; assert(m->Move(k,row)==uint16_t(expected.operand[e/2]>>((e%2)*16))); weights++;
			}
			for (unsigned row=32;row<1024;row++) if (row<128 || row>=160) for (unsigned col=0;col<16;col++) assert(!m->written[row][col]);
			panels++;
		}
	}
	for (unsigned type:{0U,8U,12U,14U,99U}) {
		PioneerQ38VectorPanelSpan s={7,9,true}; assert(!PioneerQ38VectorPanel(type,type==8?2:16,s) && s.low==7 && s.high==9 && s.second);
	}
	printf("PASS OFFLINE vector layout:%u panels/%u exact weights against original scalar decoder,raw Dst16 rows0..31 and opaque decoded Dst32 rows64..79 physically disjoint,ALL32 even/odd lane placement/HI16/MOVD2A overlays/bounds/canaries;NO actual unpack/ownership/scheduling/image/native/speed qualification\n",panels,weights);
}
