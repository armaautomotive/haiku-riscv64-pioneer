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
	return true;
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
	if (DataflowCoreServiceImageMatches(core)) {
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
		|| !DataflowCoreServiceImageMatches(core)
		|| !DataflowCoreIdleForHold(core)) return false;
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	return PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)
		&& DataflowPciMailboxReadyForStop(mailbox)
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
	return !lease.rootRunning && !lease.unsafe && !context->banks.Quarantined();
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
