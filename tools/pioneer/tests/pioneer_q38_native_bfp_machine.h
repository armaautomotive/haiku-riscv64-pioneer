/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_BFP_MACHINE_H
#define PIONEER_Q38_NATIVE_BFP_MACHINE_H
#include "../PioneerQ38NativeBfpMatvec.h"
#include "../PioneerBlackholeBlockFloat.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

// Independent OFFBOARD ISA observer,not a physical timing model. Decode
// descriptors/exponent addressing/transpose/fidelity/bank operations,NOT a
// call to the producer or CPU tile UnpackTile for simulated hardware.
struct NativeBfpMachine {
	// Independent numeric hardware contract,official cfg81989dcdb's
	// tt_metal/hw/inc/blackhole/tensix_types.h,NOT the producer's mapping.
	enum { Bfloat16=5,Bfp8B=6,Bfp4B=7 };
	static constexpr uint32_t Unknown=0x5542464e;
	unsigned config[224]={},thread[68]={},xStart[2]={},xEnd[2]={};
	unsigned rwcA=0,rwcB=0,crA=0,crB=0,fidelity=0,unpackBank[2]={},matrixBank[2]={};
	unsigned cycle=0,drains=0,unpacks[2]={},matrixOps=0,visits[4]={},outputReads=0;
	unsigned delay[2]={},dropProducer=0,dropPhase=99;bool omitFlip=false,mathPending=false;
	unsigned expectedWeight=0x90000,expectedInput=0x91000,alternateWeight=0,alternateInput=0,thirdInput=0;
	struct Source { float values[64][16];bool owner=false,pending=false;unsigned due=0; } source[2][2];
	float destination[64][16]={};bool valid[64]={};
	std::vector<uint8_t> l1;
	NativeBfpMachine():l1(0x180000,0xa7) {
		for(auto& operand:source)for(auto& bank:operand)for(auto& row:bank.values)for(auto& v:row)v=NAN;
		for(auto& row:destination)for(auto& v:row)v=NAN;
		std::fill(std::begin(valid),std::end(valid),true);
	}
	static uint32_t Bits(float v) { uint32_t b;memcpy(&b,&v,4);return b; }
	static float Float(uint32_t b) { float v;memcpy(&v,&b,4);return v; }
	bool Pending() const { return source[0][unpackBank[0]].pending || source[1][unpackBank[1]].pending; }
	void Advance() {
		for(unsigned w=0;w<2;w++) {
			auto& s=source[w][unpackBank[w]];
			if(s.pending && cycle>=s.due && dropProducer!=w+1) {
				s.pending=false;s.owner=true;unpackBank[w]^=1;
			}
		}
	}
	void Drain() {
		for(unsigned w=0;w<2;w++) {
			const auto& s=source[w][unpackBank[w]];
			if(s.pending) { if(dropProducer==w+1)throw Unknown;cycle=std::max(cycle,s.due); }
		}
		Advance();mathPending=false;drains++;
	}
	void Config(unsigned word,unsigned value) {
		if(Pending() || mathPending)throw Unknown;
		assert(word<224);config[word]=value;
	}
	void Stage(const uint8_t* weights,unsigned count,const std::array<uint16_t,32>& input) {
		assert(!Pending() && !mathPending && (count==576 || count==1088));
		memcpy(l1.data()+0x90000,weights,count);
		for(unsigned k=0;k<2;k++)for(unsigned row=0;row<8;row++)for(unsigned c=0;c<16;c++) {
			const unsigned at=0x91000+((k*8+row)*16+c)*2;
			l1[at]=uint8_t(input[k*16+c]);l1[at+1]=uint8_t(input[k*16+c]>>8);
		}
	}
	// ISA BFP sign/magnitude -> normalized BF16 value,using numerical
	// powers rather than the CPU codec's integer normalizing bit shifts.
	float Datum(unsigned address,unsigned element,unsigned format,unsigned exponents) const {
		if(format==5) { unsigned at=address+element*2;assert(at+1<l1.size());return Float(uint32_t(l1[at]|(unsigned(l1[at+1])<<8))<<16); }
		assert(format==Bfp8B || format==Bfp4B);
		unsigned width=format==6?8:4;
		unsigned at=address+exponents+element*width/8;assert(at<l1.size());
		unsigned q=(l1[at]>>((element*width)%8))&((1U<<width)-1);
		unsigned magnitude=q&((1U<<(width-1))-1),exponent=l1[address+element/16];
		if(!magnitude)return q? -INFINITY:0;
		return float(std::ldexp((q>>(width-1)?-1.L:1.L)*magnitude,int(exponent)-127-int(width-2)));
	}
	void Unpack(unsigned which) {
		assert(which<2 && thread[0]==0 && thread[5]==4);
		const unsigned base=64+which*48,format=config[base]&15,out=config[base+8]&15;
		assert(format==out && (format==Bfloat16 || format==Bfp8B || format==Bfp4B));
		assert((config[base]&0xf0)==0x10 && !(config[base+3]>>24)); // EXP section,header16
		assert((config[base+8]&~0x12fU)==0 && !(config[base+9]&0x1f0)); // no Dst/tileize/forced exp
		assert((config[base+9]&1) && config[base+10]==0x17fff && config[base+11]==0 && config[base+28]==0);
		unsigned x=which?config[base]>>16:config[86]&65535;
		unsigned y=config[base+1]&255,z=(config[base+1]>>16)&255,w=config[base+2]&255;
		if(!z)z=1;if(!w)w=1;
		const unsigned exponentBytes=format==5?0:((x*y*z*w+255)/256)*16;
		unsigned address=(config[base+12]+1)*16,count=xEnd[which]+1-xStart[which];
		assert(xStart[which]==0 && (which?(address==expectedInput || (alternateInput && address==alternateInput)
			|| (thirdInput && address==thirdInput)):(address==expectedWeight || (alternateWeight && address==alternateWeight))));
		assert(count==(which?256U:1024U));
		auto& s=source[which][unpackBank[which]];assert(!s.pending && !s.owner);
		unsigned destinationBase=which?config[61]:config[49]+(config[84]&65535);
		if(format==5) { assert(!(destinationBase&1));destinationBase/=2; }
		for(unsigned i=0;i<count;i++) {
			unsigned row=(destinationBase+i)/16,col=(destinationBase+i)%16;
			if(!which) { assert(row>=4);row=(row-4)&63;
				if(config[base+8]&0x100) { unsigned low=row&15;row=(row&~15U)|col;col=low; }
			}
			assert(row<64);s.values[row][col]=Datum(address,i,format,exponentBytes);
		}
		s.pending=true;s.due=cycle+delay[which];unpacks[which]++;
	}
	static float Part(float x,unsigned phase,bool operandA) {
		if(x==0)return x;
		int exponent;long double magnitude=std::frexp(std::fabs(static_cast<long double>(x)),&exponent);
		const unsigned high=operandA?5:7;
		long double upper=std::ldexp(std::floor(std::ldexp(magnitude,int(high))),exponent-int(high));
		float leading=float(std::signbit(x)?-upper:upper);
		return (phase&(operandA?1:2))?x-leading:leading;
	}
	void Matrix(unsigned op) {
		assert(thread[2]==1 && thread[3]==1 && !config[2] && !config[3]);
		assert((config[1]&0xe0000000)==0x60000000 && !(config[1]&0x1ffff));
		assert((config[0]&0x4210)==0x4210 && ((config[0]>>5)&15)==5);
		assert((config[0]&15)==(config[64]&15) && !thread[11] && !thread[13] && !thread[48]);
		assert(((op>>14)&3)==1 && !(op&0x003fbc00));
		auto& a=source[0][matrixBank[0]];auto& b=source[1][matrixBank[1]];
		if(!a.owner || !b.owner)throw Unknown;
		unsigned ar=rwcA&0x38,br=rwcB&0x38,dr=op&1023;assert(ar<=48 && br<=56 && (dr==32 || dr==40));
		const unsigned phase=(fidelity+thread[11])&3;visits[phase]++;
		for(unsigned row=0;row<8;row++)for(unsigned c=0;c<16;c++) {
			long double sum=0;
			for(unsigned k=0;k<16;k++)sum+=static_cast<long double>(Part(b.values[br+row][k],phase,false))*Part(a.values[ar+k][c],phase,true);
			if(phase!=dropPhase)destination[dr+row][c]=float(float(sum)+(valid[dr+row]?destination[dr+row][c]:0));
		}
		for(unsigned row=0;row<8;row++)valid[dr+row]=true;
		if(!omitFlip)for(unsigned which=0;which<2;which++)if(op&(1U<<(22+which))) {
			source[which][matrixBank[which]].owner=false;matrixBank[which]^=1;
		}
		fidelity=(fidelity+((thread[29]>>13)&3))&3;matrixOps++;mathPending=true;
	}
	void Op(unsigned op) {
		cycle++;Advance();unsigned code=op>>24;
		if(code==0xb2) { assert(!Pending() && !mathPending);unsigned word=(op>>16)&255;assert(word<68);thread[word]=op&65535; }
		else if(code==0x37) {
			assert(!(op&0x00c00000));unsigned mask=op&63,cr=(op>>18)&15;
			assert(!(mask&~15U) && !(cr&~3U) && ((op>>14)&15)==0);
			if(mask&1) { rwcA=((op>>6)&15)+((cr&1)?crA:0);crA=rwcA; }
			if(mask&2) { rwcB=((op>>10)&15)+((cr&2)?crB:0);crB=rwcB; }
			if(mask&8)fidelity=0;
		}
		else if(op==0x10180000) { for(auto& v:valid)v=false; }
		else if(code==0x5e) {
			unsigned selection=(op>>21)&7;assert(selection && !(selection&4));
			for(unsigned which=0;which<2;which++)if(selection&(1U<<which)) { xStart[which]=op&1023;xEnd[which]=(op>>10)&2047; }
		}
		else if(op==0x5160000b || op==0x5460000f) {} // other ADCs are fixed zero
		else if(op==0x420000c1 || op==0x428000c1)Unpack((op>>23)&1);
		else if(code==0x26)Matrix(op);
		else { assert(false && "Unqualified native BFP opcode"); }
	}
	bool Clean() const {
		if(Pending() || mathPending)return false;
		for(const auto& operand:source)for(const auto& bank:operand)if(bank.owner || bank.pending)return false;
		return matrixBank[0]==unpackBank[0] && matrixBank[1]==unpackBank[1];
	}
	std::array<float,32> Results() {
		if(!Clean())throw Unknown;
		std::array<float,32> out;
		for(unsigned i=0;i<32;i++) { assert(valid[i<16?32:40]);out[i]=destination[i<16?32:40][i%16]; }
		outputReads+=32;return out;
	}
};
#endif
