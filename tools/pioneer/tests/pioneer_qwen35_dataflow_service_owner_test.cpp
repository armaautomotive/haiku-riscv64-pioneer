/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../PioneerQwen35DataflowServiceOwner.h"
#include "../PioneerQwen35DataflowBlackholeWorker.h"

using namespace PioneerQwen35;

static const unsigned TestRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static void Rows(RowSpace& space,unsigned rows,unsigned rowBytes,uint32_t base)
{
	space={};
	space.rows=rows;
	space.rowBytes=rowBytes;
	space.unit=16;
	const unsigned groups=rows/16;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=groups/Banks+unsigned(bank<groups%Banks);
		space.parts[bank]={bank,base+bank*0x01000000,first*16,count*16};
		space.count++;
		first+=count;
	}
}

struct MockDataflowIO {
	bool captured[DataflowCoreCount]{};
	bool held[DataflowCoreCount]{};
	bool started[DataflowCoreCount]{};
	bool originalRunning[DataflowCoreCount]{};
	bool restored[DataflowCoreCount]{};
	bool released[DataflowCoreCount]{};
	bool idle[DataflowCoreCount];
	uint32_t text[DataflowCoreCount][128]{};
	uint32_t boot[DataflowCoreCount]{};
	DataflowQueueControl queue[DataflowCoreCount][DataflowCircularBufferCount]{};
	DataflowWorkerMailbox mailbox[DataflowCoreCount]{};
	DataflowCoreInvocation invocation[DataflowCoreCount]{};
	DataflowWorkerSnapshot snapshot[DataflowCoreCount]{};
	unsigned captureCount=0,writeCount=0,startCount=0,stopCount=0,restoreCount=0,resumeCount=0;
	unsigned failStartCore=UINT32_MAX,failIdleCore=UINT32_MAX;
	bool failStopAudit=false,failResume=false;
	bool authorizeProgram=true;

	MockDataflowIO()
	{
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			idle[core]=true;
			originalRunning[core]=(core%4)!=0;
			started[core]=originalRunning[core];
			snapshot[core].state=DataflowWorkerReady;
		}
	}

	bool SessionIdle() const { return true; }
	bool ProveFleetRoutes() const
	{
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			unsigned mapped=UINT32_MAX;
			if (!DataflowBlackholeLocalCore(DataflowBlackholeRouteForCore(core),mapped)
				|| mapped!=core) return false;
		}
		return true;
	}
	bool CoreIdle(unsigned core) const
	{
		return core<DataflowCoreCount && idle[core] && core!=failIdleCore;
	}
	bool CaptureCoreState(unsigned core)
	{
		if (core>=DataflowCoreCount || captured[core]) return false;
		captured[core]=true;
		captureCount++;
		return true;
	}
	void ReleaseCoreSnapshot(unsigned core)
	{
		assert(core<DataflowCoreCount && captured[core]);
		captured[core]=false;
		released[core]=true;
	}
	bool HoldCore(unsigned core)
	{
		if (core>=DataflowCoreCount || !captured[core]) return false;
		held[core]=true;
		started[core]=false;
		return true;
	}
	bool CoreHeld(unsigned core) const { return core<DataflowCoreCount && held[core]; }

	bool WriteCoreMemory(unsigned core,unsigned address,const void* input,unsigned bytes)
	{
		if (core>=DataflowCoreCount || !held[core] || !input || !bytes) return false;
		if (address>=DataflowServiceEntryAddress
			&& address+bytes<=DataflowServiceTextLimit) {
			const unsigned offset=(address-DataflowServiceEntryAddress)/4;
			if ((address&3) || (bytes&3) || offset+bytes/4>128) return false;
			memcpy(text[core]+offset,input,bytes);
		} else if (address==DataflowServiceBootAddress && bytes==sizeof(uint32_t)) {
			memcpy(&boot[core],input,bytes);
		} else if (address==DataflowQueueStateAddress
			&& bytes==sizeof(queue[core])) {
			memcpy(queue[core],input,bytes);
		} else if (address==DataflowWorkerMailboxAddress
			&& bytes==sizeof(mailbox[core])) {
			memcpy(&mailbox[core],input,bytes);
		} else return false;
		writeCount++;
		return true;
	}
	bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes) const
	{
		if (core>=DataflowCoreCount || !held[core] || !output || !bytes) return false;
		if (address>=DataflowServiceEntryAddress
			&& address+bytes<=DataflowServiceTextLimit) {
			const unsigned offset=(address-DataflowServiceEntryAddress)/4;
			if ((address&3) || (bytes&3) || offset+bytes/4>128) return false;
			memcpy(output,text[core]+offset,bytes);
		} else if (address==DataflowServiceBootAddress && bytes==sizeof(uint32_t)) {
			memcpy(output,&boot[core],bytes);
		} else if (address==DataflowQueueStateAddress
			&& bytes==sizeof(queue[core])) {
			memcpy(output,queue[core],bytes);
		} else if (address==DataflowWorkerMailboxAddress
			&& bytes==sizeof(mailbox[core])) {
			memcpy(output,&mailbox[core],bytes);
		} else return false;
		return true;
	}
	bool StartCore(unsigned core)
	{
		if (core>=DataflowCoreCount || !held[core]
			|| boot[core]!=DataflowServiceBootWord) return false;
		if (core==failStartCore) return false;
		started[core]=true;
		startCount++;
		return true;
	}
	bool WaitWorkerReady(unsigned core,DataflowWorkerSnapshot& result)
	{
		if (core>=DataflowCoreCount || !started[core]
			|| !DataflowWorkerMailboxStartupValid(mailbox[core])) return false;
		result={mailbox[core].commandSequence,mailbox[core].acknowledgedSequence,
			mailbox[core].completedSequence,mailbox[core].state,mailbox[core].failure,
			mailbox[core].phase};
		snapshot[core]=result;
		return true;
	}
	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& result) const
	{
		if (core>=DataflowCoreCount) return false;
		result=snapshot[core];
		return true;
	}
	bool CoreReady(unsigned core,const DataflowWorkerSnapshot& observed) const
	{
		unsigned mapped=UINT32_MAX;
		return core<DataflowCoreCount && started[core]
			&& DataflowBlackholeLocalCore(DataflowBlackholeRouteForCore(core),mapped)
			&& mapped==core && DataflowWorkerSnapshotReady(observed);
	}
	bool AuthorizeProjection(const DataflowProjectionProgram& program) const
	{
		return authorizeProgram && program.Valid();
	}
	bool WriteInvocation(unsigned core,const DataflowCoreInvocation& value)
	{
		if (core>=DataflowCoreCount || value.core!=core || !value.Valid()) return false;
		invocation[core]=value;
		return true;
	}
	bool InvocationMatches(unsigned core,const DataflowCoreInvocation& value) const
	{
		return core<DataflowCoreCount
			&& memcmp(&invocation[core],&value,sizeof(value))==0;
	}
	bool PublishCommand(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount || !started[core]) return false;
		mailbox[core].commandSequence=sequence;
		snapshot[core].commandSequence=sequence;
		return true;
	}
	bool WaitForCompletion(unsigned core,unsigned sequence,DataflowWorkerSnapshot& result)
	{
		if (core>=DataflowCoreCount || mailbox[core].commandSequence!=sequence) return false;
		mailbox[core].completedSequence=sequence;
		mailbox[core].state=DataflowWorkerComplete;
		mailbox[core].phase=2;
		snapshot[core]={sequence,mailbox[core].acknowledgedSequence,sequence,
			mailbox[core].state,0,2};
		result=snapshot[core];
		return true;
	}
	bool AuditProjection(const DataflowProjectionProgram& program,unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) const
	{
		if (!program.Valid()) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (receipts[core].commandSequence!=sequence
				|| receipts[core].completedSequence!=sequence
				|| receipts[core].state!=DataflowWorkerComplete) return false;
		return true;
	}
	bool Acknowledge(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount || mailbox[core].completedSequence!=sequence
			|| mailbox[core].state!=DataflowWorkerComplete) return false;
		mailbox[core].acknowledgedSequence=sequence;
		mailbox[core].state=DataflowWorkerReady;
		mailbox[core].phase=0;
		snapshot[core]={sequence,sequence,sequence,DataflowWorkerReady,0,0};
		return true;
	}
	bool AuditAndRecoverAll(unsigned sequence,DataflowWorkerSnapshot (&)[DataflowCoreCount])
	{
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (mailbox[core].commandSequence==sequence
				&& mailbox[core].completedSequence<sequence) {
				mailbox[core].completedSequence=sequence;
				mailbox[core].state=DataflowWorkerFailed;
				mailbox[core].failure=DataflowFailureQueue;
				mailbox[core].phase=5;
				snapshot[core]={sequence,mailbox[core].acknowledgedSequence,
					sequence,DataflowWorkerFailed,DataflowFailureQueue,5};
			}
		}
		return true;
	}
	bool StopAndAuditAll()
	{
		stopCount++;
		if (failStopAudit) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (started[core]
				&& (mailbox[core].state==DataflowWorkerRunning
					|| mailbox[core].state==DataflowWorkerFatal)) return false;
			held[core]=true;
			started[core]=false;
		}
		return true;
	}
	bool RestoreCoreState(unsigned core)
	{
		if (core>=DataflowCoreCount || !captured[core] || !held[core]) return false;
		restored[core]=true;
		restoreCount++;
		return true;
	}
	bool VerifyCoreStateRestored(unsigned core) const
	{
		return core<DataflowCoreCount && restored[core];
	}
	bool ResumeRestoredFleet()
	{
		resumeCount++;
		if (failResume) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			started[core]=originalRunning[core];
			held[core]=!originalRunning[core];
		}
		return true;
	}
};

int main()
{
	uint32_t code[96];
	for (unsigned i=0;i<96;i++) code[i]=0x10000000+i;
	DataflowServiceImageView image={DataflowServiceBootWord,code,96};

	MockDataflowIO io{};
	DataflowServiceOwner owner;
	io.failIdleCore=55;
	assert(owner.Install(image,io)==DataflowServicePreflightFailed);
	assert(io.captureCount==0 && io.writeCount==0 && !owner.Faulted());
	io.failIdleCore=UINT32_MAX;
	assert(owner.Install(image,io)==DataflowServiceComplete);
	assert(owner.Ready() && io.captureCount==DataflowCoreCount
		&& io.startCount==DataflowCoreCount);
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(DataflowWorkerMailboxStartupValid(io.mailbox[core]));
		assert(io.boot[core]==DataflowServiceBootWord);
		for (unsigned i=0;i<96;i++) assert(io.text[core][i]==code[i]);
	}

	TensorSpace weight{};
	weight.type=13;
	weight.inputs=Hidden;
	Rows(weight.place,Ffn,Hidden/256
		*(PioneerQwen35Projection::TileBytes(13)/16),0x08000000);
	RowSpace input{},output{};
	Rows(input,Ffn,sizeof(float),0x04000000);
	Rows(output,Ffn,sizeof(float),0x06000000);
	DataflowProjectionProgram program{};
	assert(DataflowProgramBuilder::Build(weight,input,Hidden,output,
		TestRoutes,program));
	MockDataflowIO unauthorized{};
	DataflowServiceOwner unauthorizedOwner;
	assert(unauthorizedOwner.Install(image,unauthorized)==DataflowServiceComplete);
	const unsigned stagedWrites=unauthorized.writeCount;
	unauthorized.authorizeProgram=false;
	assert(unauthorizedOwner.RunProjection(program,unauthorized)
		==DataflowServiceProjectionFailed);
	assert(unauthorized.writeCount==stagedWrites);
	assert(unauthorizedOwner.Close(unauthorized)==DataflowServiceClosed);
	assert(owner.RunProjection(program,io)==DataflowServiceComplete);
	assert(owner.NextSequence()==1);
	assert(owner.Close(io)==DataflowServiceClosed && owner.Closed());
	assert(io.stopCount==1 && io.restoreCount==DataflowCoreCount
		&& io.resumeCount==1);
	for (unsigned core=0;core<DataflowCoreCount;core++)
		assert(io.released[core] && !io.captured[core]
			&& io.started[core]==io.originalRunning[core]);

	MockDataflowIO partial{};
	DataflowServiceOwner partialOwner;
	partial.failStartCore=63;
	assert(partialOwner.Install(image,partial)==DataflowServiceStartFailed);
	assert(partialOwner.Faulted() && partial.captureCount==DataflowCoreCount);
	assert(partialOwner.Close(partial)==DataflowServiceClosed);
	assert(partial.restoreCount==DataflowCoreCount && partial.resumeCount==1);
	for (unsigned core=0;core<DataflowCoreCount;core++)
		assert(partial.started[core]==partial.originalRunning[core]);

	MockDataflowIO unreadable{};
	DataflowServiceOwner unreadableOwner;
	assert(unreadableOwner.Install(image,unreadable)==DataflowServiceComplete);
	unreadable.failStopAudit=true;
	assert(unreadableOwner.Close(unreadable)==DataflowServiceStopAuditFailed);
	assert(unreadableOwner.Faulted() && unreadable.restoreCount==0);
	unreadable.failStopAudit=false;
	assert(unreadableOwner.Close(unreadable)==DataflowServiceClosed);
	assert(unreadable.restoreCount==DataflowCoreCount && unreadable.resumeCount==1);

	MockDataflowIO resumeFailure{};
	DataflowServiceOwner resumeFailureOwner;
	assert(resumeFailureOwner.Install(image,resumeFailure)==DataflowServiceComplete);
	resumeFailure.failResume=true;
	assert(resumeFailureOwner.Close(resumeFailure)==DataflowServiceResumeFailed);
	assert(resumeFailureOwner.Faulted() && resumeFailure.restoreCount==DataflowCoreCount);
	resumeFailure.failResume=false;
	assert(resumeFailureOwner.Close(resumeFailure)==DataflowServiceClosed);
	assert(resumeFailure.resumeCount==2);

	std::puts("PASS Qwen35 resident owner preflights 120 cores, verifies image before launch, and restores state after clean or partial launch");
	return 0;
}
