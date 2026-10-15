/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RESIDENT_TENSOR_H
#define PIONEER_RESIDENT_TENSOR_H
#include "../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeMatvecImage.h"
#include <math.h>
#include <string.h>
#include <vector>
#include "PioneerFloat16.h"

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

// Immutable model weights only. An identity must stay alive and its data
// unchanged until this cache is destroyed. Never use with trainable tensors.
// IO performs synchronous, fail-closed owned-page uploads and real matvecs.
template<class IO> class PioneerResidentTensorCache {
	struct Entry { const void* identity; unsigned encoding,inputs,outputs; uint64_t offset; };
	IO& io;
	uint64_t capacity,next=0;
	bool failed=false;
	const bool fp16;
	std::vector<Entry> entries;
public:
	uint64_t uploadedPages=0,matvecCalls=0;
	PioneerResidentTensorCache(IO& transport,uint64_t bytes,bool half=false):io(transport),capacity(bytes),fp16(half) {}
	bool Failed() const { return failed; }
	size_t Tensors() const { return entries.size(); }
	uint64_t Bytes() const { return next; }
	template<class Decode> bool Weights(const void* identity,unsigned encoding,unsigned inputs,unsigned outputs,
		Decode decode,uint64_t& offset)
	{
		if (failed) return false;
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
		if (failed) return false;
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
};
#endif
