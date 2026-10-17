/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen38PackedDot.h"
#include "../PioneerQwen38Gguf.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <random>
static void Compare(unsigned type,const uint8_t* packed,const PioneerQ38Int8* input,float scale)
{
	unsigned n=type==8?32:256,bytes=type==8?34:type==12?144:210;
	float values[256]={},actual=0;
	bool good=type==8?PioneerDecodeQ8_0(packed,values):type==12?PioneerDecodeQ4K(packed,values):PioneerDecodeQ6K(packed,values);
	assert(good && PioneerQ38PackedDot(type,packed,bytes,input,n,scale,&actual));
	double wanted=0,absolute=0;
	for (unsigned i=0;i<n;i++) { double product=double(values[i])*(double(input[i])*scale); wanted+=product; absolute+=fabs(product); }
	if (!std::isfinite(actual) || fabs(actual-wanted)>1e-6+absolute*5e-7) {
		fprintf(stderr,"type%u actual%.12g reference%.12g normalized-error%.12g\n",type,actual,wanted,fabs(actual-wanted)/(1+absolute)); assert(false);
	}
}
int main(int argc,char** argv)
{
	assert(argc==1 || argc==2); std::mt19937 random(0x38127);
	for (unsigned type:{8U,12U,14U}) for (unsigned trial=0;trial<1024;trial++) {
		uint8_t packed[210]; PioneerQ38Int8 input[256];
		for (auto& byte:packed) byte=uint8_t(random());
		for (auto& value:input) value=PioneerQ38Int8(int(random()%256)-128);
		unsigned d=type==14?208:0;
		packed[d]=uint8_t(random()); packed[d+1]=uint8_t((random()%0x74)+1);
		if (type==12) { packed[2]=uint8_t(random()); packed[3]=uint8_t((random()%0x74)+1); }
		float scale=trial&1?-.03125f:.0173f; if (!trial) scale=0;
		Compare(type,packed,input,scale);
		unsigned bytes=type==8?34:type==12?144:210,n=type==8?32:256;
		float output=123;
		assert(!PioneerQ38PackedDot(type,packed,bytes-1,input,n,scale,&output) && output==123);
		assert(!PioneerQ38PackedDot(type,packed,bytes,input,n-1,scale,&output) && output==123);
		assert(!PioneerQ38PackedDot(type,packed,bytes,input,n,std::numeric_limits<float>::infinity(),&output) && output==123);
		assert(!PioneerQ38PackedDot(type,packed,bytes,input,n,scale,reinterpret_cast<float*>(packed)));
		packed[d]=1; packed[d+1]=0x7c;
		assert(!PioneerQ38PackedDot(type,packed,bytes,input,n,scale,&output) && output==123);
	}
	uint64_t blocks=0;
	if (argc==2) {
		FILE* file=fopen(argv[1],"rb"); assert(file); PioneerQwen38Gguf model; assert(model.Open(file));
		for (unsigned t=0;t<model.Tensors().size();t++) {
			const auto& tensor=model.Tensors()[t]; if (!tensor.type) continue;
			unsigned bytes=PioneerQ38BlockBytes(tensor.type),n=PioneerQ38BlockElements(tensor.type);
			for (uint64_t position:{UINT64_C(0),tensor.bytes-bytes}) {
				uint8_t packed[210]; PioneerQ38Int8 input[256];
				assert(model.ReadPacked(t,position,bytes,packed,sizeof(packed)));
				for (unsigned i=0;i<n;i++) input[i]=PioneerQ38Int8(int((i*37+t*17)%256)-128);
				Compare(tensor.type,packed,input,t&1?-.0371f:.0123f); blocks++;
			}
		}
		assert(blocks==996 && model.Stable()); fclose(file);
	}
	printf("PASS:3072 independent randomized packed Q4_K/Q6_K/Q8_0 dots and%llu actual checkpoint blocks,signed activation scale/min correction/int bounds/NaN+size+alias refusals;OFFLINE NOT Tensix acceleration\n",(unsigned long long)blocks);
}
