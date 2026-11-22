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
	DataflowEmbeddingInvocation embeddingInvocation[DataflowCoreCount]{};
	DataflowRmsInvocation rmsInvocation[DataflowCoreCount]{};
	DataflowWorkerSnapshot snapshot[DataflowCoreCount]{};
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
	DataflowRoleMailbox roleMailbox[DataflowCoreCount]{};
	bool roleStaged=false,roleArmed=false,roleRestored=false;
	bool roleStarted[DataflowCoreCount]{};
	bool failRoleStage=false,failRoleArm=false,failRoleRestore=false,failRoleVerify=false;
	bool corruptPriorResidentOnBoot=false;
	unsigned corruptRoleMailboxCore=UINT32_MAX;
	unsigned roleStages=0,roleArms=0,roleStarts=0,roleStops=0,roleRestores=0,roleVerifies=0;
#endif
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	uint32_t barrierLocal[DataflowCoreCount][3*DataflowBarrierPacketBytes/4]{};
	uint32_t barrierChildren[DataflowCoreCount][DataflowBarrierReleaseFanout][DataflowBarrierArrivalStride/4]{};
	unsigned failBarrierCore=UINT32_MAX,barrierWriteCount[DataflowCoreCount]{};
	uint32_t dispatchText[DataflowCoreCount][128]{};
	unsigned dispatchWrites[DataflowCoreCount]{},dispatchReads[DataflowCoreCount]{};
	unsigned failDispatchWriteCore=UINT32_MAX,failDispatchReadCore=UINT32_MAX;
	unsigned corruptDispatchReadCore=UINT32_MAX;
#endif
	unsigned captureCount=0,writeCount=0,startCount=0,waitCount=0,stopCount=0,
		restoreCount=0,resumeCount=0;
	unsigned beginCount=0,completeCount=0;
	unsigned failStartCore=UINT32_MAX,failIdleCore=UINT32_MAX;
	unsigned failReadyCore=UINT32_MAX,failReadyPhase=1;
	unsigned failWorkerCore=UINT32_MAX;
	bool failEmbeddingCounter=false;
	uint32_t expectedEmbeddingCounter=0;
	uint32_t failureNoCState=0x00000d03U;
	uint32_t failureNoCOutstandingIdMask=0x80000001U;
	unsigned failureAddress=BankBase+0x12340,failureRoute=0x456,
		failureBytes=DataflowQ6KEmbeddingTileBytes;
	bool failStopAudit=false,failResume=false,failBegin=false,failComplete=false;
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
	bool ProveFleetRoutes(uint32_t& failedStage,uint32_t& failedCore) const
	{
		failedStage=0;
		failedCore=DataflowCoreCount;
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
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		} else if (address>=DataflowServiceDispatchAddress && address+bytes<=DataflowServiceDispatchLimit) {
			if (core==failDispatchWriteCore) return false;
			const unsigned offset=(address-DataflowServiceDispatchAddress)/4;
			if ((address&3) || (bytes&3) || offset+bytes/4>128) return false;
			memcpy(dispatchText[core]+offset,input,bytes);dispatchWrites[core]++;
#endif
		} else if (address==DataflowServiceBootAddress && bytes==sizeof(uint32_t)) {
			memcpy(&boot[core],input,bytes);
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
			if (corruptPriorResidentOnBoot && core==37) {
				text[0][0]^=1; corruptPriorResidentOnBoot=false;
			}
#endif
		} else if (address==DataflowQueueStateAddress
			&& bytes==sizeof(queue[core])) {
			memcpy(queue[core],input,bytes);
		} else if (address==DataflowWorkerMailboxAddress
			&& bytes==sizeof(mailbox[core])) {
			memcpy(&mailbox[core],input,bytes);
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
		} else if (address==DataflowRoleMailboxAddress && bytes==sizeof(roleMailbox[core])) {
			memcpy(&roleMailbox[core],input,bytes);
#endif
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		} else if (address>=DataflowBarrierOutbox && address+bytes<=DataflowBarrierReleaseCommit+64) {
			if (core==failBarrierCore) return false;
			memcpy(reinterpret_cast<uint8_t*>(barrierLocal[core])+address-DataflowBarrierOutbox,input,bytes);
			barrierWriteCount[core]++;
		} else if (address>=DataflowBarrierArrivalBase && address+bytes<=DataflowBarrierArrivalEnd) {
			if (core==failBarrierCore) return false;
			const unsigned offset=address-DataflowBarrierArrivalBase;
			const unsigned child=offset/DataflowBarrierArrivalStride,within=offset%DataflowBarrierArrivalStride;
			if (!DataflowBarrierReleaseEdge(core,child) || bytes>DataflowBarrierArrivalStride-within) return false;
			memcpy(reinterpret_cast<uint8_t*>(barrierChildren[core][child-(core*DataflowBarrierReleaseFanout+1)])+within,input,bytes);
			barrierWriteCount[core]++;
#endif
		} else return false;
		writeCount++;
		return true;
	}
	bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes)
	{
		if (core>=DataflowCoreCount || !output || !bytes) return false;
		const unsigned routeAddress=DataflowWorkerMailboxAddress
			+offsetof(DataflowWorkerMailbox,reserved);
		if (address==routeAddress && bytes==sizeof(uint32_t) && started[core]) {
			memcpy(output,&mailbox[core].reserved[0],bytes);
			return true;
		}
		if (!held[core]) return false;
		if (address>=DataflowServiceEntryAddress
			&& address+bytes<=DataflowServiceTextLimit) {
			const unsigned offset=(address-DataflowServiceEntryAddress)/4;
			if ((address&3) || (bytes&3) || offset+bytes/4>128) return false;
			memcpy(output,text[core]+offset,bytes);
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		} else if (address>=DataflowServiceDispatchAddress && address+bytes<=DataflowServiceDispatchLimit) {
			if (core==failDispatchReadCore) return false;
			const unsigned offset=(address-DataflowServiceDispatchAddress)/4;
			if ((address&3) || (bytes&3) || offset+bytes/4>128) return false;
			memcpy(output,dispatchText[core]+offset,bytes);dispatchReads[core]++;
			if (core==corruptDispatchReadCore) static_cast<uint32_t*>(output)[0]^=1;
#endif
		} else if (address==DataflowServiceBootAddress && bytes==sizeof(uint32_t)) {
			memcpy(output,&boot[core],bytes);
		} else if (address==DataflowQueueStateAddress
			&& bytes==sizeof(queue[core])) {
			memcpy(output,queue[core],bytes);
		} else if (address==DataflowWorkerMailboxAddress
			&& bytes==sizeof(mailbox[core])) {
			memcpy(output,&mailbox[core],bytes);
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
		} else if (address==DataflowRoleMailboxAddress && bytes==sizeof(roleMailbox[core])) {
			memcpy(output,&roleMailbox[core],bytes);
			if (core==corruptRoleMailboxCore) static_cast<uint32_t*>(output)[0]^=1;
#endif
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		} else if (address>=DataflowBarrierOutbox && address+bytes<=DataflowBarrierReleaseCommit+64) {
			memcpy(output,reinterpret_cast<const uint8_t*>(barrierLocal[core])+address-DataflowBarrierOutbox,bytes);
		} else if (address>=DataflowBarrierArrivalBase && address+bytes<=DataflowBarrierArrivalEnd) {
			const unsigned offset=address-DataflowBarrierArrivalBase;
			const unsigned child=offset/DataflowBarrierArrivalStride,within=offset%DataflowBarrierArrivalStride;
			if (!DataflowBarrierReleaseEdge(core,child) || bytes>DataflowBarrierArrivalStride-within) return false;
			memcpy(output,reinterpret_cast<const uint8_t*>(barrierChildren[core][child-(core*DataflowBarrierReleaseFanout+1)])+within,bytes);
#endif
		} else return false;
		return true;
	}
	bool StartCore(unsigned core)
	{
		if (core>=DataflowCoreCount || !held[core]
			|| boot[core]!=DataflowServiceBootWord) return false;
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		for (unsigned i=0;i<DataflowCoreCount;i++) {
			const unsigned first=i*DataflowBarrierReleaseFanout+1;
			const unsigned children=first>=DataflowCoreCount?0
				:(DataflowCoreCount-first<DataflowBarrierReleaseFanout?DataflowCoreCount-first:DataflowBarrierReleaseFanout);
			if (barrierWriteCount[i]!=3+2*children) return false;
		}
#endif
		if (core==failStartCore) return false;
		started[core]=true;
		startCount++;
		return true;
	}
	bool WaitWorkerReady(unsigned core,DataflowWorkerSnapshot& result)
	{
		waitCount++;
		if (core>=DataflowCoreCount || !started[core]) return false;
		if (core==failReadyCore) {
			mailbox[core].state=DataflowWorkerFatal;
			mailbox[core].failure=DataflowFailureQueue;
			mailbox[core].phase=failReadyPhase;
			if (failReadyPhase==2) mailbox[core].reserved[0]=0x12345678;
			result={mailbox[core].commandSequence,
				mailbox[core].acknowledgedSequence,
				mailbox[core].completedSequence,mailbox[core].state,
				mailbox[core].failure,mailbox[core].phase,0,0};
			snapshot[core]=result;
			return false;
		}
		result={mailbox[core].commandSequence,mailbox[core].acknowledgedSequence,
			mailbox[core].completedSequence,mailbox[core].state,mailbox[core].failure,
			mailbox[core].phase,0,0};
		snapshot[core]=result;
		return result.state==DataflowWorkerReady;
	}
	bool ReadWorkerSnapshot(unsigned core,DataflowWorkerSnapshot& result) const
	{
		if (core>=DataflowCoreCount) return false;
		result=snapshot[core];
		return true;
	}
	bool ReadWorkerFailureExpected(unsigned core,unsigned& expected) const
	{
		if (!failEmbeddingCounter || core!=failWorkerCore) return false;
		expected=expectedEmbeddingCounter;
		return true;
	}
	bool ReadWorkerFailureNoC(unsigned core,unsigned& state,
		unsigned& outstandingIdMask) const
	{
		if (!failEmbeddingCounter || core!=failWorkerCore) return false;
		state=failureNoCState;
		outstandingIdMask=failureNoCOutstandingIdMask;
		return true;
	}
	bool ReadWorkerFailureRequest(unsigned core,unsigned& address,
		unsigned& route,unsigned& bytes) const
	{
		if (!failEmbeddingCounter || core!=failWorkerCore) return false;
		address=failureAddress;
		route=failureRoute;
		bytes=failureBytes;
		return true;
	}
	bool CoreReady(unsigned core,const DataflowWorkerSnapshot& observed) const
	{
		return core<DataflowCoreCount && started[core]
			&& mailbox[core].logicalCore==core
			&& DataflowWorkerSnapshotReady(observed);
	}
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
	bool SharedCaptureAndStageHeld() {
		++roleStages;
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (!held[core] || !captured[core]) return false;
		roleStaged=true;
		return !failRoleStage;
	}
	bool SharedArmAndBeginRelease() {
		++roleArms;
		if (!roleStaged || roleArmed || failRoleArm) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (!held[core] || roleMailbox[core].magic!=DataflowRoleMailboxMagic
				|| roleMailbox[core].version!=DataflowRoleMailboxVersion) return false;
		roleArmed=true; return true;
	}
	bool SharedFleetHeldAndQuiescent() const {
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (!held[core] || roleStarted[core]) return false;
		return true;
	}
	bool SharedStartCore(unsigned core) {
		if (!roleArmed || core>=DataflowCoreCount || roleStarted[core] || !StartCore(core)) return false;
		roleStarted[core]=true; ++roleStarts; return true;
	}
	bool SharedCoreReady(unsigned core,const DataflowWorkerSnapshot& observed) const {
		return core<DataflowCoreCount && roleStarted[core] && CoreReady(core,observed);
	}
	bool SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
		++roleStops;
		return StopAndAuditAll(stage,core);
	}
	bool SharedRestoreRolesHeld() {
		++roleRestores;
		for (unsigned core=0;core<DataflowCoreCount;core++) if (!held[core]) return false;
		if (failRoleRestore) return false;
		roleRestored=true; return true;
	}
	bool SharedVerifyRolesAfterResidentRestore() {
		++roleVerifies;
		for (unsigned core=0;core<DataflowCoreCount;core++) if (!held[core] || !restored[core]) return false;
		return roleRestored && !failRoleVerify;
	}
#endif
	bool AuthorizeProjection(const DataflowProjectionProgram& program) const
	{
		return authorizeProgram && program.Valid();
	}
	bool BeginProjection(const DataflowProjectionProgram& program)
	{
		beginCount++;
		return !failBegin && program.Valid();
	}
	bool CompleteProjection(const DataflowProjectionProgram& program,unsigned sequence)
	{
		completeCount++;
		return !failComplete && sequence && program.Valid();
	}
	bool AuthorizeEmbedding(const DataflowEmbeddingProgram& program) const
	{
		return authorizeProgram && program.Valid();
	}
	bool BeginEmbedding(const DataflowEmbeddingProgram& program)
	{
		beginCount++;
		return !failBegin && program.Valid();
	}
	bool CompleteEmbedding(const DataflowEmbeddingProgram& program,unsigned sequence)
	{
		completeCount++;
		return !failComplete && sequence && program.Valid();
	}
	bool AuthorizeRms(const DataflowRmsProgram& program) const
	{
		return authorizeProgram && program.Valid();
	}
	bool BeginRms(const DataflowRmsProgram& program)
	{
		beginCount++;
		return !failBegin && program.Valid();
	}
	bool CompleteRms(const DataflowRmsProgram& program,unsigned sequence)
	{
		completeCount++;
		return !failComplete && sequence && program.Valid();
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
	bool WriteEmbeddingInvocation(unsigned core,const DataflowEmbeddingInvocation& value)
	{
		if (core>=DataflowCoreCount || value.core!=core || !value.Valid()) return false;
		embeddingInvocation[core]=value;
		return true;
	}
	bool EmbeddingInvocationMatches(unsigned core,
		const DataflowEmbeddingInvocation& value) const
	{
		return core<DataflowCoreCount
			&& memcmp(&embeddingInvocation[core],&value,sizeof(value))==0;
	}
	bool WriteRmsInvocation(unsigned core,const DataflowRmsInvocation& value)
	{
		if (core>=DataflowCoreCount || value.core!=core || !value.Valid()) return false;
		rmsInvocation[core]=value;
		return true;
	}
	bool RmsInvocationMatches(unsigned core,const DataflowRmsInvocation& value) const
	{
		return core<DataflowCoreCount
			&& memcmp(&rmsInvocation[core],&value,sizeof(value))==0;
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
		mailbox[core].state=core==failWorkerCore
			?DataflowWorkerFailed:DataflowWorkerComplete;
		mailbox[core].failure=core==failWorkerCore
			?(failEmbeddingCounter?DataflowFailureEmbeddingWeightRead:DataflowFailureReader):0;
		mailbox[core].phase=core==failWorkerCore?5:2;
		snapshot[core]={sequence,mailbox[core].acknowledgedSequence,sequence,
			mailbox[core].state,mailbox[core].failure,mailbox[core].phase,0,0};
		if (core==failWorkerCore && failEmbeddingCounter) {
			snapshot[core].failureDetail=DataflowNocDiagnosticCode(
				DataflowNocDiagnostic{DataflowNocTransferCounterTimeout,0x80,0x29066,0x29067});
			snapshot[core].failureStatus=0x00029066U;
		}
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
	bool AuditEmbedding(const DataflowEmbeddingProgram& program,unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) const
	{
		if (!program.Valid()) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowEmbeddingInvocation expected{};
			if (receipts[core].state!=DataflowWorkerComplete
				|| receipts[core].commandSequence!=sequence
				|| receipts[core].completedSequence!=sequence
				|| !DataflowEmbeddingProgramBuilder::BindCore(program,core,expected)
				|| memcmp(&expected,&embeddingInvocation[core],sizeof(expected))!=0)
				return false;
		}
		return true;
	}
	bool AuditRms(const DataflowRmsProgram& program,unsigned sequence,
		const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) const
	{
		if (!program.Valid()) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowRmsInvocation expected;
			if (receipts[core].state!=DataflowWorkerComplete
				|| receipts[core].commandSequence!=sequence
				|| receipts[core].completedSequence!=sequence
				|| !DataflowRmsProgramBuilder::BindCore(program,core,expected)
				|| memcmp(&expected,&rmsInvocation[core],sizeof(expected))!=0)
				return false;
		}
		return true;
	}
	bool Acknowledge(unsigned core,unsigned sequence)
	{
		if (core>=DataflowCoreCount || mailbox[core].completedSequence!=sequence
			|| mailbox[core].state!=DataflowWorkerComplete) return false;
		mailbox[core].acknowledgedSequence=sequence;
		mailbox[core].state=DataflowWorkerReady;
		mailbox[core].phase=0;
		snapshot[core]={sequence,sequence,sequence,DataflowWorkerReady,0,0,0,0};
		return true;
	}
	bool AuditAndRecoverAll(unsigned sequence,
		DataflowWorkerSnapshot (&snapshots)[DataflowCoreCount],
		unsigned& failureStage,unsigned& failureCore,unsigned& failureDetail)
	{
		failureStage=DataflowRecoveryAuditNone;
		failureCore=DataflowCoreCount;
		failureDetail=0;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (mailbox[core].commandSequence==sequence) {
				if (mailbox[core].completedSequence<sequence) {
					mailbox[core].completedSequence=sequence;
					mailbox[core].state=DataflowWorkerFailed;
					mailbox[core].failure=DataflowFailureQueue;
					mailbox[core].phase=5;
				}
				if (mailbox[core].state!=DataflowWorkerComplete
					&& mailbox[core].state!=DataflowWorkerFailed) return false;
				mailbox[core].acknowledgedSequence=sequence;
				mailbox[core].state=DataflowWorkerReady;
				mailbox[core].failure=0;
				mailbox[core].phase=0;
			}
			snapshot[core]={mailbox[core].commandSequence,
				mailbox[core].acknowledgedSequence,mailbox[core].completedSequence,
				mailbox[core].state,mailbox[core].failure,mailbox[core].phase,0,0};
			snapshots[core]=snapshot[core];
		}
		return true;
	}
	bool AbortPendingProjection() { return true; }
	bool StopAndAuditAll(unsigned& failedStage,unsigned& failedCore)
	{
		stopCount++;
		failedStage=0;
		failedCore=DataflowCoreCount;
		if (failStopAudit) {
			failedStage=4;
			failedCore=17;
			return false;
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (started[core] && mailbox[core].state==DataflowWorkerFatal
				&& (mailbox[core].commandSequence
					|| mailbox[core].acknowledgedSequence
					|| mailbox[core].completedSequence
					|| mailbox[core].shutdownSequence
					|| mailbox[core].failure!=DataflowFailureQueue
					|| (mailbox[core].phase!=1 && mailbox[core].phase!=2)))
				return false;
			if (started[core] && mailbox[core].state==DataflowWorkerRunning)
				return false;
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
	bool ResumeRestoredFleet(unsigned& failedStage,unsigned& failedCore,
		unsigned& failedDetail)
	{
		failedStage=0;
		failedCore=DataflowCoreCount;
		failedDetail=0;
		resumeCount++;
		if (failResume) {
			failedStage=2;
			failedCore=0;
			failedDetail=DataflowServiceResumeResetMismatch;
			return false;
		}
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			started[core]=originalRunning[core];
			held[core]=!originalRunning[core];
		}
		return true;
	}
};

static DeviceLayout MakeEmbeddingLayout()
{
	DeviceLayout model{};
	model.magic=LayoutMagic;
	model.version=LayoutVersion;
	model.context=64;
	model.tied.type=14;
	model.tied.inputs=Hidden;
	model.tied.place.rows=Vocabulary;
	model.tied.place.rowBytes=2120;
	model.tied.place.count=Banks;
	model.tied.place.unit=16;
	const unsigned groups=Vocabulary/16;
	const unsigned base=groups/Banks,extra=groups%Banks;
	unsigned firstRow=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned rows=(base+(bank<extra))*16;
		model.tied.place.parts[bank]={bank,BankBase,firstRow,rows};
		firstRow+=rows;
	}
	for (unsigned slot=0;slot<ActivationBuffers;slot++) {
		auto& output=model.activation[slot];
		output.rows=9216;
		output.rowBytes=sizeof(float);
		output.count=1;
		output.unit=16;
		output.parts[0]={slot?5U:6U,BankBase+0x20000000,0,output.rows};
	}
	return model;
}

static DeviceLayout MakeRmsLayout()
{
	DeviceLayout model{};
	model.magic=LayoutMagic;
	model.version=LayoutVersion;
	model.context=64;
	TensorSpace& norm=model.layers[0].weights[AttentionNorm];
	norm.type=0;
	norm.inputs=Hidden;
	norm.place.rows=1;
	norm.place.rowBytes=Hidden*sizeof(float);
	norm.place.count=1;
	norm.place.unit=1;
	norm.place.parts[0]={1,BankBase+0x10000000,0,1};
	for (unsigned slot=0;slot<ActivationBuffers;slot++) {
		auto& output=model.activation[slot];
		output.rows=Ffn;
		output.rowBytes=sizeof(float);
		output.count=1;
		output.unit=16;
		output.parts[0]={slot?5U:6U,BankBase+(slot?0x30000000:0x20000000),
			0,output.rows};
	}
	return model;
}

int main()
{
	DataflowWorkerMailbox receipt{};
	receipt.logicalCore=119;
	for (unsigned i=0;i<6;i++) receipt.reserved[i]=i+1;
	DataflowWorkerClearReceipt(receipt);
	for (unsigned i=0;i<6;i++) assert(receipt.reserved[i]==0);
	assert(receipt.logicalCore==119);

	uint32_t code[96];
	for (unsigned i=0;i<96;i++) code[i]=0x10000000+i;
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	uint32_t dispatchCode[80];
	for (unsigned i=0;i<80;i++) dispatchCode[i]=0x20000000+i;
	DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatchCode,80};
#else
	DataflowServiceImageView image={DataflowServiceBootWord,code,96};
#endif
	assert(DataflowServicePcInImage(DataflowServiceEntryAddress,96,80));
	assert(DataflowServicePcInImage(DataflowServiceDispatchAddress,96,80));
	assert(!DataflowServicePcInImage(DataflowServiceEntryAddress+96*4,96,80));
	assert(!DataflowServicePcInImage(DataflowServiceDispatchAddress+80*4,96,80));
	assert(!DataflowServicePcInImage(DataflowServiceDispatchAddress+1,96,80));
	assert(!DataflowServicePcInImage(DataflowServiceDispatchAddress,96,0));
	assert(!DataflowServicePcInImage(DataflowServiceEntryAddress,0,80));
	assert(!DataflowServicePcInImage(DataflowServiceEntryAddress,96,
		(DataflowServiceDispatchLimit-DataflowServiceDispatchAddress)/4+1));

	MockDataflowIO io{};
	DataflowServiceOwner owner;
	io.failIdleCore=55;
	assert(owner.Install(image,io)==DataflowServicePreflightFailed);
	assert(io.captureCount==0 && io.writeCount==0 && !owner.Faulted());
	io.failIdleCore=UINT32_MAX;
	assert(owner.Install(image,io)==DataflowServiceComplete);
	assert(owner.Ready() && io.captureCount==DataflowCoreCount
		&& io.startCount==DataflowCoreCount);
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	DataflowServiceImageView invalidImage=image;invalidImage.dispatchWords=nullptr;
	MockDataflowIO invalidIo{};DataflowServiceOwner invalidOwner;
	assert(invalidOwner.Install(invalidImage,invalidIo)==DataflowServiceInvalidImage
		&& !invalidIo.captureCount && !invalidIo.writeCount);
	invalidImage=image;invalidImage.dispatchWordCount=(DataflowServiceDispatchLimit-DataflowServiceDispatchAddress)/4+1;
	assert(invalidOwner.Install(invalidImage,invalidIo)==DataflowServiceInvalidImage && !invalidIo.captureCount);
	MockDataflowIO dispatchFailure{};DataflowServiceOwner dispatchFailureOwner;
	dispatchFailure.failDispatchWriteCore=2;
	assert(dispatchFailureOwner.Install(image,dispatchFailure)==DataflowServiceImageWriteFailed
		&& dispatchFailureOwner.Faulted() && dispatchFailure.captureCount==DataflowCoreCount
		&& !dispatchFailure.startCount && !dispatchFailure.restoreCount);
	MockDataflowIO readbackFailure{};DataflowServiceOwner readbackFailureOwner;
	readbackFailure.failDispatchReadCore=2;
	assert(readbackFailureOwner.Install(image,readbackFailure)==DataflowServiceImageWriteFailed
		&& readbackFailureOwner.Faulted() && !readbackFailure.startCount && !readbackFailure.restoreCount);
	MockDataflowIO corruptReadback{};DataflowServiceOwner corruptReadbackOwner;
	corruptReadback.corruptDispatchReadCore=2;
	assert(corruptReadbackOwner.Install(image,corruptReadback)==DataflowServiceImageWriteFailed
		&& corruptReadbackOwner.Faulted() && !corruptReadback.startCount && !corruptReadback.restoreCount);
	MockDataflowIO setupFailure{};
	DataflowServiceOwner setupFailureOwner;
	setupFailure.failBarrierCore=2;
	assert(setupFailureOwner.Install(image,setupFailure)==DataflowServiceStartupWriteFailed
		&& setupFailureOwner.Faulted() && setupFailure.captureCount==DataflowCoreCount
		&& setupFailure.startCount==0 && setupFailure.restoreCount==0);
#endif
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(DataflowWorkerMailboxStartupValid(io.mailbox[core]));
		assert(io.boot[core]==DataflowServiceBootWord);
		for (unsigned i=0;i<96;i++) assert(io.text[core][i]==code[i]);
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
		assert(io.dispatchWrites[core]==2 && io.dispatchReads[core]==2);
		for (unsigned i=0;i<80;i++) assert(io.dispatchText[core][i]==dispatchCode[i]);
#endif
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
	assert(!unauthorized.beginCount && !unauthorized.completeCount);
	assert(unauthorizedOwner.Close(unauthorized)==DataflowServiceClosed);
	assert(owner.RunProjection(program,io)==DataflowServiceComplete);
	assert(owner.NextSequence()==1);
	assert(io.beginCount==1 && io.completeCount==1);
	DataflowEmbeddingProgram embedding{};
	assert(DataflowEmbeddingProgramBuilder::Build(MakeEmbeddingLayout(),15,0,
		TestRoutes,embedding));
	assert(owner.EmbedToken(embedding,io)==DataflowServiceComplete);
	assert(owner.NextSequence()==2);
	assert(io.beginCount==2 && io.completeCount==2);
	DataflowRmsProgram rms{};
	assert(DataflowRmsProgramBuilder::Build(MakeRmsLayout(),0,AttentionNorm,
		0,1,TestRoutes,rms));
	assert(owner.RunRms(rms,io)==DataflowServiceComplete);
	assert(owner.NextSequence()==3);
	assert(io.beginCount==3 && io.completeCount==3);
	assert(owner.Close(io)==DataflowServiceClosed && owner.Closed());
	assert(io.stopCount==1 && io.restoreCount==DataflowCoreCount
		&& io.resumeCount==1);
	for (unsigned core=0;core<DataflowCoreCount;core++)
		assert(io.released[core] && !io.captured[core]
			&& io.started[core]==io.originalRunning[core]);

	MockDataflowIO embeddingFailure{};
	DataflowServiceOwner embeddingFailureOwner;
	assert(embeddingFailureOwner.Install(image,embeddingFailure)
		==DataflowServiceComplete);
	embeddingFailure.failWorkerCore=7;
	embeddingFailure.failEmbeddingCounter=true;
	embeddingFailure.expectedEmbeddingCounter=0x00029067U;
	embeddingFailure.failureNoCState=0x00000d03U;
	embeddingFailure.failureNoCOutstandingIdMask=0x80000001U;
	assert(embeddingFailureOwner.EmbedToken(embedding,embeddingFailure)
		==DataflowServiceEmbeddingFailed);
	const auto& embeddingDiagnostic=embeddingFailureOwner.OwnerDiagnostic();
	assert(embeddingFailureOwner.Faulted()
		&& embeddingDiagnostic.operation==DataflowOwnerOperationEmbedding
		&& embeddingDiagnostic.result==DataflowOwnerWorkerFailed
		&& embeddingDiagnostic.core==7
		&& embeddingDiagnostic.pendingSequence==1
		&& embeddingDiagnostic.worker.state==DataflowWorkerFailed
		&& embeddingDiagnostic.worker.failure==DataflowFailureEmbeddingWeightRead
		&& embeddingDiagnostic.worker.phase==5
		&& embeddingDiagnostic.failureExpected==0x00029067U
		&& embeddingDiagnostic.failureNoCState==0x00000d03U
		&& embeddingDiagnostic.failureNoCOutstandingIdMask==0x80000001U
		&& embeddingDiagnostic.failureAddress==embeddingFailure.failureAddress
		&& embeddingDiagnostic.failureRoute==embeddingFailure.failureRoute
		&& embeddingDiagnostic.failureBytes==embeddingFailure.failureBytes);
	assert(embeddingFailureOwner.RecoverProjection(embeddingFailure)
		==DataflowServiceRecovered);
	assert(embeddingFailureOwner.Ready()
		&& !embeddingFailureOwner.Faulted()
		&& embeddingFailureOwner.NextSequence()==1);
	assert(embeddingFailureOwner.Close(embeddingFailure)==DataflowServiceClosed);
	assert(embeddingFailure.stopCount==1
		&& embeddingFailure.restoreCount==DataflowCoreCount
		&& embeddingFailure.resumeCount==1);

	MockDataflowIO partial{};
	DataflowServiceOwner partialOwner;
	partial.failStartCore=63;
	assert(partialOwner.Install(image,partial)==DataflowServiceStartFailed);
	assert(partialOwner.Faulted() && partial.captureCount==DataflowCoreCount);
	assert(partialOwner.Close(partial)==DataflowServiceClosed);
	assert(partial.restoreCount==DataflowCoreCount && partial.resumeCount==1);
	for (unsigned core=0;core<DataflowCoreCount;core++)
		assert(partial.started[core]==partial.originalRunning[core]);

	MockDataflowIO startupFailure{};
	DataflowServiceOwner startupFailureOwner;
	startupFailure.failReadyCore=29;
	startupFailure.failReadyPhase=2;
	assert(startupFailureOwner.Install(image,startupFailure)
		==DataflowServiceReadyFailed);
	const auto& readyDiagnostic=startupFailureOwner.Diagnostic();
	assert(readyDiagnostic.stage==DataflowServiceDiagnosticWorkerWait
		&& readyDiagnostic.core==29 && readyDiagnostic.state==DataflowWorkerFatal
		&& readyDiagnostic.failure==DataflowFailureQueue
		&& readyDiagnostic.phase==2 && readyDiagnostic.detail==0x12345678);
	assert(startupFailure.waitCount==DataflowCoreCount
		&& startupFailure.startCount==DataflowCoreCount);
	assert(startupFailureOwner.Close(startupFailure)==DataflowServiceClosed);
	assert(startupFailure.restoreCount==DataflowCoreCount
		&& startupFailure.resumeCount==1);

	MockDataflowIO unsafeStartupFailure{};
	DataflowServiceOwner unsafeStartupFailureOwner;
	unsafeStartupFailure.failReadyCore=29;
	unsafeStartupFailure.failReadyPhase=3;
	assert(unsafeStartupFailureOwner.Install(image,unsafeStartupFailure)
		==DataflowServiceReadyFailed);
	assert(unsafeStartupFailureOwner.Close(unsafeStartupFailure)
		==DataflowServiceStopAuditFailed);
	assert(unsafeStartupFailure.restoreCount==0
		&& unsafeStartupFailure.resumeCount==0);

	MockDataflowIO unreadable{};
	DataflowServiceOwner unreadableOwner;
	assert(unreadableOwner.Install(image,unreadable)==DataflowServiceComplete);
	unreadable.failStopAudit=true;
	assert(unreadableOwner.Close(unreadable)==DataflowServiceStopAuditFailed);
	assert(unreadableOwner.Faulted() && unreadable.restoreCount==0);
	assert(unreadableOwner.Diagnostic().stage==DataflowServiceDiagnosticStopAudit
		&& unreadableOwner.Diagnostic().core==17
		&& unreadableOwner.Diagnostic().state==4);
	unreadable.failStopAudit=false;
	assert(unreadableOwner.Close(unreadable)==DataflowServiceClosed);
	assert(unreadable.restoreCount==DataflowCoreCount && unreadable.resumeCount==1);

	MockDataflowIO resumeFailure{};
	DataflowServiceOwner resumeFailureOwner;
	assert(resumeFailureOwner.Install(image,resumeFailure)==DataflowServiceComplete);
	resumeFailure.failResume=true;
	assert(resumeFailureOwner.Close(resumeFailure)==DataflowServiceResumeFailed);
	assert(resumeFailureOwner.Faulted() && resumeFailure.restoreCount==DataflowCoreCount);
	assert(resumeFailureOwner.Diagnostic().detail==DataflowServiceResumeResetMismatch);
	resumeFailure.failResume=false;
	assert(resumeFailureOwner.Close(resumeFailure)==DataflowServiceClosed);
	assert(resumeFailure.resumeCount==2 && resumeFailure.stopCount==1
		&& resumeFailure.restoreCount==DataflowCoreCount);

	std::puts("PASS Qwen35 resident owner preflights 120 cores, verifies image before launch, and restores state after clean or partial launch");
	return 0;
}
