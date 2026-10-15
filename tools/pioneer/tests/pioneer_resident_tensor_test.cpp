/* SPDX-License-Identifier: MIT */
#include <cassert>
#include "../PioneerResidentTensor.h"
static float Float(uint32_t bits) { float f; memcpy(&f,&bits,4); return f; }
static float Weight(unsigned row,unsigned k) { return float(int((row*7+k*13)%31)-15)/32; }
struct TensorIO {
	std::vector<uint32_t> storage;
	unsigned uploads=0,calls=0,uploadFail=0,callFail=0;
	bool nonfinite=false;
	bool fp16=false;
	TensorIO():storage(16*1024*1024/4,0xfeedface) {}
	bool Upload(uint64_t offset,const uint32_t* data) {
		assert(!(offset%4096) && offset/4+1024<=storage.size());
		if (++uploads==uploadFail) return false;
		memcpy(storage.data()+offset/4,data,4096); return true;
	}
	bool Matvec(uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* x,uint32_t* y) {
		assert(PioneerMatvecShape(inputs,outputs) && !(offset%4096));
		if (++calls==callFail) return false;
		for (unsigned row=0;row<outputs;row++) {
			float sum=0;
			for (unsigned k=0;k<inputs;k++) {
				// Independent byte-reader of the documented on-card tile
				// layout, not the packer's indexing helper.
				uint64_t pos=offset+((uint64_t(row/16)*(inputs/16)+k/16)*576)+16+(k%16)*32+(row%16)*2;
				unsigned bf16=(storage[size_t(pos/4)]>>(8*(pos&3)))&65535;
				unsigned v=(x[k/2]>>(16*(k&1)))&65535;
				sum+=fp16?PioneerFloat16Value(uint16_t(bf16)&~1U)*PioneerFloat16Value(uint16_t(v)):Float(bf16<<16)*Float(v<<16);
			}
			if (nonfinite) sum=INFINITY;
			memcpy(y+row,&sum,4);
		}
		return true;
	}
};
int main() {
	TensorIO io; PioneerResidentTensorCache<TensorIO> cache(io,16*1024*1024);
	int first=0,second=0; uint64_t offset=0;
	auto decode=[](unsigned row,float* data) { for (unsigned k=0;k<1024;k++) data[k]=Weight(row,k); return true; };
	assert(cache.Weights(&first,8,1024,32,decode,offset) && offset==0 && io.uploads==18);
	assert(cache.Weights(&first,8,1024,32,decode,offset) && io.uploads==18 && cache.Tensors()==1);
	float input[1024],output[4112]={};
	for (unsigned trial=0;trial<2;trial++) {
		for (unsigned k=0;k<1024;k++) input[k]=float(int((k*3+trial*7)%17)-8)/16;
		assert(cache.Multiply(offset,1024,32,input,output));
		for (unsigned row=0;row<32;row++) {
			float reference=0; for (unsigned k=0;k<1024;k++) reference+=Weight(row,k)*input[k];
			assert(output[row]==reference);
		}
	}
	assert(io.uploads==18 && io.calls==2);
	assert(cache.Weights(&second,8,1024,4112,decode,offset) && offset==73728 && cache.Tensors()==2);
	assert(cache.Multiply(offset,1024,4112,input,output) && io.calls==4);
	for (unsigned row=0;row<4112;row++) {
		float reference=0; for (unsigned k=0;k<1024;k++) reference+=Weight(row,k)*input[k];
		assert(output[row]==reference);
	}
	assert(cache.uploadedPages==cache.Bytes()/4096 && cache.matvecCalls==4);
	// Identity/shape changes cannot silently reuse stale resident bytes.
	assert(!cache.Weights(&first,8,1024,16,decode,offset) && cache.Failed());
	unsigned calls=io.calls; assert(!cache.Multiply(0,1024,32,input,output) && calls==io.calls);
	for (unsigned fault=0;fault<7;fault++) {
		TensorIO trial; PioneerResidentTensorCache<TensorIO> c(trial,fault==5?4096:16*1024*1024);
		trial.uploadFail=fault==0?2:0;
		bool weights=c.Weights(&first,8,1024,32,[&](unsigned row,float* data) {
			decode(row,data); if (fault==1) data[0]=NAN; return fault!=2;
		},offset);
		if (fault==0 || fault==1 || fault==2 || fault==5) assert(!weights && c.Failed() && !trial.calls);
		else {
			assert(weights); trial.callFail=fault==3?1:0; trial.nonfinite=fault==4;
			if (fault==6) input[0]=INFINITY;
			assert(!c.Multiply(offset,1024,32,input,output) && c.Failed());
			if (fault==6) assert(!trial.calls);
		}
		unsigned uploads=trial.uploads;
		assert(!c.Weights(&second,8,1024,32,decode,offset) && trial.uploads==uploads);
	}
	assert(!PioneerResidentTensorShape(512,16) && !PioneerResidentTensorShape(1024,17));
	{
		TensorIO half; half.fp16=true; PioneerResidentTensorCache<TensorIO> c(half,16*1024*1024,true);
		auto fractional=[](unsigned row,float* data) { for (unsigned k=0;k<1024;k++) data[k]=Weight(row,k)/3.14159f; return true; };
		assert(c.Weights(&first,8,1024,32,fractional,offset));
		for (unsigned trial=0;trial<2;trial++) {
			for (unsigned k=0;k<1024;k++) input[k]=float(int((k*3+trial*7)%17)-8)/17;
			assert(c.Multiply(offset,1024,32,input,output));
			for (unsigned row=0;row<32;row++) {
				float reference=0;
				for (unsigned k=0;k<1024;k++) reference+=PioneerFloat16Value(PioneerFloat16(Weight(row,k)/3.14159f)&~1U)*PioneerFloat16Value(PioneerFloat16(input[k]));
				assert(output[row]==reference);
			}
		}
		assert(half.uploads==18 && half.calls==2 && c.Tensors()==1);
		input[0]=100000; assert(!c.Multiply(offset,1024,32,input,output) && half.calls==2 && c.Failed());
	}
	{
		TensorIO half; PioneerResidentTensorCache<TensorIO> c(half,16*1024*1024,true);
		assert(!c.Weights(&first,8,1024,32,[](unsigned,float* data) { for (unsigned k=0;k<1024;k++) data[k]=100000; return true; },offset));
		assert(c.Failed() && !half.calls && !half.uploads);
	}
	printf("PASS: immutable tensor packing/cache, changed vectors, multi-call classifier chunks, capacity/nonfinite/transport failures; NO hardware\n");
}
