/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38AffineReuse.h"
#include "../PioneerQwen38Gguf.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static unsigned panels=0;
static void Check(unsigned type,const std::vector<uint8_t>& tile)
{
	PioneerQ38AffineReuse cache; assert(cache.Prepare(type,tile.data(),unsigned(tile.size())));
	PioneerQ38Panel output,expected; for (auto& word:output.operand) word=0xdeadbeef;
	for (unsigned p=0;p<(type==8?2U:16U);p++) {
		assert(PioneerQ38AffineOnly(type,tile.data(),unsigned(tile.size()),p,&expected));
		for (unsigned row=0;row<16;row++) output.coefficient[row]=output.minimum[row]=0x7fc00000;
		assert(cache.Panel(p,&output));
		for (unsigned row=0;row<16;row++) { assert(output.coefficient[row]==expected.coefficient[row]); assert(output.minimum[row]==expected.minimum[row]); }
		for (uint32_t word:output.operand) assert(word==0xdeadbeef); panels++;
	}
	auto before=output; assert(!cache.Panel(type==8?2:16,&output) && !memcmp(&before,&output,sizeof(output)));
	assert(!cache.Prepare(type,tile.data(),unsigned(tile.size())) && !cache.Panel(0,&output));
}
int main(int argc,char** argv)
{
	assert(argc==1 || argc==2); unsigned real=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned trial=0;trial<256;trial++) {
		std::vector<uint8_t> tile(PioneerQ38PanelTileBytes(type)); uint32_t seed=0xb8c76135^(trial*0x512d73f9)^type;
		for (auto& byte:tile) { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; byte=uint8_t(seed); }
		for (unsigned row=0;row<16;row++) {
			unsigned at=(type==14?208:0)*16+row; uint16_t half=uint16_t((trial*541+row*761)%0x7c00)|uint16_t((row&1)<<15);
			tile[at]=uint8_t(half); tile[at+16]=uint8_t(half>>8);
			if (type==12) { tile[32+row]=uint8_t(half); tile[48+row]=uint8_t(half>>8); }
		}
		Check(type,tile);
		PioneerQ38AffineReuse ordered; assert(ordered.Prepare(type,tile.data(),unsigned(tile.size())));
		PioneerQ38Panel output={}; auto before=output;
		assert(!ordered.Panel(1,&output) && !ordered.Panel(0,&output) && !memcmp(&before,&output,sizeof(output)));
	}
	// All finite HALF x signed8 products,including signed zeros/subnormals,
	// are EXACT FP32;this validates the new Q6 multiply independently.
	unsigned products=0;
	for (unsigned half=0;half<65536;half++) if (((half>>10)&31)!=31) {
		uint32_t base=0; assert(PioneerQ38BinaryScale(uint16_t(half),1,base)); float value; memcpy(&value,&base,4);
		for (int factor=-128;factor<=127;factor++) {
			uint32_t expected=0; assert(PioneerQ38BinaryScale(uint16_t(half),factor,expected));
			float actual=value*float(factor); uint32_t bits; memcpy(&bits,&actual,4); assert(bits==expected); products++;
		}
	}
	if (argc==2) {
		FILE* f=fopen(argv[1],"rb"); assert(f); PioneerQwen38Gguf model; assert(model.Open(f));
		for (unsigned t=0;t<model.Tensors().size();t++) {
			const auto& tensor=model.Tensors()[t]; if (!tensor.type) continue;
			unsigned bytes=PioneerQ38BlockBytes(tensor.type),rowBytes=unsigned(tensor.bytes/tensor.outputs);
			for (unsigned end=0;end<2;end++) {
				std::vector<uint8_t> tile(PioneerQ38PanelTileBytes(tensor.type)); unsigned first=end?unsigned(tensor.outputs)-16:0;
				for (unsigned row=0;row<16;row++) {
					uint8_t raw[210]={}; assert(model.ReadPacked(t,uint64_t(first+row)*rowBytes+(end?rowBytes-bytes:0),bytes,raw,bytes));
					for (unsigned b=0;b<bytes;b++) tile[b*16+row]=raw[b];
				}
				Check(tensor.type,tile); real++;
			}
		}
		assert(real==996 && model.Stable()); fclose(f);
	}
	printf("PASS OFFLINE affine reuse:%u exact panels/%u real tiles,%u exhaustive HALF*signed8 FP32 products incl signed zeros/half subnormals,strict panel order/one block lifetime/operand canaries/no retry;NO image/native/speed qualification\n",panels,real,products);
}
