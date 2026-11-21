/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowGenerationSequence.h"

#include <assert.h>

using namespace PioneerQwen35;

static DataflowProjectionProgram sProjectionScratch;

struct FakeGenerationClock {
	uint64_t now=0;
	uint64_t NowMicros() { return now++; }
};

struct FakeDecoderAuthority {
	unsigned resets=0;
	unsigned tiedOutputBuilds=0;
	unsigned argmaxBuilds=0;
	bool ResetRecurrentState() { resets++; return true; }
	bool BuildEmbedding(unsigned token,unsigned slot,DataflowEmbeddingProgram& p)
		{ p.token=token; p.outputSlot=slot; return true; }
	bool BuildResidual(unsigned mode,unsigned,DataflowResidualProgram& p)
		{ p.mode=mode; return true; }
	bool BuildRms(unsigned layer,unsigned role,unsigned input,unsigned output,
		DataflowRmsProgram& p)
		{ p.layer=layer; p.role=role; p.inputSlot=input; p.outputSlot=output; return true; }
	bool BuildDenseProjection(unsigned,unsigned,unsigned,DataflowProjectionProgram&)
		{ return true; }
	bool BuildDenseAttention(unsigned layer,unsigned position,unsigned mode,
		DataflowDenseAttentionProgram& p)
		{ p.layer=layer; p.position=position; p.mode=mode; return true; }
	bool BuildDenseAttentionOutput(unsigned,unsigned,DataflowProjectionProgram&)
		{ return true; }
	bool BuildProjection(unsigned layer,unsigned role,unsigned,unsigned outputSlot,
		DataflowProjectionProgram&)
	{
		if (layer==Layers && role==TiedOutput
			&& outputSlot==DataflowGraphVocabularyOutputSlot) tiedOutputBuilds++;
		return true;
	}
	bool BuildArgmax(DataflowArgmaxProgram&) { argmaxBuilds++; return true; }
	bool BuildSilu(unsigned,DataflowSiluProgram&) { return true; }
	bool BuildConv(unsigned layer,unsigned position,DataflowConvProgram& p)
		{ p.layer=layer; p.position=position; return true; }
	bool BuildDelta(unsigned layer,unsigned position,DataflowDeltaProgram& p)
		{ p.layer=layer; p.position=position; return true; }
	bool BuildRecurrentOutput(unsigned,unsigned,DataflowProjectionProgram&)
		{ return true; }
};

struct FakeDecoderService {
	unsigned embedding=0,projections=0,rms=0,silu=0,residual=0,argmax=0;
	unsigned conv=0,delta=0,denseAttention=0;
	int failConvLayer=-1;
	int io=0;

	int EmbedToken(const DataflowEmbeddingProgram&,int&) { embedding++; return 0; }
	int RunProjection(const DataflowProjectionProgram&,int&) { projections++; return 0; }
	int RunArgmax(const DataflowArgmaxProgram&,unsigned& token,int&)
		{ argmax++; token=42; return 0; }
	int RunRms(const DataflowRmsProgram&,int&) { rms++; return 0; }
	int RunSilu(const DataflowSiluProgram&,int&) { silu++; return 0; }
	int RunResidual(const DataflowResidualProgram&,int&) { residual++; return 0; }
	int RunConv(const DataflowConvProgram& p,int&)
		{ conv++; return int(p.layer)==failConvLayer?1:0; }
	int RunDelta(const DataflowDeltaProgram&,int&) { delta++; return 0; }
	int RunDenseAttention(const DataflowDenseAttentionProgram&,int&)
		{ denseAttention++; return 0; }
};

static void TestWholeMixedStack()
{
	FakeDecoderAuthority authority;
	FakeDecoderService service;
	unsigned outputToken=Vocabulary,stage=0;
	assert(DataflowDecoderSequence::Run(0,17,0,authority,service,service.io,
		outputToken,stage,sProjectionScratch)==DataflowDecoderSequenceComplete);
	assert(authority.resets==1 && outputToken==42
		&& stage==DataflowDecoderSequenceComplete);
	assert(service.embedding==1 && service.projections==249
		&& service.rms==2*Layers+1 && service.silu==Layers
		&& service.residual==4*Layers && service.conv==Layers-Layers/4
		&& service.delta==Layers-Layers/4
		&& service.denseAttention==3*(Layers/4)
		&& authority.tiedOutputBuilds==1 && authority.argmaxBuilds==1
		&& service.argmax==1);

	assert(DataflowDecoderSequence::Run(1,18,0,authority,service,service.io,
		outputToken,stage,sProjectionScratch)==DataflowDecoderSequenceComplete);
	assert(authority.resets==1 && service.embedding==2
		&& authority.tiedOutputBuilds==2 && authority.argmaxBuilds==2
		&& service.argmax==2 && outputToken==42);
}

static void TestStopsAtFirstLayerFailure()
{
	FakeDecoderAuthority authority;
	FakeDecoderService service;
	service.failConvLayer=2;
	unsigned outputToken=Vocabulary,stage=0;
	assert(DataflowDecoderSequence::Run(0,4,0,authority,service,service.io,
		outputToken,stage,sProjectionScratch)==DataflowDecoderLayerFailed);
	assert(stage==(2U<<8|DataflowRecurrentConvRunFailed)
		&& outputToken==Vocabulary && service.conv==3
		&& service.embedding==1 && service.denseAttention==0);
}

static void TestRejectsInvalidInputBeforeWork()
{
	FakeDecoderAuthority authority;
	FakeDecoderService service;
	unsigned outputToken=Vocabulary,stage=0;
	assert(DataflowDecoderSequence::Run(0,Vocabulary,0,authority,service,
		service.io,outputToken,stage,sProjectionScratch)
		==DataflowDecoderSequenceInvalidRequest);
	assert(authority.resets==0 && service.embedding==0 && outputToken==Vocabulary);
}

static void TestPromptAndGreedyGenerationLoop()
{
	const unsigned prompt[]={11,12,13};
	unsigned output[3]{},outputCount=0,stage=0;
	uint64_t promptTime=0,decodeTime=0;
	FakeGenerationClock clock;
	FakeDecoderAuthority authority;
	FakeDecoderService service;
	assert(DataflowGenerationSequence::Run(prompt,3,3,Vocabulary,0,
		authority,service,service.io,output,outputCount,stage,clock,
		promptTime,decodeTime,sProjectionScratch)
		==DataflowGenerationSequenceComplete);
	assert(outputCount==3 && output[0]==42 && output[1]==42 && output[2]==42
		&& stage==DataflowGenerationSequenceComplete);
	assert(promptTime>0 && decodeTime>0);
	assert(authority.resets==1 && authority.tiedOutputBuilds==3
		&& authority.argmaxBuilds==3 && service.argmax==3
		&& service.embedding==5 && service.projections==3*249+2*248);

	const unsigned shortPrompt[]={7};
	unsigned earlyOutput[4]{},earlyCount=0;
	uint64_t earlyPromptTime=0,earlyDecodeTime=0;
	FakeGenerationClock earlyClock;
	FakeDecoderAuthority earlyAuthority;
	FakeDecoderService earlyService;
	assert(DataflowGenerationSequence::Run(shortPrompt,1,4,42,0,
		earlyAuthority,earlyService,earlyService.io,earlyOutput,earlyCount,stage,
		earlyClock,earlyPromptTime,earlyDecodeTime,sProjectionScratch)
		==DataflowGenerationSequenceComplete);
	assert(earlyCount==1 && earlyOutput[0]==42 && earlyService.argmax==1
		&& earlyPromptTime>0 && !earlyDecodeTime);

	FakeDecoderAuthority invalidAuthority;
	FakeDecoderService invalidService;
	FakeGenerationClock invalidClock;
	assert(DataflowGenerationSequence::Run(shortPrompt,MaxPositions,2,
		Vocabulary,0,invalidAuthority,invalidService,invalidService.io,
		earlyOutput,earlyCount,stage,invalidClock,earlyPromptTime,earlyDecodeTime,
		sProjectionScratch)
		==DataflowGenerationSequenceInvalidRequest);
	assert(!invalidAuthority.resets && !invalidService.embedding);
}

int main()
{
	TestWholeMixedStack();
	TestStopsAtFirstLayerFailure();
	TestRejectsInvalidInputBeforeWork();
	TestPromptAndGreedyGenerationLoop();
	return 0;
}
