/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowDenseLayerSequence.h"

#include <assert.h>

using namespace PioneerQwen35;

static DataflowProjectionProgram sProjectionScratch;

enum Event {
	BuildSave=1,RunSave,BuildNorm,RunNorm,BuildQuery,RunQuery,BuildKey,RunKey,
	BuildPackKey,RunPackKey,BuildValue,RunValue,BuildPackValue,RunPackValue,
	BuildAttention,RunAttention,BuildAttentionOut,RunAttentionOut,BuildAdd,RunAdd,
	BuildGate,RunGate,BuildUp,RunUp,BuildSiluEvent,RunSiluEvent,BuildDown,RunDown
};

struct FakeAuthority {
	unsigned events[64]{};
	unsigned count=0,failBuild=0;

	bool Record(unsigned event)
	{
		events[count++]=event;
		return event!=failBuild;
	}
	bool BuildResidual(unsigned mode,unsigned,DataflowResidualProgram& program)
	{
		program.mode=mode;
		return Record(mode==DataflowResidualSave?BuildSave:BuildAdd);
	}
	bool BuildRms(unsigned,unsigned,unsigned,unsigned,DataflowRmsProgram&)
		{ return Record(BuildNorm); }
	bool BuildDenseProjection(unsigned,unsigned role,unsigned,
		DataflowProjectionProgram& program)
	{
		(void)program;
		return Record(role==QueryAndGate?BuildQuery:role==Key?BuildKey:BuildValue);
	}
	bool BuildDenseAttention(unsigned,unsigned,unsigned mode,
		DataflowDenseAttentionProgram& program)
	{
		program.mode=mode;
		return Record(mode==DataflowDenseAttentionPackKey?BuildPackKey
			:mode==DataflowDenseAttentionPackValue?BuildPackValue:BuildAttention);
	}
	bool BuildDenseAttentionOutput(unsigned,unsigned,
		DataflowProjectionProgram& program)
	{
		(void)program;
		return Record(BuildAttentionOut);
	}
	bool BuildProjection(unsigned,unsigned role,unsigned,unsigned,
		DataflowProjectionProgram& program)
	{
		(void)program;
		return Record(role==Gate?BuildGate:role==Up?BuildUp:BuildDown);
	}
	bool BuildSilu(unsigned,DataflowSiluProgram&)
		{ return Record(BuildSiluEvent); }
};

struct FakeService {
	unsigned events[64]{};
	unsigned count=0,failRun=0,projectionRuns=0;

	int Record(unsigned event)
	{
		events[count++]=event;
		return event==failRun?1:0;
	}
	int RunResidual(const DataflowResidualProgram& program,int&)
		{ return Record(program.mode==DataflowResidualSave?RunSave:RunAdd); }
	int RunRms(const DataflowRmsProgram&,int&) { return Record(RunNorm); }
	int RunProjection(const DataflowProjectionProgram& program,int&)
	{
		(void)program;
		const unsigned runs[]={RunQuery,RunKey,RunValue,RunAttentionOut,
			RunGate,RunUp,RunDown};
		return Record(runs[projectionRuns++]);
	}
	int RunDenseAttention(const DataflowDenseAttentionProgram& program,int&)
		{ return Record(program.mode==DataflowDenseAttentionPackKey?RunPackKey
			:program.mode==DataflowDenseAttentionPackValue?RunPackValue:RunAttention); }
	int RunSilu(const DataflowSiluProgram&,int&) { return Record(RunSiluEvent); }
};

static void TestCompleteSequence()
{
	FakeAuthority authority;
	FakeService service;
	int io=0;
	unsigned outputSlot=ActivationBuffers,stage=0;
	assert(DataflowDenseLayerSequence::Run(3,7,0,authority,service,io,
		outputSlot,stage,sProjectionScratch)==DataflowDenseLayerComplete);
	assert(outputSlot==0 && stage==DataflowDenseLayerComplete);
	const unsigned expectedBuilds[]={BuildSave,BuildNorm,BuildQuery,BuildKey,
		BuildPackKey,BuildValue,BuildPackValue,BuildAttention,BuildAttentionOut,
		BuildAdd,BuildSave,BuildNorm,BuildGate,BuildUp,BuildSiluEvent,BuildDown,BuildAdd};
	const unsigned expectedRuns[]={RunSave,RunNorm,RunQuery,RunKey,RunPackKey,
		RunValue,RunPackValue,RunAttention,RunAttentionOut,RunAdd,RunSave,
		RunNorm,RunGate,RunUp,RunSiluEvent,RunDown,RunAdd};
	assert(authority.count==sizeof(expectedBuilds)/sizeof(expectedBuilds[0]));
	assert(service.count==sizeof(expectedRuns)/sizeof(expectedRuns[0]));
	for (unsigned i=0;i<authority.count;i++) assert(authority.events[i]==expectedBuilds[i]);
	for (unsigned i=0;i<service.count;i++) assert(service.events[i]==expectedRuns[i]);
}

static void TestRejectAndStopOnFailure()
{
	FakeAuthority invalidAuthority;
	FakeService invalidService;
	int io=0;
	unsigned outputSlot=ActivationBuffers,stage=0;
	assert(DataflowDenseLayerSequence::Run(2,0,0,invalidAuthority,
		invalidService,io,outputSlot,stage,sProjectionScratch)
		==DataflowDenseLayerInvalidRequest);
	assert(invalidAuthority.count==0 && invalidService.count==0);

	FakeAuthority authority;
	FakeService service;
	service.failRun=RunPackKey;
	outputSlot=ActivationBuffers;
	assert(DataflowDenseLayerSequence::Run(3,0,0,authority,service,io,
		outputSlot,stage,sProjectionScratch)==DataflowDensePackKeyRunFailed);
	assert(stage==DataflowDensePackKeyRunFailed && outputSlot==ActivationBuffers);
	assert(service.count==5 && service.events[4]==RunPackKey);
}

int main()
{
	TestCompleteSequence();
	TestRejectAndStopOnFailure();
	return 0;
}
