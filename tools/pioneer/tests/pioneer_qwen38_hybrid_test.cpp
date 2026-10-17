/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen38Hybrid.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
#include <algorithm>
struct HostReferenceMath {
	static float Exp(float x) { return expf(x); }
	static float InvSqrt(float x) { return 1/sqrtf(x); }
	static float Reciprocal(float x) { return 1/x; }
	static float Softplus(float x) { return x>20?x:log1pf(expf(x)); }
	static float Sigmoid(float x) { float e=expf(-fabsf(x)); return x>=0?1/(1+e):e/(1+e); }
	static float Silu(float x) { return x*Sigmoid(x); }
};
static double Sigmoid(double x) { return x>=0?1/(1+exp(-x)):exp(x)/(1+exp(x)); }
static void Near(double actual,double wanted,double tolerance=3e-5)
{
	assert(std::isfinite(actual) && std::isfinite(wanted));
	if (fabs(actual-wanted)>tolerance*(1+fabs(wanted))) {
		fprintf(stderr,"actual%.12g reference%.12g error%.12g\n",actual,wanted,actual-wanted); assert(false);
	}
}
static void Head()
{
	float q[128]={},k[128]={},v[128],out[128]; q[0]=.125f; k[0]=1;
	std::vector<float> previous(16384),next(16384,-123);
	for (unsigned j=0;j<128;j++) v[j]=float(int(j)-64)*.25f;
	assert(PioneerQ38DeltaHead(q,k,v,1,.5f,previous.data(),next.data(),out));
	for (unsigned i=0;i<128;i++) for (unsigned j=0;j<128;j++) assert(next[i*128+j]==(i?0:v[j]*.5f));
	for (unsigned j=0;j<128;j++) assert(out[j]==v[j]*.0625f);
	previous=next;
	assert(PioneerQ38DeltaHead(q,k,v,.5f,0,previous.data(),next.data(),out));
	for (unsigned i=0;i<16384;i++) assert(next[i]==previous[i]*.5f);
	assert(PioneerQ38DeltaHead(q,k,v,.5f,1,previous.data(),next.data(),out));
	for (unsigned j=0;j<128;j++) assert(next[j]==v[j]);
	for (unsigned fault=0;fault<9;fault++) {
		std::fill(next.begin(),next.end(),-123); std::fill(out,out+128,-123);
		float decay=1,beta=.5f; float* target=next.data(); float* result=out;
		if (fault==0) decay=-.1f; if (fault==1) decay=1.1f;
		if (fault==2) beta=-.1f; if (fault==3) beta=1.1f;
		if (fault==4) beta=std::numeric_limits<float>::infinity();
		if (fault==5) target=previous.data(); if (fault==6) result=v;
		if (fault==7) result=next.data()+128;
		if (fault==8) previous.back()=std::numeric_limits<float>::quiet_NaN();
		assert(!PioneerQ38DeltaHead(q,k,v,decay,beta,previous.data(),target,result));
		assert(std::all_of(next.begin(),next.end(),[](float f){return f==-123;}));
		assert(std::all_of(out,out+128,[](float f){return f==-123;}));
		previous.back()=0;
	}
}
static void Convolution()
{
	std::vector<float> input(10240),weights(40960),old(30720),next(30720),out(10240);
	std::vector<std::vector<float>> sequence;
	for (unsigned c=0;c<10240;c++) for (unsigned t=0;t<4;t++) weights[c*4+t]=float(int((c*7+t*11)%29)-14)*.03125f;
	for (unsigned step=0;step<12;step++) {
		for (unsigned c=0;c<10240;c++) input[c]=float(sin(c*.017+step*.231));
		sequence.push_back(input);
		assert(PioneerQ38Conv4<HostReferenceMath>(input.data(),weights.data(),old.data(),next.data(),out.data()));
		for (unsigned c=0;c<10240;c++) {
			double sum=0;
			for (unsigned t=0;t<4;t++) if (step+t>=3) sum+=double(sequence[step+t-3][c])*weights[c*4+t];
			Near(out[c],sum*Sigmoid(sum),1e-6);
			for (unsigned t=0;t<3;t++) assert(next[c*3+t]==(step+t>=2?sequence[step+t-2][c]:0));
		}
		old.swap(next);
	}
	std::fill(out.begin(),out.end(),-123); std::fill(next.begin(),next.end(),-123);
	weights.back()=std::numeric_limits<float>::infinity();
	assert(!PioneerQ38Conv4<HostReferenceMath>(input.data(),weights.data(),old.data(),next.data(),out.data()));
	assert(std::all_of(next.begin(),next.end(),[](float f){return f==-123;}));
	assert(std::all_of(out.begin(),out.end(),[](float f){return f==-123;}));
	weights.back()=0;
	assert(!PioneerQ38Conv4<HostReferenceMath>(input.data(),weights.data(),old.data(),old.data(),out.data()));
}
static void Layer()
{
	std::vector<float> conv(10240),alpha(48),beta(48),negativeA(48),bias(48),norm(128),z(6144);
	std::vector<float> old(786432),next(786432),out(6144);
	// Oracle deliberately works in ORIGINAL HF GROUPED head order and
	// DOUBLE precision, not the implementation's tiled loop or arithmetic.
	std::vector<double> hfState(786432),hfNext(786432),reference(6144);
	for (unsigned kh=0;kh<16;kh++) for (unsigned repeat=0;repeat<3;repeat++) {
		unsigned hf=kh*3+repeat,gg=repeat*16+kh;
		for (unsigned i=0;i<16384;i++) hfState[hf*16384+i]=old[gg*16384+i]=float(sin((i+1)*.031+gg*.19)*.003);
	}
	for (unsigned j=0;j<128;j++) norm[j]=.5f+float(j%17)*.0625f;
	for (unsigned step=0;step<8;step++) {
		for (unsigned i=0;i<10240;i++) conv[i]=float(sin(i*.013+step*.123)*.8);
		for (unsigned i=0;i<6144;i++) z[i]=float(cos(i*.017+step*.217)*1.7);
		for (unsigned h=0;h<48;h++) {
			alpha[h]=float(int(h%11)-5)*.5f+step*.01f; beta[h]=float(int(h%13)-6)*.7f;
			negativeA[h]=-.125f-float(h%7)*.0625f; bias[h]=float(int(h%9)-4)*.125f;
		}
		if (step==3) { alpha[47]=-100; beta[47]=-100; }
		if (step==4) { alpha[47]=100; beta[47]=100; }
		if (step==5) std::fill(conv.begin(),conv.begin()+4096,0);
		assert(PioneerQ38DeltaLayer<HostReferenceMath>(conv.data(),alpha.data(),beta.data(),negativeA.data(),
			bias.data(),norm.data(),z.data(),old.data(),next.data(),out.data()));
		for (unsigned kh=0;kh<16;kh++) for (unsigned repeat=0;repeat<3;repeat++) {
			unsigned hf=kh*3+repeat,gg=repeat*16+kh;
			double q[128],k[128],raw[128],delta[128],qs=1e-6,ks=1e-6;
			for (unsigned i=0;i<128;i++) { q[i]=conv[kh*128+i]; k[i]=conv[2048+kh*128+i]; qs+=q[i]*q[i]; ks+=k[i]*k[i]; }
			for (unsigned i=0;i<128;i++) { q[i]/=sqrt(qs)*sqrt(128.); k[i]/=sqrt(ks); }
			double a=double(alpha[gg])+bias[gg],decay=exp(double(negativeA[gg])*log1p(exp(a)));
			for (unsigned j=0;j<128;j++) {
				double predicted=0;
				for (unsigned i=0;i<128;i++) predicted+=hfState[hf*16384+i*128+j]*decay*k[i];
				delta[j]=(double(conv[4096+gg*128+j])-predicted)*Sigmoid(beta[gg]);
			}
			for (unsigned i=0;i<128;i++) for (unsigned j=0;j<128;j++) hfNext[hf*16384+i*128+j]=hfState[hf*16384+i*128+j]*decay+k[i]*delta[j];
			double squares=0;
			for (unsigned j=0;j<128;j++) {
				raw[j]=0; for (unsigned i=0;i<128;i++) raw[j]+=hfNext[hf*16384+i*128+j]*q[i];
				squares+=raw[j]*raw[j];
			}
			for (unsigned j=0;j<128;j++) {
				double gate=z[gg*128+j];
				reference[gg*128+j]=(raw[j]/sqrt(squares/128+1e-6))*norm[j]*(gate*Sigmoid(gate));
				Near(out[gg*128+j],reference[gg*128+j]);
			}
			for (unsigned i=0;i<16384;i++) Near(next[gg*16384+i],hfNext[hf*16384+i],1e-6);
		}
		old.swap(next); hfState.swap(hfNext);
	}
	for (unsigned fault=0;fault<4;fault++) {
		std::fill(next.begin(),next.end(),-123); std::fill(out.begin(),out.end(),-123);
		if (fault==0) negativeA.back()=.1f;
		if (fault==1) beta.back()=std::numeric_limits<float>::quiet_NaN();
		float* target=fault==2?old.data():next.data(); float* result=fault==3?conv.data():out.data();
		assert(!PioneerQ38DeltaLayer<HostReferenceMath>(conv.data(),alpha.data(),beta.data(),negativeA.data(),
			bias.data(),norm.data(),z.data(),old.data(),target,result));
		assert(std::all_of(next.begin(),next.end(),[](float f){return f==-123;}));
		assert(std::all_of(out.begin(),out.end(),[](float f){return f==-123;}));
		negativeA.back()=-.1f; beta.back()=.5f;
	}
}
static void Attention()
{
	std::vector<float> qGate(12288),keyRaw(1024),weight(256),cosine(32),sine(32),query(6144),key(1024),gate(6144);
	for (unsigned i=0;i<12288;i++) qGate[i]=float(sin(i*.137)*1.7);
	for (unsigned i=0;i<1024;i++) keyRaw[i]=float(cos(i*.173)*.9);
	for (unsigned i=0;i<256;i++) weight[i]=.75f+float(i%17)*.03125f;
	for (unsigned i=0;i<32;i++) { cosine[i]=float(cos(i*.037)); sine[i]=float(sin(i*.037)); }
	assert(PioneerQ38SplitNormRope<HostReferenceMath>(qGate.data(),keyRaw.data(),weight.data(),weight.data(),cosine.data(),sine.data(),query.data(),key.data(),gate.data()));
	for (unsigned kind=0;kind<2;kind++) for (unsigned h=0;h<(kind?4U:24U);h++) {
		const float* input=kind?keyRaw.data()+h*256:qGate.data()+h*512;
		const float* result=kind?key.data()+h*256:query.data()+h*256;
		double squares=0,normalized[256];
		for (unsigned j=0;j<256;j++) squares+=double(input[j])*input[j];
		for (unsigned j=0;j<256;j++) normalized[j]=input[j]/sqrt(squares/256+1e-6)*weight[j];
		for (unsigned j=0;j<256;j++) {
			double wanted=normalized[j];
			if (j<32) wanted=normalized[j]*cosine[j]-normalized[j+32]*sine[j];
			else if (j<64) wanted=normalized[j]*cosine[j-32]+normalized[j-32]*sine[j-32];
			Near(result[j],wanted,1e-6);
			if (!kind) assert(gate[h*256+j]==qGate[h*512+256+j]);
		}
	}
	std::vector<float> output(6144),scores(8192);
	for (unsigned tokens:{1U,3U,17U,129U,513U}) {
		std::vector<uint16_t> keys(tokens*1024),values(tokens*1024);
		for (unsigned i=0;i<tokens*1024;i++) {
			keys[i]=PioneerAttentionToHalf(float(sin(i*.037)*1.3)); values[i]=PioneerAttentionToHalf(float(cos(i*.031)*2.7));
		}
		for (unsigned precision:{PIONEER_Q38_ATTN_F32,PIONEER_Q38_ATTN_GGML_F16}) {
			assert(PioneerQ38FullAttention<HostReferenceMath>(query.data(),keys.data(),values.data(),tokens,gate.data(),precision,scores.data(),unsigned(scores.size()),output.data()));
			for (unsigned h=0;h<24;h++) {
				std::vector<double> probability(tokens); double maximum=-1e30,denominator=0;
				for (unsigned t=0;t<tokens;t++) {
					double dot=0;
					for (unsigned j=0;j<256;j++) {
						float q=precision==PIONEER_Q38_ATTN_F32?query[h*256+j]:PioneerAttentionHalf(PioneerAttentionToHalf(query[h*256+j]));
						float product=q*PioneerAttentionHalf(keys[t*1024+(h/6)*256+j]); dot+=product;
					}
					probability[t]=float(dot)*.0625f; maximum=std::max(maximum,probability[t]);
				}
				for (unsigned t=0;t<tokens;t++) { probability[t]=exp(probability[t]-maximum); denominator+=probability[t]; }
				for (auto& p:probability) { p/=denominator; if (precision==PIONEER_Q38_ATTN_GGML_F16) p=PioneerAttentionHalf(PioneerAttentionToHalf(float(p))); }
				for (unsigned j=0;j<256;j++) {
					double wanted=0; for (unsigned t=0;t<tokens;t++) wanted+=probability[t]*PioneerAttentionHalf(values[t*1024+(h/6)*256+j]);
					Near(output[h*256+j],wanted*Sigmoid(gate[h*256+j]));
				}
			}
		}
		keys.back()=0x7c00; std::fill(output.begin(),output.end(),-123);
		assert(!PioneerQ38FullAttention<HostReferenceMath>(query.data(),keys.data(),values.data(),tokens,gate.data(),1,scores.data(),8192,output.data()));
		assert(std::all_of(output.begin(),output.end(),[](float x){return x==-123;}));
	}
	std::vector<uint16_t> zero(8192*1024);
	for (unsigned bad:{0U,8193U}) assert(!PioneerQ38FullAttention<HostReferenceMath>(query.data(),zero.data(),zero.data(),bad,gate.data(),1,scores.data(),8192,output.data()));
	assert(!PioneerQ38FullAttention<HostReferenceMath>(query.data(),zero.data(),zero.data(),1,gate.data(),0,scores.data(),8192,output.data()));
	assert(!PioneerQ38FullAttention<HostReferenceMath>(query.data(),zero.data(),zero.data(),1,gate.data(),1,scores.data(),0,output.data()));
	assert(!PioneerQ38FullAttention<HostReferenceMath>(query.data(),zero.data(),zero.data(),1,gate.data(),1,scores.data(),8192,query.data()));
	assert(PioneerQ38FullAttention<HostReferenceMath>(query.data(),zero.data(),zero.data(),8192,gate.data(),1,scores.data(),8192,output.data()));
	for (float x:output) assert(x==0);
}
int main()
{
	Head(); Convolution(); Layer(); Attention();
	puts("PASS: Qwen3.8 hybrid math,48 tiled heads/8 recurrent positions/ALL state values,HF-grouped double oracle,12-step conv,gated24Q/4KV/256d attention/both precision policies/8192-token bound/partial RoPE/interleaved Q-gate/invalid+alias refusals;OFFLINE ONLY");
}
