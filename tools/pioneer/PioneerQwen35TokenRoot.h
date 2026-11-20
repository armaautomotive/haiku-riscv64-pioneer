/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_TOKEN_ROOT_H
#define PIONEER_QWEN35_TOKEN_ROOT_H

#include "PioneerQwen35Operators.h"

namespace PioneerQwen35 {

struct RootScratch {
	float hidden[Hidden], normalized[Hidden], projected[Hidden], weight[Hidden];
	float matrixA[Ffn], matrixB[Ffn];
	float gated[DeltaInner], keys[DenseKeyHeads*DenseHeadWidth], values[DenseKeyHeads*DenseHeadWidth];
	float state[DeltaStateWords];
};
static_assert(sizeof(RootScratch)==204800, "bounded card-local scratch");

inline unsigned TraceCode(unsigned family,unsigned layer,unsigned detail)
{
	return 0xa0000000U|((family&15U)<<24)|((layer&0xffU)<<16)|(detail&0xffffU);
}

template<class IO>
bool Recurrent(IO& io, RootScratch& work, unsigned layer, unsigned position)
{
	io.Trace(TraceCode(1,layer,position));
	float alpha[DeltaHeads], beta[DeltaHeads], negativeA[DeltaHeads], bias[DeltaHeads], norm[DeltaWidth];
	const unsigned projectRoles[4]={RecurrentQkv,RecurrentGate,Alpha,Beta};
	const unsigned projectSizes[4]={ConvChannels,DeltaInner,DeltaHeads,DeltaHeads};
	float* projectOutputs[4]={work.matrixA,work.matrixB,alpha,beta};
	if (!io.ProjectGroup(layer,projectRoles,work.normalized,Hidden,
		projectOutputs,projectSizes,4)
		|| !io.ReadWeight(layer,NegativeA,0,negativeA,DeltaHeads)
		|| !io.ReadWeight(layer,DeltaBias,0,bias,DeltaHeads)
		|| !io.ReadWeight(layer,DeltaNorm,0,norm,DeltaWidth))
		return false;

	// A new request starts at zero and ignores every former sequence's
	// convolution and delta cells. For later positions, the previous token
	// chose the read slot by its successful whole-token commit.
	unsigned slot=position&1;
	for (unsigned channel=0; channel<ConvChannels; channel+=16) {
		float weights[64], prev[48], next[48];
		if (!io.ReadWeight(layer,ConvWeights,channel*4,weights,64)) return false;
		if (position) {
			if (!io.ReadConv(layer,slot,channel,prev,48)) return false;
		} else
			for (float& lane:prev) lane=0;
		io.Trace(TraceCode(2,layer,channel));
		if (!Conv16(work.matrixA+channel,weights,prev,next)
			|| !io.WriteConv(layer,slot^1,channel,next,48)) return false;
	}
	float* q=work.matrixA;
	float* k=q+DeltaKeyHeads*DeltaWidth;
	const float* v=k+DeltaKeyHeads*DeltaWidth;
	for (unsigned group=0; group<DeltaKeyHeads; group++) {
		io.Trace(TraceCode(3,layer,group));
		if (!NormalizeDelta(q+group*DeltaWidth) || !NormalizeDelta(k+group*DeltaWidth)) return false;
	}
	for (unsigned head=0; head<DeltaHeads; head++) {
		const unsigned group=head%DeltaKeyHeads;
		if (position) {
			if (!io.ReadDelta(layer,slot,head,work.state,DeltaStateWords)) return false;
		} else
			for (float& cell:work.state) cell=0;
		float* output=work.gated+head*DeltaWidth;
		const float* gate=work.matrixB+head*DeltaWidth;
		io.Trace(TraceCode(4,layer,head));
		if (!DeltaHead(q+group*DeltaWidth,k+group*DeltaWidth,v+head*DeltaWidth,
			alpha[head],beta[head],negativeA[head],bias[head],work.state,output)) return false;
		io.Trace(TraceCode(5,layer,head));
		if (!Rms<DeltaWidth>(output,norm,output)) return false;
		io.Trace(TraceCode(6,layer,head));
		for (unsigned i=0; i<DeltaWidth; i++) output[i]*=gate[i]*Sigmoid(gate[i]);
		if (!Finite(output,DeltaWidth) || !io.WriteDelta(layer,slot^1,head,work.state,DeltaStateWords))
			return false;
	}
	return io.Project(layer,RecurrentOut,work.gated,DeltaInner,work.projected,Hidden);
}

template<class IO>
bool Attention(IO& io, RootScratch& work, unsigned layer, unsigned position)
{
	constexpr unsigned width=DenseHeadWidth;
	constexpr unsigned packedWords=width/2;
	float qNorm[width], kNorm[width], rope[Rotary], query[width];
	uint32_t key[packedWords], val[packedWords];
	const unsigned projectRoles[3]={QueryAndGate,Key,Value};
	const unsigned projectSizes[3]={2*DenseHeads*width,DenseKeyHeads*width,DenseKeyHeads*width};
	float* projectOutputs[3]={work.matrixA,work.keys,work.values};
	if (!io.ProjectGroup(layer,projectRoles,work.normalized,Hidden,
		projectOutputs,projectSizes,3)
		|| !io.ReadWeight(layer,QueryNorm,0,qNorm,width)
		|| !io.ReadWeight(layer,KeyNorm,0,kNorm,width)
		|| !io.ReadRope(position,rope,Rotary))
		return false;

	for (unsigned head=0; head<DenseKeyHeads; head++) {
		io.Trace(TraceCode(7,layer,head));
		float* current=work.keys+head*width;
		if (!Rms<width>(current,kNorm,current) || !Rotate(current,rope)
			|| !PackHead(current,key) || !PackHead(work.values+head*width,val)
			|| !io.WriteKv(layer,head,position,key,val,packedWords)) return false;
	}
	for (unsigned head=0; head<DenseHeads; head++) {
		io.Trace(TraceCode(8,layer,head));
		const unsigned group=head/(DenseHeads/DenseKeyHeads);
		const float* qg=work.matrixA+2*head*width;
		if (!Rms<width>(qg,qNorm,query) || !Rotate(query,rope)) return false;
		AttentionRow row{work.gated+head*width};
		for (unsigned at=0; at<=position; at++) {
			if (!io.ReadKv(layer,group,at,key,val,packedWords))
				return false;
			io.Trace(TraceCode(9,layer,(head<<8)|at));
			if (!row.Add(query,key,val)) return false;
		}
		io.Trace(TraceCode(10,layer,head));
		if (!row.Finish(qg+width)) return false;
	}
	return io.Project(layer,AttentionOut,work.gated,DenseHeads*width,work.projected,Hidden);
}

template<class IO>
bool RunLayers(IO& io, RootScratch& work, unsigned position, unsigned token)
{
	io.Trace(TraceCode(11,position,token));
	if (position>=MaxPositions || token>=Vocabulary || !io.Embed(token,work.hidden,Hidden))
		return false;
	for (unsigned layer=0; layer<Layers; layer++) {
		io.Trace(TraceCode(12,layer,position));
		if (!io.ReadWeight(layer,AttentionNorm,0,work.weight,Hidden)) return false;
		io.Trace(TraceCode(13,layer,position));
		if (!Rms<Hidden>(work.hidden,work.weight,work.normalized)) return false;
		io.Trace(TraceCode(14,layer,position));
		if (!(Dense(layer)?Attention(io,work,layer,position):Recurrent(io,work,layer,position)))
			return false;
		io.Trace(TraceCode(15,layer,position));
		for (unsigned i=0; i<Hidden; i++) work.hidden[i]+=work.projected[i];
		if (!io.ReadWeight(layer,PostAttentionNorm,0,work.weight,Hidden)) return false;
		io.Trace(TraceCode(16,layer,position));
		if (!Rms<Hidden>(work.hidden,work.weight,work.normalized)) return false;
		if (!io.ProjectPair(layer,Gate,Up,work.normalized,Hidden,
			work.matrixA,work.matrixB,Ffn))
			return false;
		if (!io.StoreActivation(0,work.matrixA,Ffn)
			|| !io.StoreActivation(1,work.matrixB,Ffn)
			|| !io.ActivateFfn(0,1,work.matrixA,work.matrixB,Ffn)
			|| !io.ProjectFromActivation(layer,Down,0,Ffn,work.projected,Hidden,
				work.matrixA))
			return false;
		io.Trace(TraceCode(2,layer,0xff00U));
		for (unsigned i=0; i<Hidden; i++) work.hidden[i]+=work.projected[i];
		io.Trace(TraceCode(3,layer,0xff00U));
		if (!Finite(work.hidden,Hidden)) return false;
	}
	return true;
}

struct Request {
	const uint32_t* prompt;
	unsigned promptCount, maxOutput, context, eos, endTurn;
};

// Invoked by the ROOT after the exclusive device owner authenticates the
// prompt and opens a fresh request. The only learned matvec entry points
// are Project and TiedArgmax. Commit may expose only an entire position.
template<class IO>
bool Generate(IO& io, RootScratch& work, const Request& request)
{
	io.Trace(TraceCode(1,0,0));
	if (!request.prompt || !request.promptCount || !request.maxOutput || !request.context
		|| request.context>MaxPositions || request.promptCount>request.context
		|| request.maxOutput-1>request.context-request.promptCount
		|| request.eos>=Vocabulary || request.endTurn>=Vocabulary || request.eos==request.endTurn)
		return false;
	for (unsigned i=0; i<request.promptCount; i++) {
		io.Trace(TraceCode(2,0,i));
		if (request.prompt[i]>=Vocabulary) return false;
	}
	unsigned generated=0, token=request.prompt[0];
	for (unsigned position=0; position<request.context; position++) {
		io.Trace(TraceCode(3,position,token));
		if (!RunLayers(io,work,position,token)) return false;
		bool sample=position+1>=request.promptCount;
		unsigned next=sample?Vocabulary:request.prompt[position+1];
		if (sample) {
			if (position+1==request.promptCount && !io.PrefillDone(position+1)) return false;
			if (!io.ReadWeight(Layers,FinalNorm,0,work.weight,Hidden)) return false;
			io.Trace(TraceCode(4,position,0));
			if (!Rms<Hidden>(work.hidden,work.weight,work.normalized)
				|| !io.TiedArgmax(work.normalized,Hidden,next) || next>=Vocabulary)
				return false;
			generated++;
		}
		if (!io.Commit(position,token,sample,next)) return false;
		if (sample && (generated==request.maxOutput || next==request.eos || next==request.endTurn)) {
			io.Trace(TraceCode(5,position,generated));
			return io.FinishRequest(position+1,generated);
		}
		token=next;
	}
	return false;
}
}
#endif
