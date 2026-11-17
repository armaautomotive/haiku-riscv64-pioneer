/* SPDX-License-Identifier: MIT */
#include <cassert>
#define PIONEER_Q38_VECTOR_Q4_METADATA_MODEL_ONLY
#include "pioneer_q38_vector_q4_metadata_test.cpp"
#include "../PioneerQ38TwoTokenQ4Affine.h"
#include "../PioneerQwen38Panel.h"

using Pair=PioneerQ38TwoTokenQ4Affine;
using Tile=std::array<uint32_t,576>;

// Strict command observer. Only original packed prefix and completed
// matrix rows enter Dst here; all affine results come from emitted commands.
struct PairMachine:MetadataMachine {
	const Tile* tile=nullptr;
	uint32_t dst[192][16]={},config[128]={};
	bool defined[192][16]={},enabled=false,drained=true;
	unsigned thread=4,imports=0,commands=0,drains=0;
	PairMachine() { config[1]=0xa03c8000; }
	void Config(unsigned i,uint32_t v) { assert(i<128 && drained && !thread);config[i]=v; }
	void Drain() { assert(okay);drains++;drained=true;cycle++; }
	void Op(uint32_t op)
	{
		commands++;drained=false;
		if (op==0xb2050000 || op==0xb2050004) { thread=op&7;cycle++;return; }
		if (op==0x3700000f || op==0x5e23fc00 || op==0x5120000a || op==0x5420000f) { cycle++;return; }
		if (op==0x42000081) {
			assert(tile && !thread && config[1]==0xa03c8000 && config[64]==0x10001e
			 && config[65]==16 && config[72]==14 && config[73]==0x13
			 && config[84]==0x00400040 && config[76]==(0x90000-16)/16);
			auto* bytes=reinterpret_cast<const uint8_t*>(tile->data());
			for (unsigned r=0;r<16;r++) for (unsigned c=0;c<16;c++) {
				dst[r][c]=bytes[r*16+c];defined[r][c]=true;
			}
			imports++;cycle++;return;
		}
		assert(thread==4);
		if (op==0x8a000002) { enabled=true;cycle++;return; }
		assert(enabled);
		const unsigned code=op>>24;
		if (code==0x70 || code==0x72) {
			const unsigned reg=(op>>20)&15,mode=(op>>16)&15,at=op&1023;
			assert(reg<8 && !(op&0xfc01));
			for (unsigned lane=0;lane<32;lane++) {
				const unsigned r=(at&~3U)+lane/8,c=(lane%8)*2+bool(at&2);
				assert(r<192);
				if (code==0x70) {
					assert(defined[r][c] && (mode==3 || (mode==4 && r>=32 && r<36) || (mode==13 && r<16)));
					lanes[reg][lane]=dst[r][c];
				} else {
					assert(mode==3 && ((r>=108 && r<120) || r>=128));
					const uint32_t x=Read(reg,lane);
					assert(!(x&0x7fffffff) || ((x&0x7f800000) && (x&0x7f800000)!=0x7f800000));
					dst[r][c]=x;defined[r][c]=true;
				}
			}
			if (code==0x70) ready[reg]=cycle+1;
			cycle++;
		} else if (op==0x8c000000) {
			uint32_t prior[8][32];
			for (unsigned r=0;r<8;r++) for (unsigned lane=0;lane<32;lane++) prior[r][lane]=Read(r,lane);
			for (unsigned base:{0U,4U}) for (unsigned r=0;r<4;r++) for (unsigned lane=0;lane<32;lane++)
				lanes[base+r][lane]=prior[base+lane/8][r*8+lane%8];
			for (auto& time:ready) time=cycle+1;
			cycle++;
		} else if (code==0x90) {
			unsigned src=(op>>8)&15,dstReg=(op>>4)&15;
			assert(!(op&0x00fff00f) && src<8 && dstReg<8);
			for (unsigned lane=0;lane<32;lane++) {
				uint32_t sm=Read(src,lane);
				const float v=float(sm&0x7fffffff);
				assert(v<=262144 && sm!=0x80000000);
				lanes[dstReg][lane]=PioneerFmaBits(sm>>31?-v:v);
			}
			ready[dstReg]=cycle+1;cycle++;
		} else MetadataMachine::Op(op);
		assert(okay);
	}
};

static uint32_t BitsOfSum(int v)
{
	return v<0?0x80000000U|unsigned(-v):unsigned(v);
}

int main()
{
	(void)&Round;
	const float scales[]={0.0f,-0.0f,0.012345f,-0.03125f,1.0f,0x1p-32f,0x1p16f,-0x1p16f};
	uint32_t seed=0xea68b451;
	auto random=[&](){ seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed; };
	unsigned outputs=0,panels=0,imports=0,nonzero=0,distinct=0;
	for (unsigned blocks:{1U,20U,24U,68U}) for (unsigned trial=0;trial<12;trial++) {
		PairMachine m;float expected[2][16]={};
		Pair::Begin(m);
		for (unsigned block=0;block<blocks;block++) {
			Tile raw;
			for (auto& word:raw) word=random();
			auto* bytes=reinterpret_cast<uint8_t*>(raw.data());
			// Include both signed zeros, finite half subnormals, and max finite.
			for (unsigned d=0;d<2;d++) for (unsigned col=0;col<16;col++) {
				static const uint16_t edge[]={0,0x8000,1,0x83ff,0x7bff,0xfbff};
				const unsigned half=(col<6)?edge[(col+d+block)%6]:((random()%0x7c00)|(random()&0x8000));
				bytes[d*32+col]=uint8_t(half);bytes[d*32+16+col]=uint8_t(half>>8);
			}
			const float s[]={scales[(trial+block)%8],scales[(trial+3*block+3)%8]};
			m.tile=&raw;
			assert(V::FiniteBlock(raw.data(),2304) && Pair::Block(m,0x90000,PioneerFmaBits(s[0]),PioneerFmaBits(s[1])));
			assert(m.drained);
			for (unsigned p=0;p<16;p++) {
				PioneerQ38Panel scalar={};assert(PioneerQ38UnpackPanel(12,bytes,2304,p,&scalar));
				int activation[2][16],sums[2]={};
				for (unsigned t=0;t<2;t++) for (unsigned j=0;j<16;j++) {
					activation[t][j]=p==0?-128:p==1?127:int(random()%256)-128;
					if (t==1 && p<2) activation[t][j]=-1-activation[t][j];
					sums[t]+=activation[t][j];
				}
				for (unsigned col=0;col<16;col++) {
					const unsigned group=128+(p/2)*8;
					for (unsigned t=0;t<4;t++) {
						assert(m.dst[group+t][col]==(t<2?scalar.coefficient[col]:0));
						assert(m.dst[group+4+t][col]==(t<2?scalar.minimum[col]:0));
						int dot=0;
						if (t<2) for (unsigned j=0;j<16;j++)
							dot+=activation[t][j]*int((scalar.operand[j*8+col/2]>>((col&1)*16))&1023);
						m.dst[Pair::Dot+t][col]=dot?BitsOfSum(dot):0x80000000;
						m.defined[Pair::Dot+t][col]=true;
						if (t>=2) continue;
						const float c=PioneerFmaFloat(scalar.coefficient[col])*s[t];
						const float b=PioneerFmaFloat(scalar.minimum[col])*s[t];
						expected[t][col]+=(c*float(dot))+(b*float(sums[t]));
					}
				}
				assert(Pair::Panel(m,p,sums[0],sums[1]) && m.drained);
				for (unsigned col=0;col<16;col++) {
					for (unsigned t=0;t<4;t++) {
						assert(m.dst[Pair::Acc+t][col]==(t<2?PioneerFmaBits(expected[t][col]):0));
						assert(m.dst[Pair::Sum+t][col]==(t<2?PioneerFmaBits(float(sums[t])):0));
						assert(m.dst[Pair::Scale+t][col]==(t<2?PioneerFmaBits(s[t]):0));
					}
					distinct+=m.dst[Pair::Acc][col]!=m.dst[Pair::Acc+1][col];
					nonzero+=m.dst[Pair::Acc][col]!=0;
					outputs+=2;
				}
				panels++;
			}
		}
		assert(m.imports==blocks && m.drains==1+blocks*(5+16));
		imports+=m.imports;
	}
	assert(distinct>outputs/4 && nonzero>outputs/4);
	PairMachine reject;
	for (unsigned bits:{0x2f7fffffU,0x47800001U,0x7f800000U,0xffc00000U}) {
		assert(!Pair::Block(reject,0x90000,bits,0) && !Pair::Block(reject,0x90000,0,bits));
	}
	assert(!Pair::Block(reject,0x91000,0,0));
	assert(!Pair::Panel(reject,16,0,0) && !Pair::Panel(reject,0,-2049,0) && !Pair::Panel(reject,0,0,2049));
	assert(!reject.commands && !reject.imports && !reject.drains);
	printf("PASS paired vector Q4 affine: %u per-panel bitwise row results; %u shared prefixes, "
		"%u panels, untouched padding, 12 inert refusals; no linked worker or hardware result\n",
		outputs,imports,panels);
}
