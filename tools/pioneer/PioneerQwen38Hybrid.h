/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_HYBRID_H
#define PIONEER_QWEN38_HYBRID_H
#include "PioneerAttentionMath.h"

// Qwen3.8-27B GGUF hybrid-layer primitives. NOT a registered backend, CPU
// fallback or device/NoC allocator. Caller owns all buffers and publishes
// scratch outputs/next state ONLY after true. Previous state is immutable.
// GGUF uses TILED V-head order: keyHead = valueHead % 16, NOT valueHead/3.
// GGUF ssm_a already contains -exp(A_log); NEVER exponentiate it again.
// Verified against pinned upstream conversion/qwen.py and models/qwen35.cpp:
// ggml-org/llama.cpp/c060ca974c773c7c3d17fd1b66dc9d312bc292c0.
enum { PIONEER_Q38_DIM=128, PIONEER_Q38_K_HEADS=16, PIONEER_Q38_V_HEADS=48,
	PIONEER_Q38_QK=2048, PIONEER_Q38_V=6144, PIONEER_Q38_CONV=10240,
	PIONEER_Q38_HEAD_STATE=16384, PIONEER_Q38_STATE=786432 };

static inline bool PioneerQ38Finite(float value)
{
	return (PioneerAttentionBits(value)&0x7f800000)!=0x7f800000;
}
static inline bool PioneerQ38FiniteVector(const float* values,unsigned count)
{
	if (!values) return false;
	for (unsigned i=0;i<count;i++) if (!PioneerQ38Finite(values[i])) return false;
	return true;
}
static inline bool PioneerQ38Disjoint(const float* a,unsigned an,const float* b,unsigned bn)
{
	typedef __UINTPTR_TYPE__ Address;
	Address aa=reinterpret_cast<Address>(a),bb=reinterpret_cast<Address>(b),limit=~Address(0);
	if (!a || !b || an>limit/4 || bn>limit/4 || aa>limit-Address(an)*4 || bb>limit-Address(bn)*4) return false;
	return aa+Address(an)*4<=bb || bb+Address(bn)*4<=aa;
}

// Local one-head recurrent primitive. q already includes 1/sqrt(128), k
// already L2-normalized. Layout is [key_dimension][value_dimension]. This
// has no transcendental/divide/sqrt instruction and can be separately
// compiled for the card; it is not a launch or ownership permission.
static inline bool PioneerQ38DeltaHead(const float* q,const float* k,const float* v,
	float decay,float beta,const float* previous,float* next,float* output)
{
	const float* read[]={q,k,v,previous}; const unsigned sizes[]={128,128,128,16384};
	if (!PioneerQ38Finite(decay) || !PioneerQ38Finite(beta) || decay<0 || decay>1 || beta<0 || beta>1
		|| !PioneerQ38Disjoint(next,16384,output,128)) return false;
	for (unsigned p=0;p<4;p++) if (!PioneerQ38FiniteVector(read[p],sizes[p])
		|| !PioneerQ38Disjoint(next,16384,read[p],sizes[p])
		|| !PioneerQ38Disjoint(output,128,read[p],sizes[p])) return false;
	float delta[128];
	for (unsigned j=0;j<128;j++) {
		float prediction=0;
		for (unsigned i=0;i<128;i++) prediction+=(previous[i*128+j]*decay)*k[i];
		delta[j]=(v[j]-prediction)*beta;
		if (!PioneerQ38Finite(delta[j])) return false;
	}
	for (unsigned i=0;i<128;i++) for (unsigned j=0;j<128;j++) {
		float value=previous[i*128+j]*decay+k[i]*delta[j];
		if (!PioneerQ38Finite(value)) return false;
		next[i*128+j]=value;
	}
	for (unsigned j=0;j<128;j++) {
		float value=0;
		for (unsigned i=0;i<128;i++) value+=next[i*128+j]*q[i];
		if (!PioneerQ38Finite(value)) return false;
		output[j]=value;
	}
	return true;
}

// Math policy supplies Exp,Softplus,Sigmoid,Silu and InvSqrt. Tests use an
// explicit host reference policy. There is NO implicit native CPU policy.
template<class Math> bool PioneerQ38Conv4(const float* input,const float* weights,
	const float* previous,float* next,float* output)
{
	const float* read[]={input,weights,previous}; const unsigned sizes[]={10240,40960,30720};
	if (!PioneerQ38Disjoint(next,30720,output,10240)) return false;
	for (unsigned p=0;p<3;p++) if (!PioneerQ38FiniteVector(read[p],sizes[p])
		|| !PioneerQ38Disjoint(next,30720,read[p],sizes[p])
		|| !PioneerQ38Disjoint(output,10240,read[p],sizes[p])) return false;
	for (unsigned c=0;c<10240;c++) {
		float value=0;
		for (unsigned t=0;t<3;t++) value+=previous[c*3+t]*weights[c*4+t];
		value+=input[c]*weights[c*4+3];
		value=Math::Silu(value); if (!PioneerQ38Finite(value)) return false;
		output[c]=value;
		next[c*3]=previous[c*3+1]; next[c*3+1]=previous[c*3+2]; next[c*3+2]=input[c];
	}
	return true;
}

template<class Math> bool PioneerQ38DeltaLayer(const float* convolved,const float* alpha,
	const float* betaLogit,const float* negativeA,const float* dtBias,const float* norm,
	const float* z,const float* previous,float* next,float* output)
{
	const float* read[]={convolved,alpha,betaLogit,negativeA,dtBias,norm,z,previous};
	const unsigned sizes[]={10240,48,48,48,48,128,6144,786432};
	if (!PioneerQ38Disjoint(next,786432,output,6144)) return false;
	for (unsigned p=0;p<8;p++) if (!PioneerQ38FiniteVector(read[p],sizes[p])
		|| !PioneerQ38Disjoint(next,786432,read[p],sizes[p])
		|| !PioneerQ38Disjoint(output,6144,read[p],sizes[p])) return false;
	for (unsigned h=0;h<48;h++) if (!(negativeA[h]<0)) return false;
	float q[2048],k[2048];
	for (unsigned h=0;h<16;h++) {
		float qs=0,ks=0;
		for (unsigned i=0;i<128;i++) { float a=convolved[h*128+i],b=convolved[2048+h*128+i]; qs+=a*a; ks+=b*b; }
		if (!PioneerQ38Finite(qs) || !PioneerQ38Finite(ks)) return false;
		float qi=Math::InvSqrt(qs+1e-6f)*0.0883883461356163f,ki=Math::InvSqrt(ks+1e-6f);
		if (!PioneerQ38Finite(qi) || !PioneerQ38Finite(ki)) return false;
		for (unsigned i=0;i<128;i++) { q[h*128+i]=convolved[h*128+i]*qi; k[h*128+i]=convolved[2048+h*128+i]*ki; }
	}
	for (unsigned h=0;h<48;h++) {
		float biased=alpha[h]+dtBias[h]; if (!PioneerQ38Finite(biased)) return false;
		float decay=Math::Exp(negativeA[h]*Math::Softplus(biased)),beta=Math::Sigmoid(betaLogit[h]);
		float raw[128];
		if (!PioneerQ38DeltaHead(q+(h%16)*128,k+(h%16)*128,convolved+4096+h*128,
			decay,beta,previous+h*16384,next+h*16384,raw)) return false;
		float sum=0;
		for (unsigned j=0;j<128;j++) sum+=raw[j]*raw[j];
		if (!PioneerQ38Finite(sum)) return false;
		float inverse=Math::InvSqrt(sum*0.0078125f+1e-6f);
		for (unsigned j=0;j<128;j++) {
			float value=((raw[j]*inverse)*norm[j])*Math::Silu(z[h*128+j]);
			if (!PioneerQ38Finite(value)) return false;
			output[h*128+j]=value;
		}
	}
	return true;
}

// Full-attention Q projection is [Q256,GATE256] interleaved PER head,NOT
// [all Q,all GATE]. Normalize all256 dimensions,rotate only first64 in
// split-half/NeoX order. Text positions use the same position on all axes;
// caller supplies the32 cos/sin pairs from the pinned1e7 rotary base.
template<class Math> bool PioneerQ38SplitNormRope(const float* qGate,const float* k,
	const float* qWeight,const float* kWeight,const float* cosine,const float* sine,
	float* query,float* key,float* gate)
{
	const float* read[]={qGate,k,qWeight,kWeight,cosine,sine};
	const unsigned sizes[]={12288,1024,256,256,32,32};
	float* write[]={query,key,gate}; const unsigned counts[]={6144,1024,6144};
	for (unsigned a=0;a<3;a++) {
		for (unsigned b=a+1;b<3;b++) if (!PioneerQ38Disjoint(write[a],counts[a],write[b],counts[b])) return false;
		for (unsigned p=0;p<6;p++) if (!PioneerQ38FiniteVector(read[p],sizes[p])
			|| !PioneerQ38Disjoint(write[a],counts[a],read[p],sizes[p])) return false;
	}
	for (unsigned kind=0;kind<2;kind++) for (unsigned h=0;h<(kind?4U:24U);h++) {
		const float* input=kind?k+h*256:qGate+h*512;
		const float* weight=kind?kWeight:qWeight;
		float* output=kind?key+h*256:query+h*256;
		float sum=0,normalized[256];
		for (unsigned j=0;j<256;j++) sum+=input[j]*input[j];
		if (!PioneerQ38Finite(sum)) return false;
		float inverse=Math::InvSqrt(sum*.00390625f+1e-6f);
		for (unsigned j=0;j<256;j++) { normalized[j]=(input[j]*inverse)*weight[j]; if (!PioneerQ38Finite(normalized[j])) return false; }
		for (unsigned j=0;j<256;j++) {
			float value=normalized[j];
			if (j<32) value=normalized[j]*cosine[j]-normalized[j+32]*sine[j];
			else if (j<64) value=normalized[j]*cosine[j-32]+normalized[j-32]*sine[j-32];
			if (!PioneerQ38Finite(value)) return false;
			output[j]=value;
			if (!kind) gate[h*256+j]=qGate[h*512+256+j];
		}
	}
	return true;
}

enum { PIONEER_Q38_ATTN_F32=1,PIONEER_Q38_ATTN_GGML_F16=2 };
// Bounded READ-ONLY already-appended FP16 cache: [token][4 KV heads][256].
// Explicit precision distinguishes the canonical F32 query/probability
// reference from the existing CPU's F16 dot operands. Neither is selected
// implicitly. All reads remain local caller buffers,not device authority.
template<class Math> bool PioneerQ38FullAttention(const float* query,const uint16_t* keys,
	const uint16_t* values,unsigned tokens,const float* gate,unsigned precision,
	float* scores,unsigned scoreCapacity,float* output)
{
	if (!tokens || tokens>8192 || scoreCapacity<tokens || !keys || !values || !scores
		|| (precision!=PIONEER_Q38_ATTN_F32 && precision!=PIONEER_Q38_ATTN_GGML_F16)
		|| !PioneerQ38FiniteVector(query,6144) || !PioneerQ38FiniteVector(gate,6144)
		|| !PioneerQ38Disjoint(scores,tokens,output,6144)) return false;
	const float* read[]={query,gate,reinterpret_cast<const float*>(keys),reinterpret_cast<const float*>(values)};
	const unsigned sizes[]={6144,6144,tokens*512,tokens*512};
	for (unsigned p=0;p<4;p++) if (!PioneerQ38Disjoint(scores,tokens,read[p],sizes[p])
		|| !PioneerQ38Disjoint(output,6144,read[p],sizes[p])) return false;
	for (unsigned i=0;i<tokens*1024;i++) if ((keys[i]&0x7c00)==0x7c00 || (values[i]&0x7c00)==0x7c00) return false;
	for (unsigned h=0;h<24;h++) {
		float q[256];
		for (unsigned j=0;j<256;j++) {
			q[j]=precision==PIONEER_Q38_ATTN_GGML_F16?PioneerAttentionHalf(PioneerAttentionToHalf(query[h*256+j])):query[h*256+j];
			if (!PioneerQ38Finite(q[j])) return false;
		}
		float maximum=-3.4028234663852886e38f;
		for (unsigned t=0;t<tokens;t++) {
			float high=0,low=0;
			for (unsigned j=0;j<256;j++) PioneerAttentionAdd(q[j]*PioneerAttentionHalf(keys[t*1024+(h/6)*256+j]),high,low);
			scores[t]=(high+low)*.0625f;
			if (!PioneerQ38Finite(scores[t])) return false;
			if (scores[t]>maximum) maximum=scores[t];
		}
		float high=0,low=0;
		for (unsigned t=0;t<tokens;t++) { scores[t]=Math::Exp(scores[t]-maximum); PioneerAttentionAdd(scores[t],high,low); }
		float inverse=Math::Reciprocal(high+low);
		if (!PioneerQ38Finite(inverse) || !(inverse>0)) return false;
		for (unsigned t=0;t<tokens;t++) {
			scores[t]*=inverse;
			if (precision==PIONEER_Q38_ATTN_GGML_F16) scores[t]=PioneerAttentionHalf(PioneerAttentionToHalf(scores[t]));
		}
		for (unsigned j=0;j<256;j++) {
			high=low=0;
			for (unsigned t=0;t<tokens;t++) PioneerAttentionAdd(scores[t]*PioneerAttentionHalf(values[t*1024+(h/6)*256+j]),high,low);
			float value=(high+low)*Math::Sigmoid(gate[h*256+j]);
			if (!PioneerQ38Finite(value)) return false;
			output[h*256+j]=value;
		}
	}
	return true;
}
#endif
