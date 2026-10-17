/* SPDX-License-Identifier: MIT */
#include <cassert>
#include "../PioneerResidentTensor.h"
#include "../TenstorrentGgufQuant.h"
static float Float(uint32_t bits) { float f; memcpy(&f,&bits,4); return f; }
static float Weight(unsigned row,unsigned k) { return float(int((row*7+k*13)%31)-15)/32; }
struct TensorIO {
	std::vector<uint32_t> storage;
	unsigned uploads=0,calls=0,uploadFail=0,callFail=0;
	bool nonfinite=false;
	bool fp16=false;
	bool compact=false;
	bool native=false;
	bool exact=false;
	bool whole=false;
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
	bool Q8Matvec(uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* x,const uint32_t* scales,uint32_t* y) {
		assert((whole?PioneerQ8CompactTensorShape(inputs,outputs):PioneerMatvecShape(inputs,outputs)) && !(offset%4096)); if (++calls==callFail) return false;
		for (unsigned row=0;row<outputs;row++) {
			float sum=0;
			for (unsigned block=0;block<inputs/32;block++) {
				int dot=0;
				for (unsigned i=0;i<32;i++) {
					unsigned k=block*32+i; uint64_t p=offset+(compact?((uint64_t(row/16)*(inputs/32)+block)*576+(exact?64:32)+(k%32)*16+row%16):((uint64_t(row/16)*(inputs/16)+k/16)*576+16+(k%16)*32+(row%16)*2));
					uint16_t a=uint16_t(storage[size_t(p/4)]>>(8*(p&3))),b=uint16_t(x[k/2]>>(16*(k&1)));
					if (compact) { int q=native?int(a&127)*((a&128)?-1:1):int(a&255); if (!native && q>=128) q-=256; a=PioneerInt8Operand(q); }
					assert(PioneerInt8OperandValid(a) && PioneerInt8OperandValid(b));
					dot+=int(a&1023)*((a&0x8000)?-1:1)*int(b&1023)*((b&0x8000)?-1:1);
				}
				uint64_t p=offset+(compact?((uint64_t(row/16)*(inputs/32)+block)*576+(row%16)*(exact?4:2)):((uint64_t(row/16)*(inputs/16)+block*2+1)*576+544+(row%16)*2));
				float dw=PioneerHalfFloat(uint16_t(storage[size_t(p/4)]>>(8*(p&3))));
				if (exact) { dw=Float(storage[size_t(p/4)]); sum=fmaf(float(dot),dw*Float(scales[block]),sum); }
				else sum+=float(dot)*(dw*Float(scales[block]));
			}
			if (nonfinite) sum=INFINITY;
			memcpy(y+row,&sum,4);
		}
		return true;
	}
};
struct BatchTensorIO:TensorIO {
	unsigned spans=0,spanFail=0,limit=4;
	unsigned UploadPageLimit() const { return limit; }
	std::vector<unsigned> sizes;
	bool UploadPages(uint64_t offset,const uint32_t* data,unsigned pages) {
		assert(pages>=1 && pages<=limit); sizes.push_back(pages);
		if (++spans==spanFail) return false;
		for (unsigned p=0;p<pages;p++) if (!Upload(offset+p*4096,data+p*1024)) return false;
		return true;
	}
};
static uint16_t QScale(unsigned r,unsigned b) { const uint16_t h[]={0x2801,0xb503,0,0x8000,1,0x83ff}; return h[(r+b)%6]; }
static void QBlock(unsigned r,unsigned b,uint8_t* out,bool native=false) {
	uint16_t h=QScale(r,b); out[0]=uint8_t(h); out[1]=uint8_t(h>>8);
	for (unsigned i=0;i<32;i++) out[2+i]=native?uint8_t(int((r*37+b*13+i*17)%255)-127):uint8_t(r*37+b*13+i*17);
}
static float QReference(unsigned row,unsigned inputs,const uint8_t* x,bool native=false,bool exact=false) {
	float sum=0;
	for (unsigned b=0;b<inputs/32;b++) {
		uint8_t w[34]; QBlock(row,b,w,native); const uint8_t* v=x+b*34; int dot=0;
		for (unsigned i=0;i<32;i++) { int a=w[2+i],q=v[2+i]; if (a>=128) a-=256; if (q>=128) q-=256; dot+=a*q; }
		float dw=PioneerHalfFloat(QScale(row,b)),dx=PioneerHalfFloat(uint16_t(v[0])|(uint16_t(v[1])<<8));
		if (exact) sum=fmaf(float(dot),dw*dx,sum); else sum+=float(dot)*(dw*dx);
	}
	return sum;
}
int main() {
	for (unsigned k:{1024U,3072U,6144U}) {
		unsigned rows=k==1024?3072:1024; int identity=0; uint64_t a=0,b=0;
		BatchTensorIO ordinary,span; span.limit=256;
		PioneerResidentTensorCache<BatchTensorIO> ca(ordinary,16*1024*1024,false,true,true,true,true),cb(span,16*1024*1024,false,true,true,true,true);
		auto decode=[](unsigned r,unsigned block,uint8_t* out) { QBlock(r,block,out,true); return true; };
		assert(ca.Q8Weights(&identity,8,k,rows,decode,a) && cb.Q8Weights(&identity,8,k,rows,decode,b));
		assert(a==b && ca.Bytes()==cb.Bytes() && ordinary.storage==span.storage && ordinary.uploads==span.uploads && span.spans<ordinary.spans);
		unsigned spans=span.spans; assert(cb.Q8Weights(&identity,8,k,rows,decode,b) && span.spans==spans);
	}
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
	for (unsigned format=0;format<4;format++) for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		bool compact=format!=0,native=format>=2,exact=format==3;
		TensorIO quant; quant.compact=compact; quant.native=native; quant.exact=exact; PioneerResidentTensorCache<TensorIO> c(quant,16*1024*1024,false,compact,native,exact);
		unsigned rows=k==1024?(compact?4128:4112):32;
		assert(c.Q8Weights(&first,8,k,rows,[&](unsigned r,unsigned b,uint8_t* out) { QBlock(r,b,out,native); return true; },offset));
		unsigned uploads=quant.uploads;
		assert(c.Q8Weights(&first,8,k,rows,[](unsigned,unsigned,uint8_t*) { assert(false); return false; },offset) && quant.uploads==uploads);
		std::vector<uint8_t> x(k/32*34);
		std::vector<float> y(rows);
		for (unsigned t=0;t<2;t++) {
			for (unsigned b=0;b<k/32;b++) QBlock(t+3,b,x.data()+b*34);
			assert(c.Q8Multiply(offset,k,rows,x.data(),y.data()));
			for (unsigned row=0;row<rows;row++) assert(y[row]==QReference(row,k,x.data(),native,exact));
		}
		assert(c.uploadedPages==c.Bytes()/4096 && c.matvecCalls==(rows>4096?4:2) && quant.uploads==uploads);
		unsigned calls=quant.calls; x[0]=0; x[1]=0x7c;
		assert(!c.Q8Multiply(offset,k,rows,x.data(),y.data()) && c.Failed() && quant.calls==calls);
	}
	for (unsigned fault=0;fault<3;fault++) {
		TensorIO quant; quant.compact=quant.native=true;
		PioneerResidentTensorCache<TensorIO> c(quant,16*1024*1024,fault==2,fault!=1,true);
		assert(!c.Q8Weights(&first,8,1024,32,[&](unsigned r,unsigned b,uint8_t* out) { QBlock(r,b,out,true); out[2]=128; return true; },offset));
		assert(c.Failed() && !quant.uploads && !quant.calls);
		assert(!c.Q8Multiply(0,1024,32,NULL,output) && !quant.calls);
	}
	// Whole learned projection is explicit: same packed bytes and exact
	// sums, but an8192-row matrix makes ONE transport call instead of two.
	{
		TensorIO quant; quant.compact=quant.native=quant.exact=quant.whole=true;
		PioneerResidentTensorCache<TensorIO> c(quant,16*1024*1024,false,true,true,true,true);
		assert(c.Q8Weights(&first,8,1024,8192,[&](unsigned r,unsigned b,uint8_t* out) { QBlock(r,b,out,true); return true; },offset));
		std::vector<uint8_t> x(32*34); for (unsigned b=0;b<32;b++) QBlock(4,b,x.data()+b*34);
		std::vector<float> y(8192);
		assert(c.Q8Multiply(offset,1024,8192,x.data(),y.data()) && quant.calls==1 && c.matvecCalls==1);
		for (unsigned row=0;row<8192;row++) assert(y[row]==QReference(row,1024,x.data(),true,true));
		quant.callFail=2; assert(!c.Q8Multiply(offset,1024,8192,x.data(),y.data()) && c.Failed() && quant.calls==2);
	}
	for (unsigned fault=0;fault<3;fault++) {
		TensorIO quant; PioneerResidentTensorCache<TensorIO> c(quant,16*1024*1024,false,true,fault!=0,true);
		assert(!c.Q8Weights(&first,8,1024,32,[&](unsigned r,unsigned b,uint8_t* out) {
			QBlock(r,b,out,true); if (fault==1) out[2]=128; if (fault==2) { out[0]=0; out[1]=0x7c; } return true;
		},offset));
		assert(c.Failed() && !quant.uploads && !quant.calls);
	}
	for (bool compact:{false,true}) for (unsigned fault=0;fault<6;fault++) {
		TensorIO quant; quant.compact=compact; PioneerResidentTensorCache<TensorIO> c(quant,16*1024*1024,false,compact);
		quant.uploadFail=fault==0?2:0;
		bool okay=c.Q8Weights(&first,8,1024,32,[&](unsigned r,unsigned b,uint8_t* out) {
			QBlock(r,b,out); if (fault==1) { out[0]=0; out[1]=0xfc; } return fault!=2;
		},offset);
		if (fault<3) assert(!okay && c.Failed() && !quant.calls);
		else {
			assert(okay); std::vector<uint8_t> x(32*34); for (unsigned b=0;b<32;b++) QBlock(2,b,x.data()+b*34);
			quant.callFail=fault==3?1:0; quant.nonfinite=fault==4;
			if (fault==5) assert(!c.Weights(&first,8,1024,32,decode,offset) && c.Failed());
			else assert(!c.Q8Multiply(offset,1024,32,x.data(),output) && c.Failed());
		}
	}
	for (unsigned fault=0;fault<7;fault++) {
		BatchTensorIO batch; batch.compact=batch.native=batch.exact=true; batch.spanFail=fault;
		PioneerResidentTensorCache<BatchTensorIO> c(batch,16*1024*1024,false,true,true,true);
		bool okay=c.Q8Weights(&first,8,1024,64,[](unsigned r,unsigned b,uint8_t* out) { QBlock(r,b,out,true); return true; },offset);
		assert(okay==(fault==0) && c.Failed()==(fault!=0));
		if (okay) {
			assert(c.uploadedPages==18 && batch.uploads==18 && batch.spans==6 && batch.sizes==std::vector<unsigned>({4,4,1,4,4,1}));
			std::vector<uint8_t> x(32*34); for (unsigned b=0;b<32;b++) QBlock(3,b,x.data()+b*34);
			assert(c.Q8Multiply(0,1024,64,x.data(),output));
			for (unsigned row=0;row<64;row++) assert(output[row]==QReference(row,1024,x.data(),true,true));
		} else {
			assert(batch.spans==fault && !batch.calls && c.Tensors()==0);
			assert(!c.Q8Weights(&second,8,1024,64,[](unsigned,unsigned,uint8_t*) { assert(false); return false; },offset) && batch.spans==fault);
		}
	}
	printf("PASS: immutable tensor packing/cache, changed vectors, multi-call classifier chunks, capacity/nonfinite/transport failures; NO hardware\n");
}
