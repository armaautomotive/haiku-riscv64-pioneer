/* SPDX-License-Identifier: MIT */
#include "../PioneerAttentionFixture.h"
#include "../PioneerFloat16.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <vector>

static void Reference(const float* q,const uint16_t* k,const uint16_t* v,unsigned n,unsigned stride,double* out)
{
	std::vector<double> scores(n); double maximum=-INFINITY,total=0;
	for (unsigned t=0;t<n;t++) {
		double dot=0;
		for (unsigned d=0;d<128;d++) dot+=double(q[d])*PioneerAttentionHalf(k[t*stride+d]);
		scores[t]=dot*double(0.0883883461356163f);
		maximum=fmax(maximum,scores[t]);
	}
	for (auto& value:scores) { value=exp(value-maximum); total+=value; }
	for (unsigned d=0;d<128;d++) {
		out[d]=0;
		for (unsigned t=0;t<n;t++) out[d]+=scores[t]/total*PioneerAttentionHalf(v[t*stride+d]);
	}
}

static void CheckFixture(const char* path)
{
	FILE* file=fopen(path,"rb"); assert(file);
	PioneerAttentionFixture h={}; assert(fread(&h,sizeof(h),1,file)==1 && PioneerAttentionFixtureValid(h));
	std::vector<float> q(h.queryCount),expected(h.resultCount),actual(h.resultCount);
	std::vector<uint16_t> k(h.kvCount),v(h.kvCount);
	assert(fread(q.data(),4,q.size(),file)==q.size() && fread(k.data(),2,k.size(),file)==k.size()
		&& fread(v.data(),2,v.size(),file)==v.size() && fread(expected.data(),4,expected.size(),file)==expected.size());
	assert(fgetc(file)==EOF && fclose(file)==0);
	double norm=0,delta=0,preciseNorm=0,preciseDelta=0; float maximum=0;
	for (unsigned head=0;head<16;head++) {
		unsigned kv=head/2; double reference[128];
		assert(PioneerAttentionHead(q.data()+head*128,k.data()+kv*128,v.data()+kv*128,h.tokens,1024,actual.data()+head*128));
		Reference(q.data()+head*128,k.data()+kv*128,v.data()+kv*128,h.tokens,1024,reference);
		for (unsigned d=0;d<128;d++) {
			unsigned at=head*128+d; double error=double(actual[at])-expected[at];
			assert(isfinite(actual[at]) && isfinite(expected[at]));
			norm+=double(expected[at])*expected[at]; delta+=error*error; maximum=fmaxf(maximum,fabsf(float(error)));
			preciseNorm+=reference[d]*reference[d]; double preciseError=actual[at]-reference[d]; preciseDelta+=preciseError*preciseError;
		}
	}
	double relative=norm?sqrt(delta/norm):sqrt(delta),precise=preciseNorm?sqrt(preciseDelta/preciseNorm):sqrt(preciseDelta);
	// ggml's F16 flash kernel rounds query/value accumulators to half;
	// the separate double-precision oracle checks the FP32 math itself.
	assert(relative<.02 && precise<.00002);
	printf("PUBLIC fixture layer=%u tokens=%u vs-ggml-f16-L2=%.9g max-abs=%.9g vs-double-L2=%.9g PASS (CPU ONLY)\n",h.layer,h.tokens,relative,maximum,precise);
}

int main(int argc,char** argv)
{
	double expError=0,inverseError=0;
	for (int i=0;i<=100000;i++) {
		float x=-79.999f*float(i)/100000;
		double reference=exp(double(x));
		expError=fmax(expError,fabs(double(PioneerAttentionExp(x))/reference-1));
		float denominator=1+255.0f*float(i)/100000;
		inverseError=fmax(inverseError,fabs(double(PioneerAttentionReciprocal(denominator))*denominator-1));
	}
	assert(expError<.000002 && inverseError<.000001);
	assert(PioneerAttentionExp(0)==1 && PioneerAttentionExp(-80)==0 && PioneerAttentionExp(-1000)==0);
	for (unsigned half=0;half<65536;half++) {
		unsigned e=(half>>10)&31,m=half&1023;
		if (e==31) continue;
		double expected=e?ldexp(double(1024+m),int(e)-25):ldexp(double(m),-24);
		if (half&0x8000) expected=-expected;
		assert(double(PioneerAttentionHalf(uint16_t(half)))==expected);
	}
	for (unsigned n:{1U,2U,3U,7U,16U,31U,32U,64U,128U,192U,256U}) for (unsigned stride:{128U,1024U}) {
		float query[128],actual[128]; double expected[128];
		std::vector<uint16_t> keys(n*stride,0x7e00),values(n*stride,0x7e00);
		for (unsigned d=0;d<128;d++) query[d]=float(sin(d*.731)*2);
		for (unsigned t=0;t<n;t++) for (unsigned d=0;d<128;d++) {
			keys[t*stride+d]=PioneerFloat16(float(sin(t*.173+d*.263)*3));
			values[t*stride+d]=PioneerFloat16(float(cos(t*.419+d*.359)*2));
		}
		assert(PioneerAttentionHead(query,keys.data(),values.data(),n,stride,actual));
		Reference(query,keys.data(),values.data(),n,stride,expected);
		double norm=0,delta=0;
		for (unsigned d=0;d<128;d++) { norm+=expected[d]*expected[d]; double diff=actual[d]-expected[d]; delta+=diff*diff; }
		assert(sqrt(delta/(norm?norm:1))<.00002);
	}
	float query[128]={},output[128]; uint16_t keys[128]={},values[128];
	for (unsigned i=0;i<128;i++) { output[i]=123; values[i]=uint16_t(i+1); }
	assert(PioneerAttentionHead(query,keys,values,1,128,output));
	for (unsigned i=0;i<128;i++) assert(output[i]==PioneerAttentionHalf(values[i]));
	assert(!PioneerAttentionHead(query,keys,values,0,128,output));
	assert(!PioneerAttentionHead(query,keys,values,257,128,output));
	assert(!PioneerAttentionHead(query,keys,values,1,127,output));
	assert(!PioneerAttentionHead(NULL,keys,values,1,128,output));
	query[0]=INFINITY; assert(!PioneerAttentionHead(query,keys,values,1,128,output));
	query[0]=NAN; assert(!PioneerAttentionHead(query,keys,values,1,128,output)); query[0]=0;
	keys[7]=0x7c00; assert(!PioneerAttentionHead(query,keys,values,1,128,output)); keys[7]=0;
	values[12]=0x7e00; assert(!PioneerAttentionHead(query,keys,values,1,128,output));
	PioneerAttentionFixture h={0x4e545450,1,0,1,128,16,8,2048,1024,2048,PioneerAttentionBits(.0883883461356163f),0};
	assert(PioneerAttentionFixtureValid(h)); h.tokens=257; assert(!PioneerAttentionFixtureValid(h));
	printf("PASS: CPU-only bounded attention reference; exp relative-error %.9g reciprocal %.9g; half/subnormal decode, grouped strides, causal prefix, finite/size guards; NO CARD IO\n",expError,inverseError);
	for (int i=1;i<argc;i++) CheckFixture(argv[i]);
	return 0;
}
