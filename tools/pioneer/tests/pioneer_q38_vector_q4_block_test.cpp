/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_Q4_METADATA_MODEL_ONLY
#include "pioneer_q38_vector_q4_metadata_test.cpp"
#define PIONEER_Q38_BLOCK_AFFINE_DST
#include "../PioneerQ38CachedAffine.h"
#include "pioneer_blackhole_sfp_load.h"
#include <map>

// Separate actual-byte/physical-Dst observer. Retain the unchanged strict
// arithmetic model;observe RAW UNPACK and inverse SFPU row/column mappings,
// not conveniently pre-expanded coefficient arrays. No silicon timing claim.
struct BlockMetadataMachine {
	struct Hazard {};
	MetadataMachine vector;
	uint16_t physical[512][16]={};bool defined[512][16]={};unsigned ready[512][16]={};
	std::array<uint8_t,2304> tile;
	std::map<unsigned,unsigned> config;
	unsigned base,thread5=4,unpacks=0,stores=0,transposes=0,drains=0,events=0;
	unsigned dropStore=0,dropTranspose=0,dropDrain=0;
	bool pending=false,dirty=false,enabled=false;
	explicit BlockMetadataMachine(unsigned at):base(at) {
		for(unsigned r=0;r<512;r++)for(unsigned c=0;c<16;c++) {
			physical[r][c]=uint16_t(0x5193+r*773+c*31);defined[r][c]=true;
		}
		// Stale metadata is defined,nonzero,finite FP32 rather than a NaN
		// trap. Missing writes must be detected by comparison,not by making
		// the unchanged finite-domain arithmetic oracle abort on poison.
		for(unsigned r=128;r<192;r++)for(unsigned c=0;c<16;c++)StoreFloat(r,c,0x3f000000+r*16+c);
		for(auto& row:ready)for(auto& r:row)r=0;
	}
	static unsigned Adj(unsigned row){return ((row&0x1f8)<<1)|(row&0x207);}
	uint32_t FloatDst(unsigned row,unsigned col)const {
		unsigned hi=Adj(row),lo=hi+8;
		if(!defined[hi][col] || !defined[lo][col] || ready[hi][col]>vector.cycle || ready[lo][col]>vector.cycle)throw Hazard{};
		unsigned h=physical[hi][col],ieee=(h&0x8000)|((h&255)<<7)|((h>>8)&127);
		return (ieee<<16)|physical[lo][col];
	}
	void StoreFloat(unsigned row,unsigned col,uint32_t bits) {
		unsigned hi=Adj(row),lo=hi+8,h=bits>>16;
		// Metadata/base FP32 is finite normal or signed zero:mode3's
		// specified denormal flush cannot change any accepted value.
		assert(!(bits&0x7fffffff) || (bits&0x7f800000));
		physical[hi][col]=uint16_t((h&0x8000)|((h&127)<<8)|((h>>7)&255));physical[lo][col]=uint16_t(bits);
		defined[hi][col]=defined[lo][col]=true;ready[hi][col]=ready[lo][col]=vector.cycle+1;
	}
	void Config(unsigned index,unsigned value) {
		events++;if(pending || dirty)throw Hazard{};config[index]=value;
	}
	void Drain() {
		events++;drains++;if(drains==dropDrain)return;
		if(pending) {
			for(unsigned r=0;r<16;r++)for(unsigned c=0;c<16;c++) {
				unsigned byte=tile[r*16+c];physical[r][c]=uint16_t((byte<<5)|(byte?16:0));
				defined[r][c]=true;ready[r][c]=vector.cycle;
			}pending=false;
		}
		vector.cycle++;dirty=false;
	}
	void Op(uint32_t op) {
		events++;const unsigned code=op>>24;
		if(pending)throw Hazard{};
		if(op==0x42000081) {
			assert(!thread5 && config[1]==0xa03c8000 && config[64]==0x10001e && config[65]==16
			 && config[72]==14 && config[73]==0x13 && config[84]==0x00400040 && (config[76]+1)*16==base);
			pending=true;unpacks++;return;
		}
		if(op==0xb2050000 || op==0xb2050004){thread5=op&15;vector.cycle++;return;}
		if(op==0x3700000f || op==0x5e23fc00 || op==0x5120000a || op==0x5420000f){vector.cycle++;return;}
		if(op==0x8a000002){enabled=true;vector.cycle++;return;}
		if(!enabled)throw Hazard{};
		if(code==0x70 || code==0x72) {
			unsigned reg=(op>>20)&15,mode=(op>>16)&15,address=op&1023;
			assert(reg<8 && !(address&1) && !(op&0xfc00));
			if(code==0x72)stores++;
			for(unsigned lane=0;lane<32;lane++) {
				unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+bool(address&2);
				if(code==0x70) {
					if(mode==13) {
						assert(row<16);if(!defined[row][col] || ready[row][col]>vector.cycle)throw Hazard{};
						vector.lanes[reg][lane]=BlackholeRawSfpLoad(physical[row][col],13);
					} else {assert(mode==3 && row>=184 && row<192);vector.lanes[reg][lane]=FloatDst(row,col);}
				} else {
					assert(mode==3 && row>=128 && row<192);
					if(stores!=dropStore)StoreFloat(row,col,vector.Read(reg,lane));
				}
			}
			if(code==0x70)vector.ready[reg]=vector.cycle+1;else dirty=true;
			vector.cycle++;
		} else if(code==0x8c) {
			assert(op==0x8c000000);transposes++;
			if(transposes!=dropTranspose) {
				uint32_t old[8][32];for(unsigned r=0;r<8;r++)for(unsigned l=0;l<32;l++)old[r][l]=vector.Read(r,l);
				for(unsigned b:{0U,4U})for(unsigned r=0;r<4;r++)for(unsigned g=0;g<4;g++)for(unsigned c=0;c<8;c++)
					vector.lanes[b+r][g*8+c]=old[b+g][r*8+c];
				for(auto& r:vector.ready)r=vector.cycle+1;
			}vector.cycle++;
		} else vector.Op(op);
		if(!vector.okay)throw Hazard{};
	}
	void Closed()const{if(pending || dirty || thread5!=4)throw Hazard{};}
};

static unsigned Check(const BlockMetadataMachine& machine) {
	PioneerQ38CachedAffine scalar;PioneerQ38Panel panel={};std::array<uint32_t,256> scratch;
	assert(scalar.Prepare(12,machine.tile.data(),2304) && scalar.BlockQ4(&panel,scratch.data(),256));
	unsigned different=0;
	for(unsigned group=0;group<8;group++)for(unsigned kind=0;kind<2;kind++)
	for(unsigned pad=0;pad<4;pad++)for(unsigned col=0;col<16;col++)
		different+=machine.FloatDst(128+group*8+kind*4+pad,col)!=(pad?0:scratch[group*32+kind*16+col]);
	return different;
}
int main() {
	uint32_t seed=0x91385742;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
	const unsigned bases[]={0x90000,0x98000,0x98900,0x9a000,0x9a900};
	const uint16_t edges[]={0,0x8000,1,0x8001,0x3ff,0x83ff,0x400,0x8400,0x7bff,0xfbff};
	unsigned cases=0,refused=0;
	for(unsigned trial=0;trial<1024;trial++) {
		BlockMetadataMachine machine(bases[trial%5]);
		for(auto& b:machine.tile)b=uint8_t(random());
		for(unsigned kind=0;kind<2;kind++)for(unsigned col=0;col<16;col++) {
			uint16_t h=trial<10?edges[(trial+col+kind*3)%10]:uint16_t((random()%0x7c00)|(random()&0x8000));
			machine.tile[kind*32+col]=uint8_t(h);machine.tile[kind*32+16+col]=uint8_t(h>>8);
		}
		std::array<uint32_t,576> words;memcpy(words.data(),machine.tile.data(),2304);
		assert(V::FiniteBlock(words.data(),2304));
		uint16_t before[512][16];memcpy(before,machine.physical,sizeof(before));
		assert(V::Block(machine,machine.base,0x3e876543));machine.Closed();assert(!Check(machine));
		assert(machine.unpacks==1 && machine.stores==36 && machine.transposes==6 && machine.drains==4);
		for(unsigned r=16;r<512;r++)if(r<256 || r>=384)for(unsigned c=0;c<16;c++)assert(machine.physical[r][c]==before[r][c]);
		for(unsigned l=0;l<32;l++)assert(machine.vector.lanes[3][l]==0x3e876543);
		cases++;
		// Finite-domain refusal:every exponent31 high-byte lane,including
		// BOTH bases and all output columns. It is rejected BEFORE issue.
		if(!trial)for(unsigned kind=0;kind<2;kind++)for(unsigned col=0;col<16;col++) {
			auto bad=words;reinterpret_cast<uint8_t*>(bad.data())[kind*32+16+col]|=0x7c;
			assert(!V::FiniteBlock(bad.data(),2304));refused++;
		}
	}
	for(unsigned scale:{0x2f7fffffU,0x47800001U,0x7f800000U,0x7fc00000U}) {
		BlockMetadataMachine m(0x98000);assert(!V::Block(m,m.base,scale) && !m.events);refused++;
	}
	for(unsigned base:{0U,0x90001U,0x94000U,0x98010U,0x9b200U,UINT32_MAX}) {
		BlockMetadataMachine m(base);assert(!V::Block(m,base,0x3f800000) && !m.events);refused++;
	}
	// Broken first/last import/transpose/final drain must not silently
	// look like the same valid block. Dirty padding is compared too.
	unsigned faults=0;
	for(unsigned fault=0;fault<8;fault++) {
		BlockMetadataMachine m(0x98000);m.tile.fill(0x35);m.tile[16]=m.tile[48]=0x3c;
		if(fault<4)m.dropStore=fault==0?1:fault==1?2:fault==2?35:36;
		if(fault==4 || fault==5)m.dropTranspose=fault==4?1:6;
		if(fault>=6)m.dropDrain=fault==6?2:4;
		bool caught=false;
		try{assert(V::Block(m,m.base,0x3f800000));m.Closed();caught=Check(m)!=0;}catch(const BlockMetadataMachine::Hazard&){caught=true;}
		assert(caught);faults++;
	}
	assert(!V::FiniteBlock(nullptr,2304));refused++;
	printf("PASS_VECTOR_Q4_BLOCK actual raw256B prefix/physical Dst:%u original-byte blocks/all8 groups/all16 outputs+ALL zero padding EXACT to unchanged scalar BlockQ4;all5 twin addresses;preserved accumulator/otherDst/scale3;one UNPACK/no bank flip/no scalar products or L1 metadata stores;%u domain refusals,%u missing store/transpose/drain controls;SYNTHETIC_ENDPOINTS_ONLY,NO native/hardware/speed grant\n",cases,refused,faults);
	(void)&Round;
}
