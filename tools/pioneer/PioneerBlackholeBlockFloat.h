/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_BLACKHOLE_BLOCK_FLOAT_H
#define PIONEER_BLACKHOLE_BLOCK_FLOAT_H
#if defined(PIONEER_ATTENTION_FREESTANDING) || defined(_KERNEL_MODE)
#error CPU-memory source-format codec only;not a shader or kernel capability
#endif
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

// Experimental NEW numerical representation,NOT lossless GGML Q4_K/Q6_K/Q8_0.
// Fixed32x32 row-major input -> four16x16 faces;64 shared exponent bytes
// precede face-major signed mantissas. B variants use the FP32/BF16 bias.
// Encoding follows the documented host convention:truncate exponent-align
// first,then round the remaining mantissa nearest-even and saturate WITHOUT
// raising the shared exponent. Negative zero and rounded zero become +0.
// Primary format reference (algorithm facts,not copied implementation):
// https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/impl/data_format/blockfloat_common.cpp
// This deliberately bounded codec accepts zero or finite FP32 normals when
// each nonzero16-value group's MAXIMUM exponent is8..254. Smaller normals
// in that group may quantize to zero. That includes finite dequantized
// HALF-based GGUF weights. Subnormal inputs,all-tiny nonzero groups and
// nonfinite values REFUSE instead of silently selecting another format.
// No file,model authentication,allocation,device IO,route,owner,launch,Work
// selection or performance permission exists here. Old source formats stay
// unchanged. A future uploader needs its OWN format/layout/lifetime binding.
class PioneerBlackholeBlockFloat {
public:
	 enum class Format : unsigned { Bfp4B=4, Bfp8B=8 };
	 static constexpr unsigned Elements=1024,ExponentBytes=64,MaxBytes=1088;
	 static constexpr unsigned Bytes(Format f) { return f==Format::Bfp4B?576:f==Format::Bfp8B?1088:0; }
private:
	 static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"IEEE FP32 source required");
	 static bool Separate(uintptr_t a,unsigned n,uintptr_t b,unsigned m) {
		const uintptr_t limit=~uintptr_t(0);
		return a<=limit-n && b<=limit-m && (a+n<=b || b+m<=a);
	 }
	 static uint32_t Bits(float value) { uint32_t bits;std::memcpy(&bits,&value,4);return bits; }
	 static float Float(uint32_t bits) { float value;std::memcpy(&value,&bits,4);return value; }
	 static bool Supported(uint32_t bits) {
		const unsigned exponent=(bits>>23)&255;
		return !(bits&0x7fffffffU) || (exponent>0 && exponent<255);
	 }
	 static unsigned At(unsigned face,unsigned row,unsigned column) {
		return ((face/2)*16+row)*32+(face%2)*16+column;
	 }
	 static unsigned Quantize(uint32_t bits,unsigned shared,unsigned magnitudeBits) {
		if(!(bits&0x7fffffffU))return 0;
		const unsigned shift=shared-((bits>>23)&255),discard=24-magnitudeBits;
		const uint32_t significand=0x800000U|(bits&0x7fffffU);
		const uint32_t aligned=shift>=32?0:significand>>shift;
		unsigned magnitude=aligned>>discard;
		const unsigned remainder=aligned&((1U<<discard)-1),half=1U<<(discard-1);
		if(remainder>half || (remainder==half && (magnitude&1)))magnitude++;
		const unsigned maximum=(1U<<magnitudeBits)-1;
		if(magnitude>maximum)magnitude=maximum;
		return magnitude?magnitude|((bits>>31)<<magnitudeBits):0;
	 }
public:
	 static bool PackTile(Format format,const float* input,unsigned count,uint8_t* output,unsigned capacity) {
		const unsigned bytes=Bytes(format);
		if(!bytes || !input || !output || count!=Elements || capacity<bytes
			|| (reinterpret_cast<uintptr_t>(input)%alignof(float))
			|| !Separate(reinterpret_cast<uintptr_t>(input),Elements*4,reinterpret_cast<uintptr_t>(output),bytes))return false;
		std::array<uint8_t,MaxBytes> stage{};
		const unsigned width=unsigned(format),magnitudeBits=width-1;
		for(unsigned face=0;face<4;face++)for(unsigned row=0;row<16;row++) {
			unsigned exponent=0;
			for(unsigned column=0;column<16;column++) {
				const uint32_t bits=Bits(input[At(face,row,column)]);
				if(!Supported(bits))return false;
				const unsigned e=(bits>>23)&255;if(e>exponent)exponent=e;
			}
			if(exponent && exponent<8)return false;
			const unsigned group=face*16+row;stage[group]=uint8_t(exponent);
			for(unsigned column=0;column<16;column++) {
				const unsigned q=Quantize(Bits(input[At(face,row,column)]),exponent,magnitudeBits);
				const unsigned element=group*16+column;
				if(width==8)stage[ExponentBytes+element]=uint8_t(q);
				else stage[ExponentBytes+element/2]|=uint8_t(q<<((element&1)*4));
			}
		}
		std::memcpy(output,stage.data(),bytes);return true;
	 }
	 // Strict CPU inspection of OUR canonical weight tiles,not a hardware
	 // decoder/quality oracle. Publish ALL1024 values only after validation.
	 static bool UnpackTile(Format format,const uint8_t* input,unsigned bytes,float* output,unsigned capacity) {
		if(!Bytes(format) || !input || !output || bytes!=Bytes(format) || capacity<Elements
			|| (reinterpret_cast<uintptr_t>(output)%alignof(float))
			|| !Separate(reinterpret_cast<uintptr_t>(input),bytes,reinterpret_cast<uintptr_t>(output),Elements*4))return false;
		std::array<float,Elements> stage{};
		const unsigned width=unsigned(format),magnitudeBits=width-1,top=1U<<(magnitudeBits-1);
		for(unsigned face=0;face<4;face++)for(unsigned row=0;row<16;row++) {
			const unsigned group=face*16+row,exponent=input[group];bool topPresent=false;
			if(exponent && (exponent<8 || exponent==255))return false;
			for(unsigned column=0;column<16;column++) {
				const unsigned element=group*16+column;
				const unsigned q=width==8?input[ExponentBytes+element]:
					(input[ExponentBytes+element/2]>>((element&1)*4))&15;
				unsigned magnitude=q&(top*2-1),shift=0;
				if(!magnitude) { if(q)return false;continue; } // canonical +0 only
				if(!exponent)return false;
				topPresent|=magnitude>=top;
				while(magnitude<top) { magnitude<<=1;shift++; }
				const uint32_t bits=((q>>magnitudeBits)<<31)|((exponent-shift)<<23)
					|((magnitude-top)<<(24-magnitudeBits));
				stage[At(face,row,column)]=Float(bits);
			}
			if(bool(exponent)!=topPresent)return false;
		}
		std::memcpy(output,stage.data(),Elements*4);return true;
	 }
};
#endif
