/* SPDX-License-Identifier: MIT */
#include "../PioneerDecoderEmbedding.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ8NativeF32Layout.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
struct EmbeddingIO {
	static const uint32_t Base=0x40000000;
	unsigned token,reads=0,corrupt=0,fail=0;
	bool badScale=false,badInteger=false;
	std::vector<uint32_t> packed=std::vector<uint32_t>(18432/4);
	float reference[1024]={};
	EmbeddingIO(unsigned selected):token(selected) {
		for (unsigned row=0;row<16;row++) for (unsigned b=0;b<32;b++) {
			uint16_t half=uint16_t(((row+b)&1?0x8000:0)|((row*17+b*29)%0x2400+0x1000));
			_Float16 h; memcpy(&h,&half,2); float scale=float(h);
			packed[PioneerQ8NativeF32ScaleByte(1024,b,row)/4]=PioneerAttentionBits(scale);
			for (unsigned i=0;i<32;i++) {
				int q=int((row*23+b*19+i*13)%255)-127;
				unsigned encoded=q<0?0x80|unsigned(-q):unsigned(q);
				unsigned byte=unsigned(PioneerQ8NativeF32ValueByte(1024,b*32+i,row)); packed[byte/4]|=encoded<<((byte&3)*8);
				if (row==(token&15)) reference[b*32+i]=float(q)*scale;
			}
		}
	}
	bool ReadNativeEmbeddingBlock(uint32_t address,uint32_t (&words)[144]) {
		unsigned block=reads/2;
		assert(reads<64 && address==Base+(token/16)*18432+block*576 && !(address&31)); reads++;
		if (reads==fail) return false;
		for (unsigned i=0;i<144;i++) words[i]=packed[block*144+i];
		if (reads==corrupt) words[143]^=1; // ALL-bit compare,including an unrelated row
		if (badScale) words[token&15]=0x3f800001;
		if (badInteger) { unsigned byte=64+(token&15); words[byte/4]=(words[byte/4]&~(255U<<((byte&3)*8)))|(128U<<((byte&3)*8)); }
		return true;
	}
};
#ifndef PIONEER_EMBEDDING_IO_ONLY
int main()
{
	for (unsigned tile:{0U,1U,947U,9495U}) for (unsigned row=0;row<16;row++) {
		EmbeddingIO io(tile*16+row); float output[1026]; for (auto& v:output) v=42;
		assert(PioneerDecoderEmbedding(io,io.token,io.Base,output+1) && io.reads==64 && output[0]==42 && output[1025]==42);
		for (unsigned i=0;i<1024;i++) assert(PioneerAttentionBits(output[i+1])==PioneerAttentionBits(io.reference[i]));
	}
	for (unsigned packet=1;packet<=64;packet++) for (bool fail:{false,true}) {
		EmbeddingIO io(151935); float output[1024]; if (fail) io.fail=packet; else io.corrupt=packet;
		assert(!PioneerDecoderEmbedding(io,io.token,io.Base,output)); assert(io.reads==(fail?packet:(packet+1)/2*2));
	}
	for (unsigned fault=0;fault<6;fault++) {
		EmbeddingIO io(15); float output[1024];
		bool okay=fault==0?PioneerDecoderEmbedding(io,151936,io.Base,output)
			:fault==1?PioneerDecoderEmbedding(io,15,io.Base+1,output)
			:fault==2?PioneerDecoderEmbedding(io,15,0x82400000-4096,output)
			:fault==3?PioneerDecoderEmbedding(io,15,io.Base,nullptr)
			:(io.badScale=fault==4,io.badInteger=fault==5,PioneerDecoderEmbedding(io,15,io.Base,output));
		assert(!okay && io.reads==(fault<4?0U:2U));
	}
	puts("PASS OFFLINE resident tied-embedding prerequisite:independent native packing/IEEE GGUF dequant bits,first/last vocabulary tiles/all16 rows/1024 values/guards,64-read exact source plan,ALL-bit corruption/timeout/format/bounds refusal;NOT wired/native");
}
#endif
