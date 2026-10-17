/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RESIDENT_TENSOR_H
#define PIONEER_RESIDENT_TENSOR_H
#include "../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeMatvecImage.h"
#include <math.h>
#include <string.h>
#include <vector>
#include "PioneerFloat16.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ8CompactLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Unpack.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ8NativeF32Layout.h"

static inline uint16_t PioneerResidentBf16(float value)
{
	uint32_t bits; memcpy(&bits,&value,4);
	if ((bits&0x7f800000)==0) return (bits>>16)&0x8000;
	bits+=0x7fff+((bits>>16)&1); return uint16_t(bits>>16);
}
static inline bool PioneerResidentTensorShape(unsigned inputs,unsigned outputs)
{
	return PioneerMatvecShape(inputs,16) && outputs>=16 && outputs<=262144 && !(outputs&15);
}
static inline uint64_t PioneerResidentTensorBytes(unsigned inputs,unsigned outputs)
{
	return PioneerResidentTensorShape(inputs,outputs)?uint64_t(inputs/16)*(outputs/16)*576:0;
}

// Prefer a synchronous bounded span when the transport exposes it. Legacy
// adapters retain their exact page-at-a-time behaviour, including failure
// boundaries. Neither path queues data beyond this call.
template<class IO> auto PioneerTensorUploadLimit(const IO& io,int)->decltype(io.UploadPageLimit()) { return io.UploadPageLimit(); }
template<class IO> unsigned PioneerTensorUploadLimit(const IO&,long) { return 4; }
template<class IO>
auto PioneerTensorUpload(IO& io,uint64_t offset,const uint32_t* data,unsigned pages,uint64_t& uploaded,int)
	-> decltype(io.UploadPages(offset,data,pages),bool())
{
	if (!io.UploadPages(offset,data,pages)) return false;
	uploaded+=pages; return true;
}
template<class IO>
bool PioneerTensorUpload(IO& io,uint64_t offset,const uint32_t* data,unsigned pages,uint64_t& uploaded,long)
{
	for (unsigned page=0;page<pages;page++) {
		if (!io.Upload(offset+page*4096,data+page*1024)) return false;
		uploaded++;
	}
	return true;
}

// Immutable model weights only. An identity must stay alive and its data
// unchanged until this cache is destroyed. Never use with trainable tensors.
// IO performs synchronous, fail-closed owned-page uploads and real matvecs.
template<class IO> class PioneerResidentTensorCache {
	struct Entry { const void* identity; unsigned encoding,inputs,outputs; uint64_t offset; };
	IO& io;
	uint64_t capacity,next=0;
	bool failed=false;
	const bool fp16;
	const bool compact;
	const bool native;
	const bool f32Scales;
	const bool wholeProjection;
	unsigned mode=0; // one immutable packing/operation format per cache
	std::vector<Entry> entries;
public:
	uint64_t uploadedPages=0,matvecCalls=0;
	PioneerResidentTensorCache(IO& transport,uint64_t bytes,bool half=false,bool rawBytes=false,bool hardwareUnpack=false,bool exactScales=false,bool whole=false):io(transport),capacity(bytes),fp16(half),compact(rawBytes),native(hardwareUnpack),f32Scales(exactScales),wholeProjection(whole) { failed=(native && (!compact || fp16)) || (f32Scales && !native) || (whole && !exactScales); }
	bool Failed() const { return failed; }
	size_t Tensors() const { return entries.size(); }
	uint64_t Bytes() const { return next; }
	template<class Decode> bool Weights(const void* identity,unsigned encoding,unsigned inputs,unsigned outputs,
		Decode decode,uint64_t& offset)
	{
		if (failed || mode==2 || compact) { failed=true; return false; }
		mode=1;
		for (const auto& e:entries) if (e.identity==identity) {
			if (e.encoding!=encoding || e.inputs!=inputs || e.outputs!=outputs) { failed=true; return false; }
			offset=e.offset; return true;
		}
		uint64_t bytes=PioneerResidentTensorBytes(inputs,outputs);
		if (!identity || !bytes || !PioneerResidentCapacity(capacity) || next>capacity || bytes>capacity-next) {
			failed=true; return false;
		}
		const unsigned tileBytes=unsigned(PioneerMatvecWeightBytes(inputs,16));
		std::vector<float> rows(size_t(inputs)*16);
		std::vector<uint32_t> packed(tileBytes/4);
		for (unsigned row=0;row<outputs;row+=16) {
			for (unsigned r=0;r<16;r++) if (!decode(row+r,rows.data()+size_t(r)*inputs)) { failed=true; return false; }
			memset(packed.data(),0,tileBytes);
			for (unsigned r=0;r<16;r++) for (unsigned k=0;k<inputs;k++) {
				float value=rows[size_t(r)*inputs+k]; uint16_t bf16=fp16?PioneerFloat16(value):PioneerResidentBf16(value);
				uint16_t mask=fp16?0x7c00:0x7f80;
				if (!isfinite(value) || (bf16&mask)==mask) { failed=true; return false; }
				uint64_t byte=PioneerMatvecWeightElement(inputs,k,r);
				packed[size_t(byte/4)]|=uint32_t(bf16)<<(8*(byte&3));
			}
			for (unsigned page=0;page<tileBytes;page+=4096) {
				if (!io.Upload(next+uint64_t(row/16)*tileBytes+page,packed.data()+page/4)) { failed=true; return false; }
				uploadedPages++;
			}
		}
		offset=next; next+=bytes; entries.push_back({identity,encoding,inputs,outputs,offset}); return true;
	}
	bool Multiply(uint64_t offset,unsigned inputs,unsigned outputs,const float* input,float* output)
	{
		if (failed || mode!=1) { failed=true; return false; }
		bool found=false;
		for (const auto& e:entries) found |= e.offset==offset && e.inputs==inputs && e.outputs==outputs;
		if (!found || !input || !output) { failed=true; return false; }
		uint32_t vector[TT_RUNTIME_MATVEC_INPUTS_MAX/2]={};
		for (unsigned k=0;k<inputs;k++) {
			uint16_t bf16=fp16?PioneerFloat16(input[k]):PioneerResidentBf16(input[k]);
			uint16_t mask=fp16?0x7c00:0x7f80;
			if (!isfinite(input[k]) || (bf16&mask)==mask) { failed=true; return false; }
			vector[k/2]|=uint32_t(bf16)<<(16*(k&1));
		}
		uint32_t result[TT_RUNTIME_MATVEC_OUTPUTS_MAX];
		for (unsigned row=0;row<outputs;row+=TT_RUNTIME_MATVEC_OUTPUTS_MAX) {
			unsigned n=outputs-row; if (n>TT_RUNTIME_MATVEC_OUTPUTS_MAX) n=TT_RUNTIME_MATVEC_OUTPUTS_MAX;
			uint64_t position=offset+uint64_t(row/16)*(inputs/16)*576;
			if (!io.Matvec(position,inputs,n,vector,result)) { failed=true; return false; }
			matvecCalls++;
			for (unsigned r=0;r<n;r++) {
				float value; memcpy(&value,result+r,4);
				if (!isfinite(value)) { failed=true; return false; }
				output[row+r]=value;
			}
		}
		return true;
	}
	// Lossless Q8_0 path: preserve original signed bytes and FP16 block
	// scales; do not dequantize or re-quantize immutable learned weights.
	// DecodeBlock supplies exactly the original 34-byte GGML block.
	template<class DecodeBlock> bool Q8Weights(const void* identity,unsigned encoding,unsigned inputs,unsigned outputs,
		DecodeBlock decode,uint64_t& offset)
	{
		if (failed || fp16 || mode==1) { failed=true; return false; }
		mode=2;
		for (const auto& e:entries) if (e.identity==identity) {
			if (e.encoding!=encoding || e.inputs!=inputs || e.outputs!=outputs) { failed=true; return false; }
			offset=e.offset; return true;
		}
		uint64_t bytes=compact?PioneerQ8CompactWeightBytes(inputs,outputs):PioneerResidentTensorBytes(inputs,outputs);
		if (!identity || !bytes || !PioneerResidentCapacity(capacity) || next>capacity || bytes>capacity-next) { failed=true; return false; }
		const unsigned rowStep=compact?32:16;
		const unsigned tileBytes=unsigned(compact?PioneerQ8CompactWeightBytes(inputs,rowStep):PioneerMatvecWeightBytes(inputs,rowStep));
		unsigned uploadPages=PioneerTensorUploadLimit(io,0);
		if (!uploadPages || uploadPages>256) { failed=true; return false; }
		unsigned tilesPerBatch=uploadPages*4096/tileBytes; if (!tilesPerBatch) tilesPerBatch=1;
		std::vector<uint32_t> packed(size_t(tileBytes/4)*tilesPerBatch);
		for (unsigned row=0;row<outputs;) {
			unsigned tiles=(outputs-row)/rowStep; if (tiles>tilesPerBatch) tiles=tilesPerBatch;
			unsigned batchBytes=tiles*tileBytes; memset(packed.data(),0,batchBytes);
			for (unsigned tile=0;tile<tiles;tile++) for (unsigned r=0;r<rowStep;r++) for (unsigned block=0;block<inputs/32;block++) {
				uint8_t data[34];
				if (!decode(row+tile*rowStep+r,block,data)) { failed=true; return false; }
				uint16_t d=uint16_t(data[0])|(uint16_t(data[1])<<8);
				if ((d&0x7c00)==0x7c00) { failed=true; return false; }
				for (unsigned i=0;i<32;i++) {
					int q=data[2+i]; if (q>=128) q-=256;
					uint8_t encoded=data[2+i];
					if (native && !PioneerInt8UnpackEncode(q,encoded)) { failed=true; return false; }
					uint64_t byte=f32Scales?PioneerQ8NativeF32ValueByte(inputs,block*32+i,r):compact?PioneerQ8CompactValueByte(inputs,block*32+i,r):PioneerMatvecWeightElement(inputs,block*32+i,r);
					packed[size_t(tile)*tileBytes/4+size_t(byte/4)]|=uint32_t(compact?uint16_t(encoded):PioneerInt8Operand(q))<<(8*(byte&3));
				}
				uint64_t byte=f32Scales?PioneerQ8NativeF32ScaleByte(inputs,block,r):compact?PioneerQ8CompactScaleByte(inputs,block,r):uint64_t(block*2+1)*576+544+r*2;
				if (f32Scales) packed[size_t(tile)*tileBytes/4+size_t(byte/4)]=PioneerQ8HalfToFloatBits(d);
				else packed[size_t(tile)*tileBytes/4+size_t(byte/4)]|=uint32_t(d)<<(8*(byte&3));
			}
			for (unsigned page=0;page<batchBytes;page+=uploadPages*4096) {
				unsigned pages=(batchBytes-page)/4096; if (pages>uploadPages) pages=uploadPages;
				if (!PioneerTensorUpload(io,next+uint64_t(row/rowStep)*tileBytes+page,packed.data()+page/4,pages,uploadedPages,0)) { failed=true; return false; }
			}
			row+=tiles*rowStep;
		}
		offset=next; next+=bytes; entries.push_back({identity,encoding,inputs,outputs,offset}); return true;
	}
	// Activation blocks are produced by the pinned GGML reference quantizer,
	// not by an approximate local rounding implementation.
	bool Q8Ffn(uint64_t gate,uint64_t up,uint64_t down,const uint8_t* blocks,float* output)
	{
		if (failed || fp16 || mode!=2 || !wholeProjection || !f32Scales || !blocks || !output) { failed=true; return false; }
		bool found[3]={}; const uint64_t offsets[]={gate,up,down};
		for (const auto& e:entries) for (unsigned i=0;i<3;i++) found[i] |= e.offset==offsets[i]
			&& e.inputs==(i==2?3072U:1024U) && e.outputs==(i==2?1024U:3072U);
		if (!found[0] || !found[1] || !found[2]) { failed=true; return false; }
		uint32_t vector[512]={},scales[32],result[1024];
		for (unsigned b=0;b<32;b++) {
			const uint8_t* data=blocks+b*34; uint16_t d=uint16_t(data[0])|(uint16_t(data[1])<<8);
			if ((d&0x7c00)==0x7c00) { failed=true; return false; }
			scales[b]=PioneerQ8HalfToFloatBits(d);
			for (unsigned i=0;i<32;i++) { int q=data[2+i]; if (q>=128) q-=256; vector[b*16+i/2]|=uint32_t(PioneerInt8Operand(q))<<(16*(i&1)); }
		}
		if (!io.LayerFfn(gate,up,down,vector,scales,result)) { failed=true; return false; }
		for (unsigned i=0;i<1024;i++) if ((result[i]&0x7f800000)==0x7f800000) { failed=true; return false; }
		memcpy(output,result,4096); matvecCalls+=3; return true;
	}
	bool Q8Multiply(uint64_t offset,unsigned inputs,unsigned outputs,const uint8_t* blocks,float* output)
	{
		if (failed || fp16 || mode!=2) { failed=true; return false; }
		bool found=false; for (const auto& e:entries) found |= e.offset==offset && e.inputs==inputs && e.outputs==outputs;
		if (!found || !blocks || !output) { failed=true; return false; }
		uint32_t vector[TT_RUNTIME_MATVEC_INPUTS_MAX/2]={},scales[TT_RUNTIME_MATVEC_INPUTS_MAX/32]={};
		for (unsigned block=0;block<inputs/32;block++) {
			const uint8_t* data=blocks+block*34;
			uint16_t half=uint16_t(data[0])|(uint16_t(data[1])<<8);
			if ((half&0x7c00)==0x7c00) { failed=true; return false; }
			// Exact half -> float conversion, including subnormal dX.
			uint32_t sign=uint32_t(half&0x8000)<<16,exp=(half>>10)&31,m=half&1023;
			if (exp) scales[block]=sign|((exp+112)<<23)|(m<<13);
			else if (!m) scales[block]=sign;
			else { unsigned shifts=0; while (!(m&1024)) { m<<=1; shifts++; } scales[block]=sign|((113-shifts)<<23)|((m&1023)<<13); }
			for (unsigned i=0;i<32;i++) {
				int q=data[2+i]; if (q>=128) q-=256; unsigned k=block*32+i;
				vector[k/2]|=uint32_t(PioneerInt8Operand(q))<<(16*(k&1));
			}
		}
		uint32_t local[TT_RUNTIME_MATVEC_OUTPUTS_MAX];
		std::vector<uint32_t> large(wholeProjection?outputs:0);
		uint32_t* result=wholeProjection?large.data():local;
		unsigned step=wholeProjection?outputs:unsigned(TT_RUNTIME_MATVEC_OUTPUTS_MAX);
		for (unsigned row=0;row<outputs;row+=step) {
			unsigned n=outputs-row; if (n>step) n=step;
			uint64_t position=offset+uint64_t(row/16)*(inputs/(compact?32:16))*576;
			if (!io.Q8Matvec(position,inputs,n,vector,scales,result)) { failed=true; return false; }
			matvecCalls++;
			for (unsigned r=0;r<n;r++) { float value; memcpy(&value,result+r,4); if (!isfinite(value)) { failed=true; return false; } output[row+r]=value; }
		}
		return true;
	}
};
#endif
