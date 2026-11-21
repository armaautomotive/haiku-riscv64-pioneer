/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowFfnSequence.h"

#include <assert.h>

using namespace PioneerQwen35;

static DataflowProjectionProgram sProjectionScratch;

enum Stage {
	EventBuildSave=1,EventRunSave,EventBuildNorm,EventRunNorm,
	EventBuildGate,EventRunGate,EventBuildUp,EventRunUp,
	EventBuildSilu,EventRunSilu,EventBuildDown,EventRunDown,
	EventBuildAdd,EventRunAdd
};

struct FakeAuthority {
	unsigned events[32]{};
	unsigned count=0;
	unsigned failBuild=0;

	bool Record(unsigned event)
	{
		events[count++]=event;
		return event!=failBuild;
	}
	bool BuildResidual(unsigned mode,unsigned,
		DataflowResidualProgram& program)
	{
		program.mode=mode;
		return Record(mode==DataflowResidualSave?EventBuildSave:EventBuildAdd);
	}
	bool BuildRms(unsigned,unsigned,unsigned,unsigned,DataflowRmsProgram&)
	{
		return Record(EventBuildNorm);
	}
	bool BuildProjection(unsigned,unsigned role,unsigned,unsigned,
		DataflowProjectionProgram&)
	{
		return Record(role==Gate?EventBuildGate:role==Up?EventBuildUp:EventBuildDown);
	}
	bool BuildSilu(unsigned,DataflowSiluProgram&)
	{
		return Record(EventBuildSilu);
	}
};

struct FakeService {
	unsigned events[32]{};
	unsigned count=0;
	unsigned failRun=0;

	int Record(unsigned event)
	{
		events[count++]=event;
		return event==failRun?22:0;
	}
	int RunResidual(const DataflowResidualProgram& program,int&)
	{
		return Record(program.mode==DataflowResidualSave?EventRunSave:EventRunAdd);
	}
	int RunRms(const DataflowRmsProgram&,int&)
	{
		return Record(EventRunNorm);
	}
	int RunProjection(const DataflowProjectionProgram&,int&)
	{
		const unsigned run=projectionRuns++;
		return Record(run==0?EventRunGate:run==1?EventRunUp:EventRunDown);
	}
	int RunSilu(const DataflowSiluProgram&,int&)
	{
		return Record(EventRunSilu);
	}
	unsigned projectionRuns=0;
};

static void TestCompleteSequence()
{
	FakeAuthority authority;
	FakeService service;
	int io=0;
	unsigned outputSlot=ActivationBuffers;
	assert(DataflowFfnSequence::Run(7,1,authority,service,io,outputSlot,
		sProjectionScratch)
		==DataflowFfnSequenceComplete);
	assert(outputSlot==0);
	const unsigned expected[]={EventBuildSave,EventBuildNorm,EventBuildGate,
		EventBuildUp,EventBuildSilu,EventBuildDown,EventBuildAdd};
	assert(authority.count==sizeof(expected)/sizeof(expected[0]));
	for (unsigned i=0;i<authority.count;i++) assert(authority.events[i]==expected[i]);
	const unsigned runs[]={EventRunSave,EventRunNorm,EventRunGate,EventRunUp,
		EventRunSilu,EventRunDown,EventRunAdd};
	assert(service.count==sizeof(runs)/sizeof(runs[0]));
	for (unsigned i=0;i<service.count;i++) assert(service.events[i]==runs[i]);
}

static void TestInvalidAndEarlyFailure()
{
	FakeAuthority invalidAuthority;
	FakeService invalidService;
	int io=0;
	unsigned outputSlot=ActivationBuffers;
	assert(DataflowFfnSequence::Run(Layers,0,invalidAuthority,invalidService,
		io,outputSlot,sProjectionScratch)==DataflowFfnSequenceInvalidRequest);
	assert(invalidAuthority.count==0 && invalidService.count==0);
	assert(outputSlot==ActivationBuffers);

	FakeAuthority authority;
	FakeService service;
	service.failRun=EventRunUp;
	outputSlot=ActivationBuffers;
	assert(DataflowFfnSequence::Run(3,0,authority,service,io,outputSlot,
		sProjectionScratch)
		==DataflowFfnUpRunFailed);
	assert(outputSlot==ActivationBuffers);
	assert(authority.count==4);
	assert(service.count==4);
	assert(service.events[3]==EventRunUp);
}

int main()
{
	TestCompleteSequence();
	TestInvalidAndEarlyFailure();
	return 0;
}
