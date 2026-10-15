/* SPDX-License-Identifier: MIT */
#include "../TenstorrentGgufQuant.h"
#include <cassert>
#include <cstdio>
int main()
{
	for(unsigned half=0;half<65536;half++) {
		unsigned exponent=(half>>10)&31,mantissa=half&1023;
		if(exponent==31) continue;
		float reference=exponent==0 ? ldexpf(float(mantissa),-24) : ldexpf(float(1024+mantissa),int(exponent)-25);
		if(half&0x8000) reference=-reference;
		assert(PioneerHalfFloat(half)==reference);
	}
	for(unsigned trial=0;trial<64;trial++) {
		uint8_t block[144]={}; block[1]=0x3c; block[3]=0x38; // d=1, dmin=0.5
		unsigned scales[8],mins[8];
		for(unsigned g=0;g<8;g++) { scales[g]=(g*13+trial)%64; mins[g]=(g*17+trial*3)%64; }
		// Independent encoder works bit-by-bit, rather than mirroring decoder masks.
		for(unsigned g=0;g<8;g++) for(unsigned bit=0;bit<6;bit++) {
			unsigned scaleByte=bit<4 || g<4 ? (g<4 ? 4+g : 8+g) : g;
			unsigned scaleBit=bit<4 || g<4 ? bit : bit+2;
			unsigned minByte=g<4 ? 8+g : bit<4 ? 8+g : 4+g;
			unsigned minBit=g<4 ? bit : bit<4 ? bit+4 : bit+2;
			block[scaleByte]|=((scales[g]>>bit)&1)<<scaleBit;
			block[minByte]|=((mins[g]>>bit)&1)<<minBit;
		}
		for(unsigned i=0;i<256;i++) {
			unsigned q=(i*7+trial)%16;
			unsigned byte=16+(i/64)*32+i%32,shift=(i%64>=32)?4:0;
			block[byte]|=q<<shift;
		}
		float out[256]; assert(PioneerDecodeQ4K(block,out));
		for(unsigned i=0;i<256;i++) assert(out[i]==float(scales[i/32]*((i*7+trial)%16))-0.5f*mins[i/32]);
		block[0]=1; block[1]=0x7c; assert(!PioneerDecodeQ4K(block,out));
	}
	for(unsigned trial=0;trial<256;trial++) {
		uint8_t q8[34]={},q6[210]={}; q8[1]=0x3c; q6[209]=0x3c;
		for(unsigned i=0;i<32;i++) q8[2+i]=(i*17+trial)%256;
		for(unsigned g=0;g<16;g++) q6[192+g]=(g*29+trial)%256;
		for(unsigned half=0;half<2;half++) for(unsigned lane=0;lane<32;lane++) {
			unsigned quant[4];
			for(unsigned plane=0;plane<4;plane++) quant[plane]=((half*128+plane*32+lane)*7+trial)%64;
			q6[half*64+lane]=(quant[0]&15)|((quant[2]&15)<<4);
			q6[half*64+lane+32]=(quant[1]&15)|((quant[3]&15)<<4);
			for(unsigned plane=0;plane<4;plane++) q6[128+half*32+lane]|=(quant[plane]>>4)<<(plane*2);
		}
		float a[32],b[256]; assert(PioneerDecodeQ8_0(q8,a) && PioneerDecodeQ6K(q6,b));
		for(unsigned i=0;i<32;i++) { int q=(i*17+trial)%256; if(q>=128) q-=256; assert(a[i]==q); }
		for(unsigned i=0;i<256;i++) {
			int scale=(i/16*29+trial)%256; if(scale>=128) scale-=256;
			assert(b[i]==scale*(int((i*7+trial)%64)-32));
		}
		q8[1]=0x7c; q6[209]=0x7c;
		assert(!PioneerDecodeQ8_0(q8,a) && !PioneerDecodeQ6K(q6,b));
	}
	printf("PASS: finite FP16 encodings, 16384 Q4_K, 8192 Q8_0 and 65536 Q6_K values from independent fixtures; no model/device access\n");
}
