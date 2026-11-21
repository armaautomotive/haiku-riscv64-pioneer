/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_QWEN35_DATAFLOW_REWRITE

static bool
PioneerQwen35DataflowReadStableMailbox(PioneerSessionIO& io,unsigned core,
	PioneerQwen35::DataflowWorkerMailbox& mailbox)
{
	constexpr unsigned Words=sizeof(PioneerQwen35::DataflowWorkerMailbox)/sizeof(uint32_t);
	uint32_t first[Words],second[Words];
	if (!io.SelectRuntimeCore(core) || !io.MapQ38Panel()
		|| !io.DataflowPanelMappingStable()) return false;
	for (unsigned i=0;i<Words;i++)
		first[i]=io.Q38PanelRead(PioneerQwen35::DataflowWorkerMailboxAddress+i*4);
	io.Pause();
	for (unsigned i=0;i<Words;i++)
		second[i]=io.Q38PanelRead(PioneerQwen35::DataflowWorkerMailboxAddress+i*4);
	if (memcmp(first,second,sizeof(first))!=0) return false;
	memcpy(&mailbox,second,sizeof(mailbox));
	return true;
}

static bool
PioneerQwen35DataflowQueuesDrained(PioneerSessionIO& io,unsigned core)
{
	constexpr unsigned Words=PioneerQwen35::DataflowQueueStateBytes/sizeof(uint32_t);
	uint32_t first[Words],second[Words];
	if (!io.SelectRuntimeCore(core) || !io.MapQ38Panel()
		|| !io.DataflowPanelMappingStable()) return false;
	for (unsigned i=0;i<Words;i++)
		first[i]=io.Q38PanelRead(PioneerQwen35::DataflowQueueStateAddress+i*4);
	io.Pause();
	for (unsigned i=0;i<Words;i++)
		second[i]=io.Q38PanelRead(PioneerQwen35::DataflowQueueStateAddress+i*4);
	if (memcmp(first,second,sizeof(first))!=0) return false;
	for (unsigned value:second) if (value) return false;
	return true;
}

static bool
PioneerQwen35DataflowServicePc(PioneerSessionIO& io)
{
	const unsigned end=PioneerQwen35::DataflowServiceEntryAddress
		+sizeof(PioneerQwen35::DataflowServiceTextWords);
	io.Pause();
	const uint32_t first=io.RuntimePcRead();
	io.Pause();
	const uint32_t second=io.RuntimePcRead();
	return first>=PioneerQwen35::DataflowServiceEntryAddress && first<end
		&& second>=PioneerQwen35::DataflowServiceEntryAddress && second<end;
}

static bool
PioneerQwen35DataflowQueuesBalanced(PioneerSessionIO& io,unsigned core,
	bool allowFailures)
{
	constexpr unsigned Words=PioneerQwen35::DataflowQueueStateBytes/sizeof(uint32_t);
	uint32_t first[Words],second[Words];
	if (!io.SelectRuntimeCore(core) || !io.MapQ38Panel()
		|| !io.DataflowPanelMappingStable()) return false;
	for (unsigned i=0;i<Words;i++)
		first[i]=io.Q38PanelRead(PioneerQwen35::DataflowQueueStateAddress+i*4);
	io.Pause();
	for (unsigned i=0;i<Words;i++)
		second[i]=io.Q38PanelRead(PioneerQwen35::DataflowQueueStateAddress+i*4);
	if (memcmp(first,second,sizeof(first))!=0) return false;
	for (unsigned queue=0;queue<PioneerQwen35::DataflowCircularBufferCount;queue++) {
		const unsigned base=queue*(PioneerQwen35::DataflowQueueControlBytes/4);
		if (second[base]!=second[base+1] || second[base+6] || second[base+7])
			return false;
		if (!allowFailures && (second[base+2] || second[base+3]
			|| second[base+4] || second[base+5])) return false;
	}
	return true;
}

bool
PioneerSessionIO::DataflowPanelMappingStable()
{
	return qwen35DataflowContext && fleetEnabled && runtimeCore<120
		&& unsigned(TT_Q38_PANEL_VIEW_BASE)==DataflowPciL1ViewBase
		&& unsigned(TT_Q38_PANEL_VIEW_BYTES)>=DataflowPciL1ViewBytes
		&& Q38PanelMappingStable();
}

bool
PioneerSessionIO::DataflowCoreServiceImageMatches(unsigned core)
{
	if (core>=PioneerQwen35::DataflowCoreCount
		|| !SelectRuntimeCore(core) || !MapQ38Panel()
		|| !DataflowPanelMappingStable()
		|| RuntimeCodeRead(0)!=PioneerQwen35::DataflowServiceImageBootWord)
		return false;
	for (unsigned i=0;i<sizeof(PioneerQwen35::DataflowServiceTextWords)
			/sizeof(PioneerQwen35::DataflowServiceTextWords[0]);i++) {
		const unsigned address=PioneerQwen35::DataflowServiceEntryAddress+i*4;
		if (Q38PanelRead(address)!=PioneerQwen35::DataflowServiceTextWords[i])
			return false;
	}
	qwen35DataflowContext->qwen35DataflowImageVerified[core]=true;
	return true;
}

static bool
PioneerSessionIODataflowServiceImageTrusted(PioneerSessionIO& io,unsigned core)
{
	auto* context=io.qwen35DataflowContext;
	return context && core<PioneerQwen35::DataflowCoreCount
		&& io.SelectRuntimeCore(core) && io.MapQ38Panel()
		&& io.DataflowPanelMappingStable()
		&& io.RuntimeCodeRead(0)==PioneerQwen35::DataflowServiceImageBootWord
		&& (context->qwen35DataflowImageVerified[core]
			|| io.DataflowCoreServiceImageMatches(core));
}

static bool
PioneerQwen35DataflowCoreRuntimeStable(PioneerSessionIO& io,unsigned core)
{
	auto* context=io.qwen35DataflowContext;
	if (!context || core>=PioneerQwen35::DataflowCoreCount
		|| !io.SelectRuntimeCore(core) || !io.MapQ38Panel()
		|| !io.DataflowPanelMappingStable()
		|| !PioneerSessionIODataflowServiceImageTrusted(io,core)) return false;
	const unsigned reset=io.ResetRead();
	if (reset!=0x46800 || io.ResetRead()!=reset) return false;
	auto& dram=context->baseline.baseline.baseline.baseline.dram;
	if (!context->q38Dispatch || context->q38Dispatch->unsafe
		|| context->q38Dispatch->closed || context->q38Dispatch->rootRunning)
		return false;
	auto& node=context->q38Dispatch->worker[core];
	const uint32_t expectedNode=PioneerWorkerX(core)|(PioneerWorkerY(core)<<6);
	const uint32_t nodeFirst=io.ResidentNocRead(0x14);
	io.Pause();
	const uint32_t nodeSecond=io.ResidentNocRead(0x14);
	return nodeFirst==expectedNode && nodeSecond==expectedNode
		&& node.noc.state==TT_COMPUTE_NOC_PASSED && node.noc.tlb_restored
		&& !node.noc.recovery_required
		&& node.noc.version==(core?PioneerWorkerVersion(core):1)
		&& PioneerWorkerRoute(dram,node.noc.source[0],core)
		&& PioneerRuntimeSources(io,dram,node.noc,node.Store())
		&& PioneerQwen35DataflowServicePc(io) && BlackholeResidentIdle(io);
}

bool
PioneerSessionIO::DataflowCoreIdleForHold(unsigned core)
{
	auto* context=qwen35DataflowContext;
	if (!context || core>=PioneerQwen35::DataflowCoreCount
		|| !SelectRuntimeCore(core) || !MapQ38Panel()
		|| !DataflowPanelMappingStable()) return false;
	const unsigned reset=ResetRead();
	if (reset!=ResetRead() || (reset!=0x46800 && reset!=0x47800)) return false;
	auto& dram=context->baseline.baseline.baseline.baseline.dram;
	if (!context->q38Dispatch || !context->q38DispatchRoutes
		|| context->q38Dispatch->prepared!=PioneerQwen35::DataflowCoreCount
		|| context->q38Dispatch->unsafe || context->q38Dispatch->closed)
		return false;
	auto& lease=*context->q38Dispatch;
	auto& node=lease.worker[core];
	if (node.noc.state!=TT_COMPUTE_NOC_PASSED || !node.noc.tlb_restored
		|| node.noc.recovery_required
		|| node.noc.version!=(core?PioneerWorkerVersion(core):1)
		|| !PioneerWorkerRoute(dram,node.noc.source[0],core)
		|| !node.Store().ready || node.Store().pending
		|| node.Store().recovery_required) return false;
	if (PioneerSessionIODataflowServiceImageTrusted(*this,core)) {
		if (reset!=0x46800 && reset!=0x47800) return false;
		const uint32_t expectedNode=PioneerWorkerX(core)|(PioneerWorkerY(core)<<6);
		const uint32_t nodeFirst=ResidentNocRead(0x14);
		Pause();
		const uint32_t nodeSecond=ResidentNocRead(0x14);
		PioneerQwen35::DataflowWorkerMailbox mailbox{};
		if (nodeFirst!=expectedNode || nodeSecond!=expectedNode
			|| !PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)
			|| !(DataflowPciMailboxReadyForStop(mailbox)
				|| DataflowPciMailboxStopped(mailbox))
			|| !PioneerQwen35DataflowQueuesDrained(*this,core)
			|| !PioneerQwen35DataflowServicePc(*this)
			|| !PioneerRuntimeSources(*this,dram,node.noc,node.Store())
			|| !BlackholeResidentIdle(*this)) return false;
		return true;
	}
	const unsigned expectedReset=core?0x46800:0x47800;
	if (reset!=expectedReset
		|| !PioneerQ38DispatchSources(*this,dram,context->banks,node,expectedReset)
		|| !PioneerQ38DispatchCodeMatches(*this,core,&lease)) return false;
	if (!core) return !lease.rootRunning && !node.image.running;
	return lease.booted && !lease.rootRunning && node.image.running
		&& lease.peerNonce[core-1]
		&& PioneerQ38DispatchPeerStatus(*this,lease.peerNonce[core-1],false,&lease);
}

bool
PioneerSessionIO::DataflowCoreIdleForWrite(unsigned core,unsigned address,
	unsigned bytes)
{
	if (core>=PioneerQwen35::DataflowCoreCount
		|| ResetRead()!=0x46800 || ResetRead()!=0x46800
		|| !DataflowPciRunningWriteRange(address,bytes)
		|| !PioneerSessionIODataflowServiceImageTrusted(*this,core)) return false;
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	if (!PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)
		|| !PioneerQwen35DataflowServicePc(*this)
		|| !PioneerQwen35DataflowQueuesBalanced(*this,core,
			address==PioneerQwen35::DataflowWorkerMailboxAddress
				+offsetof(PioneerQwen35::DataflowWorkerMailbox,acknowledgedSequence)))
		return false;
	const unsigned acknowledgement=PioneerQwen35::DataflowWorkerMailboxAddress
		+offsetof(PioneerQwen35::DataflowWorkerMailbox,acknowledgedSequence);
	if (address==acknowledgement && bytes==sizeof(uint32_t)) {
		return mailbox.commandSequence==mailbox.completedSequence
			&& mailbox.commandSequence>mailbox.acknowledgedSequence
			&& !mailbox.shutdownSequence
			&& ((mailbox.state==PioneerQwen35::DataflowWorkerComplete
				&& !mailbox.failure && mailbox.phase==2)
				|| (mailbox.state==PioneerQwen35::DataflowWorkerFailed
					&& mailbox.failure && mailbox.phase==5))
			&& PioneerQwen35DataflowCoreRuntimeStable(*this,core);
	}
	return DataflowCoreIdleForHold(core)
		&& DataflowPciMailboxReadyForStop(mailbox)
		&& mailbox.shutdownSequence==0
		&& PioneerQwen35DataflowQueuesDrained(*this,core)
		&& BlackholeResidentIdle(*this);
}

bool
PioneerSessionIO::SessionIdle()
{
	auto* context=qwen35DataflowContext;
	return context && context->bankStorageOnly
		&& context->bytes==PioneerResidentBanks::Total
		&& context->runtimeCoreMode==-1 && !context->Unsafe()
		&& context->banks.Matches(context->baseline.baseline.baseline.baseline.dram)
		&& context->baseline.audit.version==1
		&& context->baseline.audit.state==TT_HOST_DMA_AUDIT_PASSED
		&& context->baseline.audit.tlb_restored==1
		&& !context->baseline.audit.recovery_required
		&& context->q38Dispatch && context->q38DispatchRoutes
		&& context->q38Dispatch->booted && !context->q38Dispatch->closed
		&& !PioneerQ38DispatchUnsafe(*context->q38Dispatch)
		&& context->q38Dispatch->prepared==PioneerQwen35::DataflowCoreCount
		&& (!context->qwen35DataflowService
			|| !context->qwen35DataflowService->Started());
}

bool
PioneerSessionIO::ProveFleetRoutes()
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->q38Dispatch || !context->q38DispatchRoutes
		|| !context->banks.Matches(context->baseline.baseline.baseline.baseline.dram)
		|| !fleetEnabled || !MapRuntime()) return false;
	auto& lease=*context->q38Dispatch;
	auto& dram=context->baseline.baseline.baseline.baseline.dram;
	if (!lease.booted || lease.closed || lease.unsafe || lease.rootRunning
		|| lease.prepared!=PioneerQwen35::DataflowCoreCount) return false;
	unsigned auditedRoutes[2][PioneerQwen35::Banks]={};
	for (unsigned bank=0;bank<PioneerQwen35::Banks;bank++) {
		uint32_t address=0,route=0,index=0;
		if (!context->banks.Page(dram,bank,0,address,route,index)
			|| route!=(dram.translated[bank][0]|(dram.translated[bank][1]<<6)))
			return false;
		auditedRoutes[0][bank]=route;
		auditedRoutes[1][bank]=route;
	}
	if (!PioneerQwen35::DataflowBankRoutesValid(auditedRoutes)) return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		const unsigned reset=core?0x46800:0x47800;
		auto& node=lease.worker[core];
		if (!SelectRuntimeCore(core) || !MapQ38Panel()
			|| !DataflowPanelMappingStable()
			|| node.noc.state!=TT_COMPUTE_NOC_PASSED || !node.noc.tlb_restored
			|| node.noc.recovery_required
			|| node.noc.version!=(core?PioneerWorkerVersion(core):1)
			|| !PioneerWorkerRoute(dram,node.noc.source[0],core)
			|| !PioneerQ38DispatchSources(*this,dram,context->banks,node,reset)
			|| !PioneerQ38DispatchCodeMatches(*this,core,&lease)) return false;
		if (!core) {
			if (node.image.running) return false;
		} else if (!node.image.running || !lease.peerNonce[core-1]
			|| !PioneerQ38DispatchPeerStatus(*this,lease.peerNonce[core-1],false,&lease))
			return false;
	}
	if (lease.rootRunning || lease.unsafe || context->banks.Quarantined()) return false;
	if (context->qwen35DataflowRoutesAudited
		&& memcmp(context->qwen35DataflowRoutes,auditedRoutes,sizeof(auditedRoutes))!=0)
		return false;
	memcpy(context->qwen35DataflowRoutes,auditedRoutes,sizeof(auditedRoutes));
	context->qwen35DataflowRoutesAudited=true;
	return true;
}

bool
PioneerSessionIO::CoreIdle(unsigned core)
{
	return DataflowCoreIdleForHold(core);
}

bool
PioneerSessionIO::CaptureCoreState(unsigned core)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->CaptureCoreState(core);
}

void
PioneerSessionIO::ReleaseCoreSnapshot(unsigned core)
{
	if (qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory)
		qwen35DataflowContext->qwen35DataflowMemory->ReleaseCoreSnapshot(core);
}

bool
PioneerSessionIO::HoldCore(unsigned core)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->HoldCore(core);
}

bool
PioneerSessionIO::CoreHeld(unsigned core)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->CoreHeld(core);
}

bool
PioneerSessionIO::WriteCoreMemory(unsigned core,unsigned address,const void* input,
	unsigned bytes)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->WriteCoreMemory(core,address,input,bytes);
}

bool
PioneerSessionIO::ReadCoreMemory(unsigned core,unsigned address,void* output,
	unsigned bytes)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->ReadCoreMemory(core,address,output,bytes);
}

bool
PioneerSessionIO::StartCore(unsigned core)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->StartCore(core);
}

bool
PioneerSessionIO::ReadWorkerSnapshot(unsigned core,
	PioneerQwen35::DataflowWorkerSnapshot& snapshot)
{
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	if (core>=PioneerQwen35::DataflowCoreCount
		|| !PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)) return false;
	snapshot={mailbox.commandSequence,mailbox.acknowledgedSequence,
		mailbox.completedSequence,mailbox.state,mailbox.failure,mailbox.phase};
	return true;
}

bool
PioneerSessionIO::WaitWorkerReady(unsigned core,
	PioneerQwen35::DataflowWorkerSnapshot& snapshot)
{
	for (unsigned poll=0;poll<65536;poll++) {
		snapshot={};
		if (ReadWorkerSnapshot(core,snapshot)
			&& snapshot.state==PioneerQwen35::DataflowWorkerReady
			&& PioneerQwen35::DataflowWorkerSnapshotReady(snapshot)
			&& CoreReady(core,snapshot)) return true;
		if (snapshot.state==PioneerQwen35::DataflowWorkerFatal
			|| snapshot.state==PioneerQwen35::DataflowWorkerFailed) return false;
		Pause();
	}
	return false;
}

bool
PioneerSessionIO::CoreReady(unsigned core,
	const PioneerQwen35::DataflowWorkerSnapshot& snapshot)
{
	PioneerQwen35::DataflowWorkerSnapshot observed{};
	return core<PioneerQwen35::DataflowCoreCount
		&& DataflowCoreIdleForHold(core)
		&& ResetRead()==0x46800 && ResetRead()==0x46800
		&& ReadWorkerSnapshot(core,observed)
		&& memcmp(&snapshot,&observed,sizeof(snapshot))==0
		&& PioneerQwen35::DataflowWorkerSnapshotReady(observed);
}

bool
PioneerSessionIO::AuthorizeProjection(
	const PioneerQwen35::DataflowProjectionProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->Authorize(program);
}

bool
PioneerSessionIO::BeginProjection(
	const PioneerQwen35::DataflowProjectionProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->Begin(program);
}

bool
PioneerSessionIO::AuthorizeEmbedding(
	const PioneerQwen35::DataflowEmbeddingProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->AuthorizeEmbedding(program);
}

bool
PioneerSessionIO::BeginEmbedding(
	const PioneerQwen35::DataflowEmbeddingProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->BeginEmbedding(program);
}

bool
PioneerSessionIO::WriteInvocation(unsigned core,
	const PioneerQwen35::DataflowCoreInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowWorkerSnapshot snapshot{};
	return ReadWorkerSnapshot(core,snapshot)
		&& PioneerQwen35::DataflowWorkerSnapshotReady(snapshot)
		&& CoreReady(core,snapshot)
		&& WriteCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation));
}

bool
PioneerSessionIO::InvocationMatches(unsigned core,
	const PioneerQwen35::DataflowCoreInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowCoreInvocation observed{};
	return ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
		&observed,sizeof(observed))
		&& memcmp(&invocation,&observed,sizeof(invocation))==0;
}

bool
PioneerSessionIO::WriteEmbeddingInvocation(unsigned core,
	const PioneerQwen35::DataflowEmbeddingInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowWorkerSnapshot snapshot{};
	return ReadWorkerSnapshot(core,snapshot)
		&& PioneerQwen35::DataflowWorkerSnapshotReady(snapshot)
		&& CoreReady(core,snapshot)
		&& WriteCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation));
}

bool
PioneerSessionIO::EmbeddingInvocationMatches(unsigned core,
	const PioneerQwen35::DataflowEmbeddingInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowEmbeddingInvocation observed{};
	return ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
		&observed,sizeof(observed))
		&& memcmp(&invocation,&observed,sizeof(invocation))==0;
}

bool
PioneerSessionIO::PublishCommand(unsigned core,unsigned sequence)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !sequence
		|| sequence>PioneerQwen35::DataflowWorkerSequenceLimit) return false;
	PioneerQwen35::DataflowWorkerSnapshot snapshot{};
	if (!ReadWorkerSnapshot(core,snapshot)
		|| !PioneerQwen35::DataflowWorkerSnapshotReady(snapshot)
		|| sequence<=snapshot.completedSequence) return false;
	uint32 magic=0;
	if (!ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,&magic,
		sizeof(magic))) return false;
	if (magic==PioneerQwen35::DataflowProgramMagic) {
		PioneerQwen35::DataflowCoreInvocation invocation{};
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation))
			|| !invocation.Valid() || invocation.core!=core) return false;
	} else if (magic==PioneerQwen35::DataflowEmbeddingMagic) {
		PioneerQwen35::DataflowEmbeddingInvocation invocation{};
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation))
			|| !invocation.Valid() || invocation.core!=core) return false;
	} else return false;
	const uint32_t command=sequence;
	const unsigned address=PioneerQwen35::DataflowWorkerMailboxAddress
		+offsetof(PioneerQwen35::DataflowWorkerMailbox,commandSequence);
	return WriteCoreMemory(core,address,&command,sizeof(command));
}

bool
PioneerSessionIO::WaitForCompletion(unsigned core,unsigned sequence,
	PioneerQwen35::DataflowWorkerSnapshot& snapshot)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !sequence) return false;
	for (unsigned poll=0;poll<65536;poll++) {
		if (ReadWorkerSnapshot(core,snapshot)) {
			if (snapshot.commandSequence==sequence
				&& snapshot.completedSequence==sequence
				&& (snapshot.state==PioneerQwen35::DataflowWorkerComplete
					|| snapshot.state==PioneerQwen35::DataflowWorkerFailed))
				return true;
			if (snapshot.state==PioneerQwen35::DataflowWorkerFatal
				|| snapshot.state==PioneerQwen35::DataflowWorkerStopped
				|| snapshot.commandSequence>sequence) return false;
		}
		Pause();
	}
	return false;
}

bool
PioneerSessionIO::AuditProjection(
	const PioneerQwen35::DataflowProjectionProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority
		|| !program.Valid() || !sequence || !context->qwen35DataflowAuthority->Pending())
		return false;
	const auto* manifest=context->qwen35DataflowAuthority->Manifest();
	if (!manifest || !PioneerQwen35::DataflowProjectionMatchesLayout(*manifest,
		context->qwen35DataflowRoutes,program)) return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (receipts[core].commandSequence!=sequence
			|| receipts[core].completedSequence!=sequence
			|| receipts[core].state!=PioneerQwen35::DataflowWorkerComplete
			|| receipts[core].failure || receipts[core].phase!=2
			|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
		PioneerQwen35::DataflowWorkerSnapshot observed{};
		PioneerQwen35::DataflowCoreInvocation invocation{};
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))) return false;
		PioneerQwen35::DataflowCoreInvocation expected{};
		if (!PioneerQwen35::DataflowProgramBuilder::BindCore(program,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::AuditEmbedding(
	const PioneerQwen35::DataflowEmbeddingProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !program.Valid()
		|| !sequence || !context->qwen35DataflowAuthority->PendingEmbeddingMatches(program))
		return false;
	const auto* manifest=context->qwen35DataflowAuthority->Manifest();
	PioneerQwen35::DataflowEmbeddingProgram expectedProgram{};
	if (!manifest || !PioneerQwen35::DataflowEmbeddingProgramBuilder::Build(
		*manifest,program.token,program.outputSlot,context->qwen35DataflowRoutes,
		expectedProgram)
		|| expectedProgram.weightBank!=program.weightBank
		|| expectedProgram.weightBase!=program.weightBase
		|| expectedProgram.lane!=program.lane
		|| !PioneerQwen35::DataflowRowSpaceMatches(expectedProgram.output,
			program.output)) return false;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<PioneerQwen35::Banks;bank++)
		if (expectedProgram.bankRoute[noc][bank]!=program.bankRoute[noc][bank])
			return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (receipts[core].commandSequence!=sequence
			|| receipts[core].completedSequence!=sequence
			|| receipts[core].state!=PioneerQwen35::DataflowWorkerComplete
			|| receipts[core].failure || receipts[core].phase!=2
			|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
		PioneerQwen35::DataflowWorkerSnapshot observed{};
		PioneerQwen35::DataflowEmbeddingInvocation invocation{};
		PioneerQwen35::DataflowEmbeddingInvocation expected{};
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))
			|| !PioneerQwen35::DataflowEmbeddingProgramBuilder::BindCore(
				expectedProgram,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::Acknowledge(unsigned core,unsigned sequence)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !sequence) return false;
	PioneerQwen35::DataflowWorkerSnapshot snapshot{};
	if (!ReadWorkerSnapshot(core,snapshot)
		|| snapshot.commandSequence!=sequence || snapshot.completedSequence!=sequence
		|| snapshot.state!=PioneerQwen35::DataflowWorkerComplete || snapshot.failure
		|| snapshot.phase!=2) return false;
	const uint32_t acknowledgement=sequence;
	const unsigned address=PioneerQwen35::DataflowWorkerMailboxAddress
		+offsetof(PioneerQwen35::DataflowWorkerMailbox,acknowledgedSequence);
	return WriteCoreMemory(core,address,&acknowledgement,sizeof(acknowledgement));
}

bool
PioneerSessionIO::CompleteProjection(
	const PioneerQwen35::DataflowProjectionProgram& program,unsigned sequence)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->Complete(program);
}

bool
PioneerSessionIO::CompleteEmbedding(
	const PioneerQwen35::DataflowEmbeddingProgram& program,unsigned sequence)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->CompleteEmbedding(program);
}

bool
PioneerSessionIO::AuditAndRecoverAll(unsigned sequence,
	PioneerQwen35::DataflowWorkerSnapshot (&snapshots)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !sequence || !context->qwen35DataflowAuthority
		|| !context->qwen35DataflowAuthority->Pending()) return false;
	auto inspect=[&](unsigned core,PioneerQwen35::DataflowWorkerSnapshot& observed,
		bool& terminal) {
		terminal=false;
		if (!PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !ReadWorkerSnapshot(core,observed)) return false;
		if (PioneerQwen35::DataflowWorkerSnapshotReady(observed)) {
			if (observed.commandSequence>sequence
				|| !PioneerQwen35DataflowQueuesDrained(*this,core)) return false;
			return true;
		}
		if (observed.commandSequence!=sequence || observed.completedSequence!=sequence
			|| observed.acknowledgedSequence>=sequence
			|| (observed.state!=PioneerQwen35::DataflowWorkerComplete
				&& observed.state!=PioneerQwen35::DataflowWorkerFailed)
			|| (observed.state==PioneerQwen35::DataflowWorkerComplete
				&& (observed.failure || observed.phase!=2))
			|| (observed.state==PioneerQwen35::DataflowWorkerFailed
				&& (!observed.failure || observed.phase!=5))
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,true)) return false;
		terminal=true;
		return true;
	};

	// A running shard may still own NoC work. Wait for a terminal receipt before
	// attempting any acknowledgement; a timeout leaves the fleet untouched.
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		bool settled=false;
		for (unsigned poll=0;poll<65536;poll++) {
			PioneerQwen35::DataflowWorkerSnapshot current{};
			if (!ReadWorkerSnapshot(core,current)) { Pause(); continue; }
			if (current.state==PioneerQwen35::DataflowWorkerFatal
				|| current.commandSequence>sequence) return false;
			if (PioneerQwen35::DataflowWorkerSnapshotReady(current)) {
				settled=true;
				break;
			}
			if (current.commandSequence==sequence
				&& current.completedSequence==sequence
				&& (current.state==PioneerQwen35::DataflowWorkerComplete
					|| current.state==PioneerQwen35::DataflowWorkerFailed)) {
				if (current.acknowledgedSequence==sequence) {
					Pause();
					continue;
				}
				settled=true;
				break;
			}
			Pause();
		}
		if (!settled) return false;
	}

	PioneerQwen35::DataflowWorkerSnapshot first[PioneerQwen35::DataflowCoreCount]{};
	PioneerQwen35::DataflowWorkerSnapshot second[PioneerQwen35::DataflowCoreCount]{};
	bool terminal[PioneerQwen35::DataflowCoreCount]{};
	for (unsigned pass=0;pass<2;pass++) {
		auto& target=pass?second:first;
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			bool isTerminal=false;
			if (!inspect(core,target[core],isTerminal)) return false;
			if (pass==0) terminal[core]=isTerminal;
			else if (terminal[core]!=isTerminal
				|| memcmp(&first[core],&second[core],sizeof(first[core]))!=0)
				return false;
		}
		if (!pass) Pause();
	}

	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (!terminal[core]) continue;
		const uint32_t acknowledgement=sequence;
		const unsigned address=PioneerQwen35::DataflowWorkerMailboxAddress
			+offsetof(PioneerQwen35::DataflowWorkerMailbox,acknowledgedSequence);
		if (!WriteCoreMemory(core,address,&acknowledgement,sizeof(acknowledgement)))
			return false;
	}
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		PioneerQwen35::DataflowWorkerSnapshot ready{};
		if (!WaitWorkerReady(core,ready)
			|| ready.completedSequence>sequence) return false;
		snapshots[core]=ready;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::AbortPendingProjection()
{
	return qwen35DataflowContext
		&& qwen35DataflowContext->qwen35DataflowAuthority
		&& qwen35DataflowContext->qwen35DataflowAuthority->AbortPendingProjection();
}

bool
PioneerSessionIO::StopAndAuditAll()
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->StopAndAuditAll();
}

bool
PioneerSessionIO::RestoreCoreState(unsigned core)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->RestoreCoreState(core);
}

bool
PioneerSessionIO::VerifyCoreStateRestored(unsigned core)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->VerifyCoreStateRestored(core);
}

bool
PioneerSessionIO::ResumeRestoredFleet()
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->ResumeRestoredFleet();
}
#endif
