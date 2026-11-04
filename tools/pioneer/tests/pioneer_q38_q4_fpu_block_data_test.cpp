/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q4FpuBlockData.h"
#include "../TenstorrentQ4K.h"
#include <array>
#include <cassert>
#include <cstdio>
static uint32_t Bits(float x){uint32_t bits;memcpy(&bits,&x,4);return bits;}
static uint32_t Read(const uint8_t* p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
int main()
{
	using D=PioneerQ38Q4FpuBlockData;
	std::array<uint8_t,D::OriginalBytes> original;std::array<uint8_t,D::Bytes+16> output;
	uint32_t seed=0xa381b047;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
	unsigned fields=0,quant=0,refused=0;
	for(unsigned trial=0;trial<128;trial++) {
		for(auto& b:original)b=uint8_t(random());
		for(unsigned row=0;row<32;row++)for(unsigned at:{0U,2U}) {
			uint8_t& hi=original[row*144+at+1];if((hi&0x7c)==0x7c)hi^=4;
			if(trial<4){original[row*144+at]=uint8_t(1+row);hi=uint8_t(trial&1?0x80:0);}
		}
		output.fill(0xa7);assert(D::Prepare(original.data(),original.size(),output.data(),D::Bytes));
		for(unsigned i=D::Bytes;i<output.size();i++)assert(output[i]==0xa7);
		for(unsigned row=0;row<32;row++) {
			const uint8_t* q=original.data()+row*144;
			for(unsigned byte=0;byte<128;byte++){assert(output[(row/16)*2048+byte*16+row%16]==q[16+byte]);quant++;}
			for(unsigned group=0;group<8;group++) {
				unsigned s,m;
				if(group<4){s=q[4+group]&63;m=q[8+group]&63;}
				else{s=(q[8+group]&15)|((q[group]>>6)<<4);m=(q[8+group]>>4)|((q[4+group]>>6)<<4);}
				float c=PioneerHalfFloat(uint16_t(q[0]|(unsigned(q[1])<<8)))*float(s);
				float n=PioneerHalfFloat(uint16_t(q[2]|(unsigned(q[3])<<8)))*float(m);
				unsigned at=4096+(group*4+(row/16)*2)*64+(row%16)*4;
				assert(Read(output.data()+at)==Bits(c) && Read(output.data()+at+64)==(Bits(n)^0x80000000));fields+=2;
			}
		}
	}
	for(unsigned row=0;row<32;row++)for(unsigned at:{0U,2U})for(unsigned special:{0x7c00U,0xfc00U,0x7fffU,0xffffU}) {
		auto bad=original;bad[row*144+at]=uint8_t(special);bad[row*144+at+1]=uint8_t(special>>8);output.fill(0xa7);
		assert(!D::Prepare(bad.data(),bad.size(),output.data(),D::Bytes));for(auto byte:output)assert(byte==0xa7);refused++;
	}
	assert(!D::Prepare(nullptr,D::OriginalBytes,output.data(),D::Bytes));
	assert(!D::Prepare(original.data(),D::OriginalBytes,nullptr,D::Bytes));
	assert(!D::Prepare(original.data(),D::OriginalBytes-1,output.data(),D::Bytes));
	assert(!D::Prepare(original.data(),D::OriginalBytes,output.data(),D::Bytes-1));
	assert(!D::Prepare(output.data(),D::OriginalBytes,output.data()+1,D::Bytes));
	printf("PASS CPU-only Q4 FPU recipe:%u unchanged packed bytes,%u exact FP32 metadata fields,subnormal half scales/signed zero retained,%u nonfinite atomic refusals,bounds/overlap/canary checks;NO disk/model/driver/device authority\n",quant,fields,refused);
}
