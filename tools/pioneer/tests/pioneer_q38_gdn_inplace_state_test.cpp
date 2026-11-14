/* SPDX-License-Identifier: MIT */
// Offline order-preserving check of a column-at-a-time in-place GDN update.
// This only tests arithmetic/storage aliasing; it is not a device image.
#if !defined(PIONEER_Q38_GDN_INPLACE_LAYOUT_TEST) || PIONEER_Q38_GDN_INPLACE_LAYOUT_TEST != 1 \
	|| defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
	|| defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(_KERNEL_MODE) || defined(__FAST_MATH__)
#error Offline isolated GDN in-place layout test only
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
constexpr unsigned N=128,StateSize=N*N;
using State=std::array<float,StateSize>;
using Vec=std::array<float,N>;
struct Result { State state; Vec output; };

Result TwoBuffer(const State& prior,const Vec& q,const Vec& k,const Vec& v,float decay,float beta)
{
	Result r{};
	Vec delta{};
	for(unsigned j=0;j<N;j++){
		float predicted=0;
		for(unsigned i=0;i<N;i++)predicted+=prior[i*N+j]*decay*k[i];
		delta[j]=(v[j]-predicted)*beta;
	}
	for(unsigned i=0;i<N;i++)for(unsigned j=0;j<N;j++)
		r.state[i*N+j]=prior[i*N+j]*decay+k[i]*delta[j];
	for(unsigned j=0;j<N;j++){
		float sum=0;
		for(unsigned i=0;i<N;i++)sum+=r.state[i*N+j]*q[i];
		r.output[j]=sum;
	}
	return r;
}

Result InPlace(State state,const Vec& q,const Vec& k,const Vec& v,float decay,float beta)
{
	Result r{};Vec delta{};
	for(unsigned j=0;j<N;j++){
		float predicted=0;
		for(unsigned i=0;i<N;i++)predicted+=state[i*N+j]*decay*k[i];
		delta[j]=(v[j]-predicted)*beta;
		float projected=0;
		for(unsigned i=0;i<N;i++){
			const unsigned at=i*N+j;
			const float next=state[at]*decay+k[i]*delta[j];
			state[at]=next;
			projected+=next*q[i];
		}
		r.output[j]=projected;
	}
	r.state=state;
	return r;
}

uint32_t Next(uint32_t& x){x^=x<<13;x^=x>>17;x^=x<<5;return x;}
float Sample(uint32_t& x,float scale)
{
	const int32_t signedValue=int32_t(Next(x)>>8)-0x7fffff;
	return float(signedValue)*(scale/8388608.0f);
}
bool Same(const Result& a,const Result& b)
{
	return std::memcmp(a.state.data(),b.state.data(),sizeof(a.state))==0
		&& std::memcmp(a.output.data(),b.output.data(),sizeof(a.output))==0;
}
}

int main()
{
	uint32_t seed=0x8f31a2c7;
	unsigned exact=0;
	for(unsigned trial=0;trial<128;trial++){
		State prior{};Vec q{},k{},v{};
		for(float& x:prior)x=Sample(seed,0.35f);
		for(unsigned i=0;i<N;i++){q[i]=Sample(seed,0.5f);k[i]=Sample(seed,0.2f);v[i]=Sample(seed,0.3f);}
		const float decay=0.55f+float(Next(seed)%440000)/1000000.0f;
		const float beta=0.05f+float(Next(seed)%900000)/1000000.0f;
		const auto baseline=TwoBuffer(prior,q,k,v,decay,beta);
		const auto candidate=InPlace(prior,q,k,v,decay,beta);
		assert(Same(baseline,candidate));exact++;
	}
	for(float decay:{0.0f,0.25f,1.0f})for(float beta:{0.0f,0.5f,1.0f}){
		State prior{};Vec q{},k{},v{};
		for(unsigned i=0;i<N;i++){q[i]=(i&1)?-0.5f:0.5f;k[i]=(i%3)?0.125f:-0.25f;v[i]=(i%5)?0.75f:-0.75f;}
		for(unsigned i=0;i<StateSize;i++)prior[i]=float(int(i%17)-8)*0.03125f;
		assert(Same(TwoBuffer(prior,q,k,v,decay,beta),InPlace(prior,q,k,v,decay,beta)));
	}
	std::printf("PASS offline GDN in-place state aliasing: %u randomized 128x128 cases bit-exact plus decay/beta boundaries; removes one 64KiB next-state buffer in this scalar ordering only; NO vector image, residency, device or speed proof\n",exact);
}
