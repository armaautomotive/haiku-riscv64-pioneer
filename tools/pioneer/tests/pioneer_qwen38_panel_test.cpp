/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen38Panel.h"
#include "../PioneerQwen38FastPanel.h"
#include "../PioneerQ38BinaryScale.h"
#include "../PioneerQwen38TiledReader.h"
#include "../TenstorrentGgufQuant.h"
#include <cassert>
#include <cmath>
#include <random>
static int Operand(uint16_t value)
{
	assert(PioneerInt8OperandValid(value)); return int(value&1023)*((value&0x8000)?-1:1);
}
static void Compare(unsigned type,const uint8_t* tiled,const uint8_t* raw,unsigned rowBytes)
{
	assert(PioneerQ38PanelTileBytes(type)==PioneerQ38TileBytes(type));
	for (unsigned p=0;p<(type==8?2U:16U);p++) {
		PioneerQ38Panel panel={}; assert(PioneerQ38UnpackPanel(type,tiled,PioneerQ38TileBytes(type),p,&panel));
		PioneerQ38Panel fast={}; assert(PioneerQ38FastPanel(type,tiled,PioneerQ38TileBytes(type),p,&fast) && !memcmp(&panel,&fast,sizeof(panel)));
		for (unsigned row=0;row<16;row++) {
			float values[256]={}; const uint8_t* source=raw+row*rowBytes;
			assert(type==8?PioneerDecodeQ8_0(source,values):type==12?PioneerDecodeQ4K(source,values):PioneerDecodeQ6K(source,values));
			for (unsigned lane=0;lane<16;lane++) {
				unsigned element=lane*16+row;
				int q=Operand(uint16_t(panel.operand[element/2]>>((element%2)*16)));
				float reconstructed=PioneerAttentionFloat(panel.coefficient[row])*q+PioneerAttentionFloat(panel.minimum[row]);
				float expected=values[p*16+lane];
				assert(reconstructed==expected);
			}
		}
	}
}
int main(int argc,char** argv)
{
	assert(argc<=2); std::mt19937 random(0x381627);
	// Exhaust ALL finite/invalid half bit patterns and signed scale factors.
	for (unsigned bits=0;bits<65536;bits++) for (int factor=-128;factor<=127;factor++) {
		uint32_t output=0x12345678; bool valid=PioneerQ38PanelScale(uint16_t(bits),factor,output);
		uint32_t binary=0x12345678; assert(PioneerQ38BinaryScale(uint16_t(bits),factor,binary)==valid && binary==output);
		assert(valid==((bits&0x7c00)!=0x7c00));
		if (valid) assert(output==PioneerAttentionBits(PioneerAttentionHalf(uint16_t(bits))*float(factor)));
		else assert(output==0x12345678);
	}
	for (unsigned type:{8U,12U,14U}) for (unsigned trial=0;trial<128;trial++) {
		unsigned bytes=PioneerQ38BlockBytes(type),tileBytes=PioneerQ38TileBytes(type);
		alignas(4) uint8_t raw[16*210]={},tile[3392]={};
		for (unsigned row=0;row<16;row++) {
			for (unsigned byte=0;byte<bytes;byte++) raw[row*bytes+byte]=uint8_t(random());
			unsigned d=type==14?208:0;
			uint16_t half=uint16_t(random()); if ((half&0x7c00)==0x7c00) half^=0x400;
			raw[row*bytes+d]=uint8_t(half); raw[row*bytes+d+1]=uint8_t(half>>8);
			if (type==12) { half=uint16_t(random()); if ((half&0x7c00)==0x7c00) half^=0x400;
				raw[row*bytes+2]=uint8_t(half); raw[row*bytes+3]=uint8_t(half>>8); }
			for (unsigned byte=0;byte<bytes;byte++) tile[byte*16+row]=raw[row*bytes+byte];
		}
		Compare(type,tile,raw,bytes);
		PioneerQ38Panel output; memset(&output,0xa5,sizeof(output)); auto before=output;
		assert(!PioneerQ38UnpackPanel(type,tile,tileBytes-1,0,&output) && !memcmp(&output,&before,sizeof(output)));
		assert(!PioneerQ38UnpackPanel(type,tile,tileBytes,type==8?2:16,&output) && !memcmp(&output,&before,sizeof(output)));
		assert(!PioneerQ38UnpackPanel(type,tile,tileBytes,0,reinterpret_cast<PioneerQ38Panel*>(tile)));
		unsigned d=type==14?208:0; tile[d*16]=1; tile[(d+1)*16]=0x7c;
		assert(!PioneerQ38UnpackPanel(type,tile,tileBytes,0,&output) && !memcmp(&output,&before,sizeof(output)));
		assert(!PioneerQ38FastPanel(type,tile,tileBytes,0,&output) && !memcmp(&output,&before,sizeof(output)));
	}
	unsigned real=0;
	if (argc==2) {
		FILE* f=fopen(argv[1],"rb"); assert(f); PioneerQwen38Gguf model; assert(model.Open(f));
		for (unsigned tensor=0;tensor<model.Tensors().size();tensor++) {
			const auto& t=model.Tensors()[tensor]; if (!t.type) continue;
			unsigned block=PioneerQ38BlockBytes(t.type),rowBytes=unsigned(t.bytes/t.outputs);
			for (unsigned end=0;end<2;end++) {
				alignas(4) uint8_t raw[16*210]={},tile[3392]={}; unsigned first=end?unsigned(t.outputs)-16:0;
				for (unsigned row=0;row<16;row++) {
					assert(model.ReadPacked(tensor,uint64_t(first+row)*rowBytes+(end?rowBytes-block:0),block,raw+row*block,block));
					for (unsigned byte=0;byte<block;byte++) tile[byte*16+row]=raw[row*block+byte];
				}
				Compare(t.type,tile,raw,block); real++;
			}
		}
		assert(real==996 && model.Stable()); fclose(f);
	}
	printf("PASS: lossless16x16 integer-overlay Q4_K/Q6_K/Q8_0 panels,ALL16777216 half/factor combinations,384 random tiles,%u real edge tiles,-128/subnormals/affine minima/alias+failure atomicity;OFFLINE not GPU inference\n",real);
}
