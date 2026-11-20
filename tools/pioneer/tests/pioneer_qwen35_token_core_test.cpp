/* SPDX-License-Identifier: MIT */
#define PIONEER_Q35_OPERATOR_TEST
#include "../PioneerQwen35TokenRoot.h"
#include "../PioneerQwen35Gguf.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

namespace Q=PioneerQwen35;

static void CheckOperators()
{
	float tiny[Q::DeltaWidth]={1e-8f};
	assert(Q::NormalizeDelta(tiny) && std::fabs(tiny[0]-0.01f)<1e-8f);

	float query[Q::DeltaWidth]={}, key[Q::DeltaWidth]={}, value[Q::DeltaWidth]={};
	float output[Q::DeltaWidth]={}, state[Q::DeltaStateWords]={};
	query[3]=0.5f;
	key[1]=0.5f;
	key[3]=0.25f;
	value[17]=0.75f;
	state[17*Q::DeltaWidth+1]=0.5f;
	state[17*Q::DeltaWidth+3]=0.25f;
	double s1=0.5, s3=0.25;
	const double decay=std::exp(-2*std::log1p(std::exp(0.5)));
	const double beta=1/(1+std::exp(-0.25));
	for (unsigned step=0; step<2; step++) {
		assert(Q::DeltaHead(query,key,value,0.75f,0.25f,-2.0f,-0.25f,state,output));
		s1*=decay;
		s3*=decay;
		double delta=(0.75-(s1*0.5+s3*0.25))*beta;
		s1+=0.5*delta;
		s3+=0.25*delta;
		assert(std::fabs(state[17*Q::DeltaWidth+1]-s1)<2e-7);
		assert(std::fabs(state[17*Q::DeltaWidth+3]-s3)<2e-7);
		assert(std::fabs(output[17]-(s3*0.5/std::sqrt(128.0)))<2e-8);
		assert(output[3]==0 && state[3*Q::DeltaWidth+17]==0);
	}

	float row[Q::DenseHeadWidth]={}, q[Q::DenseHeadWidth]={1};
	float gate[Q::DenseHeadWidth]={};
	Q::AttentionRow attention{row};
	double denominator=0, numerator=0;
	for (unsigned i=0; i<3; i++) {
		const float score=i==0?-2.0f:i==1?3.0f:1.0f;
		const float lane=i==0?0.5f:i==1?0.25f:-1.0f;
		float keys[Q::DenseHeadWidth]={score*16}, values[Q::DenseHeadWidth]={lane};
		uint32_t packedK[Q::DenseHeadWidth/2], packedV[Q::DenseHeadWidth/2];
		assert(Q::PackHead(keys,packedK) && Q::PackHead(values,packedV));
		assert(attention.Add(q,packedK,packedV));
		denominator+=std::exp(double(score));
		numerator+=std::exp(double(score))*lane;
	}
	assert(attention.Finish(gate) && std::fabs(row[0]-0.5*numerator/denominator)<1e-7);
	assert(row[1]==0);

	float input[16]={2}, weights[64]={1,2,3,4}, previous[48]={1,3,5}, next[48]={};
	assert(Q::Conv16(input,weights,previous,next));
	assert(next[0]==3 && next[1]==5 && next[2]==2);
	assert(std::fabs(input[0]-30.0/(1+std::exp(-30.0)))<1e-6);
}

struct Stub {
	constexpr static unsigned Positions=3, Packed=Q::DenseHeadWidth/2;
	unsigned position=0, currentLayer=0, index=0, headCalls=0, commits=0, prefill=0, finishes=0;
	unsigned convReads=0, deltaReads=0;
	bool failSecondPosition=false;
	bool deltaReady[Q::Layers][2][Q::DeltaHeads]={};
	bool convReady[Q::Layers][2][Q::ConvChannels/16]={};
	struct KvCell { uint32_t key[Packed], value[Packed]; bool written=false; };
	KvCell kv[Q::Layers/4][Q::DenseKeyHeads][Positions];
	std::vector<unsigned> tokens;
	void Trace(unsigned) {}

	bool Embed(unsigned token,float* hidden,unsigned n)
	{
		assert(n==Q::Hidden && token==(position==0?3U:position==1?4U:7U));
		std::fill_n(hidden,n,0.02f);
		return true;
	}

	bool Project(unsigned layer,unsigned role,const float* input,unsigned inputs,float* output,unsigned n)
	{
		const auto* expected=PioneerQ35Role(layer,role);
		assert(expected && expected->type && inputs==expected->inputs && n==expected->outputs);
		assert(Q::Finite(input,inputs) && layer==currentLayer);
		const unsigned delta[]={6,5,10,11,13,2,3,4};
		const unsigned dense[]={5,6,7,10,2,3,4};
		const bool full=Q::Dense(layer);
		assert(index<(full?7U:8U) && role==(full?dense[index]:delta[index]));
		if (++index==(full?7U:8U)) { currentLayer++; index=0; }
		if (failSecondPosition && position==1 && layer==9 && role==6) return false;
		for (unsigned i=0;i<n;i++) output[i]=0.03f+float(i%13)*0.001f;
		return true;
	}

	bool ProjectPair(unsigned layer,unsigned firstRole,unsigned secondRole,const float* input,
		unsigned inputs,float* firstOutput,float* secondOutput,unsigned n)
	{
		return Project(layer,firstRole,input,inputs,firstOutput,n)
			&& Project(layer,secondRole,input,inputs,secondOutput,n);
	}

	bool ReadWeight(unsigned layer,unsigned role,unsigned offset,float* output,unsigned values)
	{
		const auto* expected=PioneerQ35Role(layer,role);
		assert(expected && !expected->type && values<=expected->inputs*expected->outputs-offset);
		std::fill_n(output,values,layer<Q::Layers && !Q::Dense(layer) && role==Q::NegativeA?-1.0f:1.0f);
		return true;
	}

	bool ReadRope(unsigned at,float* output,unsigned values)
	{
		assert(at==position && values==Q::Rotary);
		for (unsigned i=0;i<values;i++) output[i]=(i&1)?0.0f:1.0f;
		return true;
	}

	bool ReadConv(unsigned layer,unsigned slot,unsigned channel,float* output,unsigned values)
	{
		assert(position && slot==(position&1) && values==48 && convReady[layer][slot][channel/16]);
		std::fill_n(output,values,0.03f);
		convReads++;
		return true;
	}

	bool WriteConv(unsigned layer,unsigned slot,unsigned channel,const float* input,unsigned values)
	{
		assert(!Q::Dense(layer) && channel<Q::ConvChannels && !(channel&15)
			&& slot==((position&1)^1) && values==48 && Q::Finite(input,values));
		if (!position) {
			// A warm request must ignore retained history for its first token.
			assert(input[0]==0 && input[1]==0 && input[2]!=0);
		}
		convReady[layer][slot][channel/16]=true;
		return true;
	}

	bool ReadDelta(unsigned layer,unsigned slot,unsigned head,float* output,unsigned values)
	{
		assert(position && slot==(position&1) && values==Q::DeltaStateWords && deltaReady[layer][slot][head]);
		std::fill_n(output,values,0.007f);
		deltaReads++;
		return true;
	}

	bool WriteDelta(unsigned layer,unsigned slot,unsigned head,const float* input,unsigned values)
	{
		assert(!Q::Dense(layer) && head<Q::DeltaHeads && slot==((position&1)^1)
			&& values==Q::DeltaStateWords && Q::Finite(input,values));
		deltaReady[layer][slot][head]=true;
		return true;
	}

	bool ReadKv(unsigned layer,unsigned head,unsigned at,uint32_t* key,uint32_t* value,unsigned words)
	{
		assert(Q::Dense(layer) && head<Q::DenseKeyHeads && at<=position && words==Packed);
		const auto& cell=kv[layer/4][head][at];
		assert(cell.written);
		std::copy_n(cell.key,words,key);
		std::copy_n(cell.value,words,value);
		return true;
	}

	bool WriteKv(unsigned layer,unsigned head,unsigned at,const uint32_t* key,const uint32_t* value,unsigned words)
	{
		assert(Q::Dense(layer) && head<Q::DenseKeyHeads && at==position && words==Packed);
		auto& cell=kv[layer/4][head][at];
		std::copy_n(key,words,cell.key);
		std::copy_n(value,words,cell.value);
		cell.written=true;
		return true;
	}

	bool PrefillDone(unsigned positions)
	{
		assert(positions==2 && position==1 && currentLayer==Q::Layers);
		prefill++;
		return true;
	}

	bool TiedArgmax(const float* input,unsigned inputs,unsigned& next)
	{
		assert(position>=1 && inputs==Q::Hidden && currentLayer==Q::Layers && Q::Finite(input,inputs));
		next=position==1?7:102;
		headCalls++;
		return true;
	}

	bool Commit(unsigned at,unsigned token,bool sampled,unsigned next)
	{
		assert(at==position && currentLayer==Q::Layers && sampled==(at>=1));
		assert(token==(at==0?3U:at==1?4U:7U));
		assert(next==(at==0?4U:at==1?7U:102U));
		if (sampled) tokens.push_back(next);
		commits++;
		position++;
		currentLayer=0;
		return true;
	}

	bool FinishRequest(unsigned positions,unsigned generated)
	{
		assert(position==positions && positions==3 && generated==2);
		finishes++;
		position=0; // the next request retains state storage and starts at zero
		return true;
	}
};

int main()
{
	CheckOperators();
	auto scratch=std::make_unique<Q::RootScratch>();
	uint32_t prompt[]={3,4};
	const Q::Request request{prompt,2,3,4,101,102};
	Stub io{};
	assert(Q::Generate(io,*scratch,request));
	assert(Q::Generate(io,*scratch,request));
	assert(io.finishes==2 && io.commits==6 && io.prefill==2 && io.headCalls==4);
	assert((io.tokens==std::vector<unsigned>{7,102,7,102}));
	assert(io.convReads==4*24*(Q::ConvChannels/16) && io.deltaReads==4*24*Q::DeltaHeads);
	Stub failure{};
	failure.failSecondPosition=true;
	assert(!Q::Generate(failure,*scratch,request));
	assert(failure.commits==1 && failure.finishes==0 && failure.headCalls==0);
	prompt[1]=Q::Vocabulary;
	Stub invalid{};
	assert(!Q::Generate(invalid,*scratch,request) && invalid.commits==0 && invalid.currentLayer==0);
	puts("PASS 4B ROOT operators, all 32 layer schedules, fresh warm state, and refusal before partial publication (stub device).");
}
