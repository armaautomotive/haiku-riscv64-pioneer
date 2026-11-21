/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRecurrentLayerSequence.h"

#include <assert.h>

using namespace PioneerQwen35;

static DataflowProjectionProgram sProjectionScratch;

enum Event {
	BuildSave=1,RunSave,BuildAttentionNorm,RunAttentionNorm,
	BuildQkv,RunQkv,BuildGate,RunGate,BuildAlpha,RunAlpha,
	BuildBeta,RunBeta,EventBuildConv,EventRunConv,EventBuildDelta,EventRunDelta,
	BuildRecurrentOut,RunRecurrentOut,BuildAttentionAdd,RunAttentionAdd,
	BuildFfnSave,RunFfnSave,BuildPostNorm,RunPostNorm,
	BuildUp,RunUp,EventBuildSilu,EventRunSilu,BuildDown,RunDown,
	BuildFfnAdd,RunFfnAdd
};

struct FakeAuthority {
	unsigned events[64]{};
	unsigned count=0;
	unsigned failBuild=0;

	bool Record(unsigned event)
	{
		events[count++]=event;
		return event!=failBuild;
	}
	bool BuildResidual(unsigned mode,unsigned slot,DataflowResidualProgram& program)
	{
		program.mode=mode;
		program.activationSlot=slot;
		return Record(mode==DataflowResidualSave
			?(slot?BuildFfnSave:BuildSave)
			:(slot?BuildAttentionAdd:BuildFfnAdd));
	}
	bool BuildRms(unsigned,unsigned role,unsigned,unsigned,DataflowRmsProgram& program)
	{
		program.role=role;
		return Record(role==AttentionNorm?BuildAttentionNorm:BuildPostNorm);
	}
	bool BuildProjection(unsigned,unsigned role,unsigned,unsigned,
		DataflowProjectionProgram& program)
	{
		program.type=role;
		return Record(role==RecurrentQkv?BuildQkv:
			role==RecurrentGate?BuildGate:
			role==Alpha?BuildAlpha:
			role==Beta?BuildBeta:
			role==Gate?BuildGate:
			role==Up?BuildUp:BuildDown);
	}
	bool BuildConv(unsigned,unsigned,DataflowConvProgram&)
		{ return Record(EventBuildConv); }
	bool BuildDelta(unsigned,unsigned,DataflowDeltaProgram&)
		{ return Record(EventBuildDelta); }
	bool BuildRecurrentOutput(unsigned,unsigned,DataflowProjectionProgram& program)
	{
		program.type=RecurrentOut;
		return Record(BuildRecurrentOut);
	}
	bool BuildSilu(unsigned,DataflowSiluProgram&)
		{ return Record(EventBuildSilu); }
};

struct FakeService {
	unsigned events[64]{};
	unsigned count=0;
	unsigned failRun=0;

	int Record(unsigned event)
	{
		events[count++]=event;
		return event==failRun?1:0;
	}
	int RunResidual(const DataflowResidualProgram& program,int&)
	{
		return Record(program.mode==DataflowResidualSave
			?(program.activationSlot?RunFfnSave:RunSave)
			:(program.activationSlot?RunAttentionAdd:RunFfnAdd));
	}
	int RunRms(const DataflowRmsProgram& program,int&)
		{ return Record(program.role==AttentionNorm?RunAttentionNorm:RunPostNorm); }
	int RunProjection(const DataflowProjectionProgram& program,int&)
	{
		return Record(program.type==RecurrentQkv?RunQkv:
			program.type==RecurrentGate?RunGate:
			program.type==Alpha?RunAlpha:
			program.type==Beta?RunBeta:
			program.type==RecurrentOut?RunRecurrentOut:
			program.type==Gate?RunGate:
			program.type==Up?RunUp:RunDown);
		}
	int RunConv(const DataflowConvProgram&,int&) { return Record(EventRunConv); }
	int RunDelta(const DataflowDeltaProgram&,int&) { return Record(EventRunDelta); }
	int RunSilu(const DataflowSiluProgram&,int&) { return Record(EventRunSilu); }
};

static void TestCompleteLayer()
{
	FakeAuthority authority;
	FakeService service;
	int io=0;
	unsigned outputSlot=ActivationBuffers,stage=0;
	assert(DataflowRecurrentLayerSequence::Run(2,17,0,authority,service,io,
		outputSlot,stage,sProjectionScratch)==DataflowRecurrentLayerComplete);
	assert(outputSlot==0 && stage==DataflowRecurrentLayerComplete);
	const unsigned expectedBuilds[]={BuildSave,BuildAttentionNorm,BuildQkv,
		BuildGate,BuildAlpha,BuildBeta,EventBuildConv,EventBuildDelta,
		BuildRecurrentOut,
		BuildAttentionAdd,BuildFfnSave,BuildPostNorm,BuildGate,BuildUp,
		EventBuildSilu,BuildDown,BuildFfnAdd};
	assert(authority.count==sizeof(expectedBuilds)/sizeof(expectedBuilds[0]));
	for (unsigned i=0;i<authority.count;i++)
		assert(authority.events[i]==expectedBuilds[i]);
	const unsigned expectedRuns[]={RunSave,RunAttentionNorm,RunQkv,RunGate,
		RunAlpha,RunBeta,EventRunConv,EventRunDelta,RunRecurrentOut,
		RunAttentionAdd,
		RunFfnSave,RunPostNorm,RunGate,RunUp,EventRunSilu,RunDown,RunFfnAdd};
	assert(service.count==sizeof(expectedRuns)/sizeof(expectedRuns[0]));
	for (unsigned i=0;i<service.count;i++) assert(service.events[i]==expectedRuns[i]);
}

static void TestInvalidAndEarlyFailure()
{
	FakeAuthority invalidAuthority;
	FakeService invalidService;
	int io=0;
	unsigned outputSlot=ActivationBuffers,stage=0;
	assert(DataflowRecurrentLayerSequence::Run(3,0,0,invalidAuthority,
		invalidService,io,outputSlot,stage,sProjectionScratch)
		==DataflowRecurrentLayerInvalidRequest);
	assert(outputSlot==ActivationBuffers && stage==DataflowRecurrentLayerInvalidRequest
		&& invalidAuthority.count==0 && invalidService.count==0);

	FakeAuthority authority;
	FakeService service;
	service.failRun=EventRunDelta;
	outputSlot=ActivationBuffers;
	assert(DataflowRecurrentLayerSequence::Run(0,0,1,authority,service,io,
		outputSlot,stage,sProjectionScratch)==DataflowRecurrentDeltaRunFailed);
	assert(outputSlot==ActivationBuffers && stage==DataflowRecurrentDeltaRunFailed);
	assert(service.count==8 && service.events[7]==EventRunDelta);
	assert(authority.count==8 && authority.events[7]==EventBuildDelta);
}

int main()
{
	TestCompleteLayer();
	TestInvalidAndEarlyFailure();
	return 0;
}
