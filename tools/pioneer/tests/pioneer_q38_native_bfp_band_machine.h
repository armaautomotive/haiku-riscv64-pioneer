/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_BFP_BAND_MACHINE_H
#define PIONEER_Q38_NATIVE_BFP_BAND_MACHINE_H
#include "pioneer_q38_native_bfp_machine.h"
#include "../PioneerQ38NativeBfpBand.h"
// Extends ONLY the new full-band L1/UNPACK/broadcast contract. Original
// first-tile observer and strict RV32 decoder are UNCHANGED. Arithmetic here
// remains that observer's IDEAL phasewise FP32 model, NOT silicon rounding.
struct NativeBfpBandMachine:NativeBfpMachine {
	unsigned tiles,phases,clears=0;const unsigned maxBands;
	const unsigned weightBase,inputBase,weightStride,inputStride,headerBytes,operandLimit;
	NativeBfpBandMachine(unsigned k,unsigned fidelity,unsigned w=0x90000,unsigned b=0x124000,
		unsigned ws=1104,unsigned bs=80,unsigned h=16,unsigned limit=0x130000,unsigned bands=2)
		:tiles(k/32),phases(fidelity),maxBands(bands),weightBase(w),inputBase(b),weightStride(ws),inputStride(bs),headerBytes(h),operandLimit(limit) {
		assert((k==5120 || k==6144 || k==17408) && (phases==1 || phases==4));
		assert(bands>=1 && bands<=256);
		assert(h>=16 && !(h&15) && ws==1088+h && bs==64+h && w>=h && b>=h);
		assert(w-h+tiles*ws<=b-h && b-h+tiles*bs<=limit && limit<=l1.size());
	}
	void UnpackBand(unsigned which) {
		assert(which<2 && thread[0]==0 && thread[5]==4);
		const unsigned base=64+which*48,format=config[base]&15;
		assert(format==(which?5U:6U) && (config[base+8]&15)==format);
		assert((config[base]&0xf0)==0x10 && !(config[base+3]>>24));
		assert((config[base+8]&~0x12fU)==0 && !(config[base+9]&0x1f0));
		assert((config[base+9]&1) && config[base+10]==0x17fff && !config[base+11] && !config[base+28]);
		assert(config[base]==(which?0x00200015U:0x01000016U));
		assert(config[base+1]==(which?0x00010001U:0x00040001U) && config[base+2]==1);
		const unsigned address=(config[base+12]+1)*16,tile=unpacks[which]%tiles;
		assert(address==(which?inputBase+tile*inputStride:weightBase+tile*weightStride));
		assert(xStart[which]==0 && xEnd[which]==(which?31U:1023U));
		assert(address>=headerBytes && address+(which?64U:1088U)<=operandLimit);
		for(unsigned h=0;h<headerBytes;h++)assert(!l1[address-headerBytes+h]);
		auto& s=source[which][unpackBank[which]];if(s.pending || s.owner)throw Unknown;
		unsigned destinationBase=which?config[61]:config[49]+(config[84]&65535);
		if(which){assert(!(destinationBase&1));destinationBase/=2;}
		for(unsigned i=0;i<(which?32U:1024U);i++) {
			unsigned row=(destinationBase+i)/16,col=(destinationBase+i)%16;
			if(!which){assert(row>=4);row=(row-4)&63;if(config[base+8]&0x100){const unsigned low=row&15;row=(row&~15U)|col;col=low;}}
			assert(row<64);s.values[row][col]=Datum(address,i,format,which?0:64);
		}
		s.pending=true;s.due=cycle+delay[which];unpacks[which]++;
	}
	void BroadcastMatrix(unsigned op) {
		assert(thread[2]==1 && thread[3]==1 && !config[2] && !config[3]);
		assert((config[1]&0xe0000000)==0x60000000 && !(config[1]&0x1ffff));
		assert((config[0]&0x4210)==0x4210 && ((config[0]>>5)&15)==5 && (config[0]&15)==6);
		assert(!thread[11] && !thread[13] && !thread[48] && ((thread[29]>>13)&3)==1);
		const unsigned dr=op&1023;assert(dr==32 || dr==40);
		assert((op&~0x00c003ffU)==0x26084000); // REQUIRED broadcast19 + AddrMod1
		auto& a=source[0][matrixBank[0]];auto& b=source[1][matrixBank[1]];if(!a.owner || !b.owner)throw Unknown;
		const unsigned ar=rwcA&0x30,br=rwcB&63,phase=(fidelity+thread[11])&3;
		assert(ar<=48 && br==(ar/16&1) && dr==(ar<32?32U:40U));
		assert(phases==4 || phase==0);visits[phase]++;
		// Broadcast's mathematical product is identical for its written rows.
		// Only rows0/2/4/6 are written; valid bits are upper4 + requested dr.
		for(unsigned col=0;col<16;col++) {
			long double sum=0;
			for(unsigned k=0;k<16;k++)sum+=static_cast<long double>(Part(b.values[br][k],phase,false))*Part(a.values[ar+k][col],phase,true);
			if(phase!=dropPhase)for(unsigned row=0;row<7;row+=2)destination[dr+row][col]=float(float(sum)+(valid[dr+row]?destination[dr+row][col]:0));
		}
		valid[dr]=true;for(unsigned row=4;row<8;row++)valid[dr+row]=true;
		if(!omitFlip)for(unsigned which=0;which<2;which++)if(op&(1U<<(22+which))){source[which][matrixBank[which]].owner=false;matrixBank[which]^=1;}
		fidelity=(fidelity+1)&3;matrixOps++;mathPending=true;
	}
	void Op(unsigned op) {
		if(op==0x10180000) {
			assert(clears<maxBands && unpacks[0]==clears*tiles && unpacks[1]==unpacks[0] && matrixOps==clears*tiles*4*phases);clears++;
			NativeBfpMachine::Op(op);
		}else if(op==0x420000c1 || op==0x428000c1){cycle++;Advance();UnpackBand((op>>23)&1);}
		else if((op>>24)==0x26){cycle++;Advance();BroadcastMatrix(op);}
		else NativeBfpMachine::Op(op);
	}
	struct Fixture {
		std::vector<std::array<uint8_t,1088>> packed;
		std::vector<std::array<uint16_t,32>> input;
		std::array<float,32> full{},fast{};
	};
	static Fixture Data(unsigned k) {
		Fixture data;data.packed.resize(k/32);data.input.resize(k/32);
		for(unsigned tile=0;tile<k/32;tile++) {
			std::array<float,1024> original{},decoded{};
			for(unsigned r=0;r<32;r++)for(unsigned c=0;c<32;c++)original[r*32+c]=float(int((r*37+c*19+tile*11)%255)-127)/128;
			for(unsigned c=0;c<32;c++)data.input[tile][c]=uint16_t(0x3f00+((c*13+tile*7)%256)+((c&3)==2?0x8000:0));
			assert(PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,original.data(),1024,data.packed[tile].data(),1088));
			assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,data.packed[tile].data(),1088,decoded.data(),1024));
			// Independent scalar reference over the original untransposed data,
			// with explicit SAME phasewise FP32 sum order;not kernel emulation.
			for(unsigned half=0;half<2;half++)for(unsigned phase=0;phase<4;phase++)for(unsigned row=0;row<32;row++) {
				long double sum=0;for(unsigned c=0;c<16;c++)sum+=static_cast<long double>(Part(Float(uint32_t(data.input[tile][half*16+c])<<16),phase,false))*Part(decoded[row*32+half*16+c],phase,true);
				data.full[row]=float(float(sum)+data.full[row]);if(!phase)data.fast[row]=float(float(sum)+data.fast[row]);
			}
		}
		return data;
	}
	void Stage(const Fixture& data) {
		assert(data.packed.size()==tiles && data.input.size()==tiles && !unpacks[0] && !unpacks[1]);
		for(unsigned tile=0;tile<tiles;tile++) {
			const unsigned wa=weightBase+tile*weightStride,ba=inputBase+tile*inputStride;
			memset(l1.data()+wa-headerBytes,0,headerBytes);memcpy(l1.data()+wa,data.packed[tile].data(),1088);memset(l1.data()+ba-headerBytes,0,headerBytes);
			for(unsigned c=0;c<32;c++){l1[ba+c*2]=uint8_t(data.input[tile][c]);l1[ba+c*2+1]=uint8_t(data.input[tile][c]>>8);}
		}
	}
};
#endif
