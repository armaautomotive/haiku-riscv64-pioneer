/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_QWEN35_DATAFLOW_REWRITE

#include "../../../../../tools/pioneer/PioneerQwen35DataflowRuntimeRecovery.h"
#include "PioneerQwen35DataflowNoC1Audit.h"
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
#include "../../../../../tools/pioneer/PioneerQwen35DataflowGraphFaultTeardown.h"
#endif

enum PioneerQwen35RuntimeAuditDetail : unsigned {
	PioneerDataflowRuntimeDetailContext=1,
	PioneerDataflowRuntimeDetailMapping=2,
	PioneerDataflowRuntimeDetailServiceImage=3,
	PioneerDataflowRuntimeDetailReset=4,
	PioneerDataflowRuntimeDetailDispatch=5,
	PioneerDataflowRuntimeDetailLocalRoute=6,
	PioneerDataflowRuntimeDetailNoC=7,
	PioneerDataflowRuntimeDetailRoute=8,
	PioneerDataflowRuntimeDetailSources=9,
	PioneerDataflowRuntimeDetailServicePC=10,
	PioneerDataflowRuntimeDetailNoCIdle=11
};

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
	const unsigned primary=sizeof(PioneerQwen35::DataflowServiceTextWords)/sizeof(uint32_t);
	unsigned dispatch=0;
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	dispatch=sizeof(PioneerQwen35::DataflowServiceDispatchWords)/sizeof(uint32_t);
#endif
	io.Pause();
	const uint32_t first=io.RuntimePcRead();
	io.Pause();
	const uint32_t second=io.RuntimePcRead();
	return PioneerQwen35::DataflowServicePcInImage(first,primary,dispatch)
		&& PioneerQwen35::DataflowServicePcInImage(second,primary,dispatch);
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
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
	for (unsigned i=0;i<sizeof(PioneerQwen35::DataflowServiceDispatchWords)
			/sizeof(PioneerQwen35::DataflowServiceDispatchWords[0]);i++) {
		const unsigned address=PioneerQwen35::DataflowServiceDispatchAddress+i*4;
		if (Q38PanelRead(address)!=PioneerQwen35::DataflowServiceDispatchWords[i])
			return false;
	}
#endif
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
PioneerQwen35DataflowCaptureLocalRoute(PioneerSessionIO& io,unsigned core)
{
	auto* context=io.qwen35DataflowContext;
	if (!context || core>=PioneerQwen35::DataflowCoreCount
		|| !io.SelectRuntimeCore(core)) return false;
	const uint32_t firstRaw=io.ResidentNocRead(PioneerQwen35::DataflowNocLogicalIdOffset);
	io.Pause();
	const uint32_t secondRaw=io.ResidentNocRead(PioneerQwen35::DataflowNocLogicalIdOffset);
	uint32_t first=0,second=0;
	if (!PioneerQwen35::DataflowNocDecodeLocalRoute(firstRaw,first)
		|| !PioneerQwen35::DataflowNocDecodeLocalRoute(secondRaw,second)
		|| first!=second || !PioneerQwen35::DataflowNocRouteMatchesWorker(first,core)) return false;
	if (context->qwen35DataflowLocalRouteValid[core])
		return context->qwen35DataflowLocalRoute[core]==first;
	for (unsigned prior=0;prior<PioneerQwen35::DataflowCoreCount;prior++)
		if (context->qwen35DataflowLocalRouteValid[prior]
			&& context->qwen35DataflowLocalRoute[prior]==first) return false;
	context->qwen35DataflowLocalRoute[core]=first;
	context->qwen35DataflowLocalRouteValid[core]=true;
	return true;
}

static bool
PioneerQwen35DataflowLocalRouteMatches(PioneerSessionIO& io,unsigned core)
{
	auto* context=io.qwen35DataflowContext;
	return context && core<PioneerQwen35::DataflowCoreCount
		&& context->qwen35DataflowLocalRouteValid[core]
		&& PioneerQwen35DataflowCaptureLocalRoute(io,core);
}

static bool
PioneerQwen35DataflowCoreRuntimeStable(PioneerSessionIO& io,unsigned core,
	unsigned* failureDetail=NULL)
{
	auto fail=[&](unsigned detail) {
		if (failureDetail) *failureDetail=detail;
		return false;
	};
	if (failureDetail) *failureDetail=0;
	auto* context=io.qwen35DataflowContext;
	if (!context || core>=PioneerQwen35::DataflowCoreCount)
		return fail(PioneerDataflowRuntimeDetailContext);
	if (!io.SelectRuntimeCore(core) || !io.MapQ38Panel()
		|| !io.DataflowPanelMappingStable())
		return fail(PioneerDataflowRuntimeDetailMapping);
	if (!PioneerSessionIODataflowServiceImageTrusted(io,core))
		return fail(PioneerDataflowRuntimeDetailServiceImage);
	const unsigned reset=io.ResetRead();
	if (reset!=0x46800 || io.ResetRead()!=reset)
		return fail(PioneerDataflowRuntimeDetailReset);
	auto& dram=context->baseline.baseline.baseline.baseline.dram;
	if (!context->q38Dispatch || context->q38Dispatch->unsafe
		|| context->q38Dispatch->closed || context->q38Dispatch->rootRunning)
		return fail(PioneerDataflowRuntimeDetailDispatch);
	auto& node=context->q38Dispatch->worker[core];
	if (!PioneerQwen35DataflowLocalRouteMatches(io,core))
		return fail(PioneerDataflowRuntimeDetailLocalRoute);
	if (node.noc.state!=TT_COMPUTE_NOC_PASSED || !node.noc.tlb_restored
		|| node.noc.recovery_required
		|| node.noc.version!=(core?PioneerWorkerVersion(core):1))
		return fail(PioneerDataflowRuntimeDetailNoC);
	if (!PioneerWorkerRoute(dram,node.noc.source[0],core))
		return fail(PioneerDataflowRuntimeDetailRoute);
	if (!PioneerRuntimeSources(io,dram,node.noc,node.Store()))
		return fail(PioneerDataflowRuntimeDetailSources);
	if (!PioneerQwen35DataflowServicePc(io))
		return fail(PioneerDataflowRuntimeDetailServicePC);
	if (!BlackholeResidentIdle(io))
		return fail(PioneerDataflowRuntimeDetailNoCIdle);
	return true;
}

enum PioneerQwen35DataflowIdleDetail : unsigned {
	PioneerDataflowIdleDetailContext=0x101,
	PioneerDataflowIdleDetailMapping=0x102,
	PioneerDataflowIdleDetailReset=0x103,
	PioneerDataflowIdleDetailDispatch=0x104,
	PioneerDataflowIdleDetailNoC=0x105,
	PioneerDataflowIdleDetailRoute=0x106,
	PioneerDataflowIdleDetailStore=0x107,
	PioneerDataflowIdleDetailLocalRoute=0x108,
	PioneerDataflowIdleDetailServiceMailbox=0x110,
	PioneerDataflowIdleDetailServiceMailboxState=0x111,
	PioneerDataflowIdleDetailServiceQueues=0x112,
	PioneerDataflowIdleDetailServicePC=0x113,
	PioneerDataflowIdleDetailRuntimeSources=0x114,
	PioneerDataflowIdleDetailTensixIdle=0x115,
	PioneerDataflowIdleDetailDispatchSources=0x120,
	PioneerDataflowIdleDetailDispatchCode=0x121,
	PioneerDataflowIdleDetailRootState=0x122,
	PioneerDataflowIdleDetailPeerState=0x123,
	PioneerDataflowIdleDetailPeerReceipt=0x124
};

bool
PioneerSessionIO::DataflowCoreIdleForHold(unsigned core)
{
	unsigned ignored=0;
	return DataflowCoreIdleForHold(core,ignored);
}

bool
PioneerSessionIO::DataflowCoreIdleForHold(unsigned core,unsigned& diagnosticDetail)
{
	diagnosticDetail=0;
	auto reject=[&](unsigned detail) {
		diagnosticDetail=detail;
		return false;
	};
	auto* context=qwen35DataflowContext;
	if (!context || core>=PioneerQwen35::DataflowCoreCount)
		return reject(PioneerDataflowIdleDetailContext);
	if (!SelectRuntimeCore(core) || !MapQ38Panel()
		|| !DataflowPanelMappingStable())
		return reject(PioneerDataflowIdleDetailMapping);
	const unsigned reset=ResetRead();
	if (reset!=ResetRead() || (reset!=0x46800 && reset!=0x47800))
		return reject(PioneerDataflowIdleDetailReset);
	auto& dram=context->baseline.baseline.baseline.baseline.dram;
	if (!context->q38Dispatch || !context->q38DispatchRoutes
		|| context->q38Dispatch->prepared!=PioneerQwen35::DataflowCoreCount
		|| context->q38Dispatch->unsafe || context->q38Dispatch->closed)
		return reject(PioneerDataflowIdleDetailDispatch);
	auto& lease=*context->q38Dispatch;
	auto& node=lease.worker[core];
	if (node.noc.state!=TT_COMPUTE_NOC_PASSED || !node.noc.tlb_restored
		|| node.noc.recovery_required
		|| node.noc.version!=(core?PioneerWorkerVersion(core):1))
		return reject(PioneerDataflowIdleDetailNoC);
	if (!PioneerWorkerRoute(dram,node.noc.source[0],core))
		return reject(PioneerDataflowIdleDetailRoute);
	if (!node.Store().ready || node.Store().pending
		|| node.Store().recovery_required)
		return reject(PioneerDataflowIdleDetailStore);
	if (!PioneerQwen35DataflowCaptureLocalRoute(*this,core))
		return reject(PioneerDataflowIdleDetailLocalRoute);
	if (PioneerSessionIODataflowServiceImageTrusted(*this,core)) {
		PioneerQwen35::DataflowWorkerMailbox mailbox{};
		if (!PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox))
			return reject(PioneerDataflowIdleDetailServiceMailbox);
		if (mailbox.logicalCore!=core
			|| !(DataflowPciMailboxReadyForStop(mailbox,core)
				|| DataflowPciMailboxStopped(mailbox,core)))
			return reject(PioneerDataflowIdleDetailServiceMailboxState);
		if (!PioneerQwen35DataflowQueuesDrained(*this,core))
			return reject(PioneerDataflowIdleDetailServiceQueues);
		if (!PioneerQwen35DataflowServicePc(*this))
			return reject(PioneerDataflowIdleDetailServicePC);
		if (!PioneerRuntimeSources(*this,dram,node.noc,node.Store()))
			return reject(PioneerDataflowIdleDetailRuntimeSources);
		if (!BlackholeResidentIdle(*this))
			return reject(PioneerDataflowIdleDetailTensixIdle);
		return true;
	}
	const unsigned expectedReset=core?0x46800:0x47800;
	if (reset!=expectedReset
		|| !PioneerQ38DispatchSources(*this,dram,context->banks,node,expectedReset))
		return reject(PioneerDataflowIdleDetailDispatchSources);
	if (!PioneerQ38DispatchCodeMatches(*this,core,&lease))
		return reject(PioneerDataflowIdleDetailDispatchCode);
	if (!core) {
		if (lease.rootRunning || node.image.running)
			return reject(PioneerDataflowIdleDetailRootState);
		return true;
	}
	if (!lease.booted || lease.rootRunning || !node.image.running)
		return reject(PioneerDataflowIdleDetailPeerState);
	if (!PioneerQ38DispatchPeerStatus(*this,lease.peerNonce[core-1],false,&lease))
		return reject(PioneerDataflowIdleDetailPeerReceipt);
	return true;
}

bool
PioneerSessionIO::DataflowCoreStartupFatalIdle(unsigned core)
{
	auto* context=qwen35DataflowContext;
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	if (!context || core>=PioneerQwen35::DataflowCoreCount
		|| !PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)
		|| !DataflowPciMailboxStartupFatal(mailbox,core)
		|| !PioneerQwen35DataflowQueuesDrained(*this,core)) return false;
	if (mailbox.phase==1)
		return PioneerQwen35DataflowCoreRuntimeStable(*this,core);

	// Phase 2 means the image rejected its raw local NoC route as invalid.
	// Preserve that fatal loop only when the selected physical core, pinned
	// service image, captured core route, NoC sources, and idle traffic agree.
	if (mailbox.phase!=2 || !context->q38Dispatch
		|| !SelectRuntimeCore(core) || !MapQ38Panel()
		|| !DataflowPanelMappingStable()
		|| !PioneerSessionIODataflowServiceImageTrusted(*this,core)) return false;
	const unsigned reset=ResetRead();
	if (reset!=0x46800 || ResetRead()!=reset) return false;
	if (!PioneerQwen35DataflowLocalRouteMatches(*this,core)) return false;
	auto& lease=*context->q38Dispatch;
	if (lease.unsafe || lease.closed || lease.rootRunning
		|| lease.prepared!=PioneerQwen35::DataflowCoreCount) return false;
	const uint32_t routeFirst=ResidentNocRead(PioneerQwen35::DataflowNocLogicalIdOffset);
	Pause();
	const uint32_t routeSecond=ResidentNocRead(PioneerQwen35::DataflowNocLogicalIdOffset);
	uint32_t route=0,routeAgain=0;
	if (routeFirst!=mailbox.reserved[0]
		|| !PioneerQwen35::DataflowNocDecodeLocalRoute(routeFirst,route)
		|| !PioneerQwen35::DataflowNocDecodeLocalRoute(routeSecond,routeAgain)
		|| route!=routeAgain || route!=context->qwen35DataflowLocalRoute[core]) return false;
	auto& node=lease.worker[core];
	auto& dram=context->baseline.baseline.baseline.baseline.dram;
	return node.noc.state==TT_COMPUTE_NOC_PASSED && node.noc.tlb_restored
		&& !node.noc.recovery_required
		&& node.noc.version==(core?PioneerWorkerVersion(core):1)
		&& PioneerWorkerRoute(dram,node.noc.source[0],core)
		&& PioneerRuntimeSources(*this,dram,node.noc,node.Store())
		&& PioneerQwen35DataflowServicePc(*this) && BlackholeResidentIdle(*this);
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
		&& DataflowPciMailboxReadyForStop(mailbox,core)
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
PioneerSessionIO::ProveFleetRoutes(uint32_t& failedStage,uint32_t& failedCore)
{
	failedStage=1;
	failedCore=0xff;
	auto* context=qwen35DataflowContext;
	if (!context || !context->q38Dispatch || !context->q38DispatchRoutes
		|| !context->banks.Matches(context->baseline.baseline.baseline.baseline.dram)
		|| !fleetEnabled || !MapRuntime()) return false;
	auto& lease=*context->q38Dispatch;
	auto& dram=context->baseline.baseline.baseline.baseline.dram;
	failedStage=2;
	if (!lease.booted || lease.closed || lease.unsafe || lease.rootRunning
		|| lease.prepared!=PioneerQwen35::DataflowCoreCount) return false;
	unsigned auditedRoutes[2][PioneerQwen35::Banks]={};
	for (unsigned bank=0;bank<PioneerQwen35::Banks;bank++) {
		failedStage=3;
		failedCore=bank;
		uint32_t address=0,route=0,index=0;
		if (!context->banks.Page(dram,bank,0,address,route,index)
			|| route!=(dram.translated[bank][0]|(dram.translated[bank][1]<<6)))
			return false;
		auditedRoutes[0][bank]=route;
		auditedRoutes[1][bank]=route;
	}
	failedStage=4;
	failedCore=0xff;
	if (!PioneerQwen35::DataflowBankRoutesValid(auditedRoutes)) return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		const unsigned reset=core?0x46800:0x47800;
		auto& node=lease.worker[core];
		failedCore=core;
		failedStage=5;
		if (!SelectRuntimeCore(core)) return false;
		failedStage=6;
		if (!MapQ38Panel()) return false;
		failedStage=7;
		if (!DataflowPanelMappingStable()) return false;
		failedStage=8;
		if (node.noc.state!=TT_COMPUTE_NOC_PASSED || !node.noc.tlb_restored
			|| node.noc.recovery_required
			|| node.noc.version!=(core?PioneerWorkerVersion(core):1)) return false;
		failedStage=9;
		if (!PioneerWorkerRoute(dram,node.noc.source[0],core)) return false;
		failedStage=10;
		if (!PioneerQ38DispatchSources(*this,dram,context->banks,node,reset))
			return false;
		failedStage=17;
		struct NoC1Reader {
			PioneerSessionIO& io;
			uint32_t Read(unsigned offset) { return io.ResidentNocRead(0x10000+offset); }
			void Pause() { io.Pause(); }
		} noC1{*this};
		if (!PioneerQwen35DataflowNoC1Audit(noC1,dram,core)) return false;
		failedStage=11;
		if (!PioneerQ38DispatchCodeMatches(*this,core,&lease)) return false;
		if (!core) {
			failedStage=12;
			if (node.image.running) return false;
		} else {
			failedStage=13;
			if (!node.image.running) return false;
			// Bootstrap peers are valid at nonce zero. Their exact initial
			// idle receipt is proved by PeerStatus just like later nonces.
			failedStage=14;
			if (!PioneerQ38DispatchPeerStatus(*this,lease.peerNonce[core-1],false,
				&lease)) return false;
		}
	}
	failedStage=15;
	failedCore=0xff;
	if (lease.rootRunning || lease.unsafe || context->banks.Quarantined()) return false;
	failedStage=16;
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
		mailbox.completedSequence,mailbox.state,mailbox.failure,mailbox.phase,0,0};
	if (mailbox.state==PioneerQwen35::DataflowWorkerFailed
		&& mailbox.failure==PioneerQwen35::DataflowFailureEmbeddingWeightRead
		&& mailbox.phase==5) {
		snapshot.failureDetail=PioneerQwen35::DataflowEmbeddingFailureDetail(
			mailbox.reserved[1]);
		snapshot.failureStatus=mailbox.reserved[2];
	}
	return true;
}

bool
PioneerSessionIO::ReadWorkerFailureExpected(unsigned core,unsigned& expected)
{
	expected=0;
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	if (core>=PioneerQwen35::DataflowCoreCount
		|| !PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)
		|| mailbox.state!=PioneerQwen35::DataflowWorkerFailed
		|| mailbox.failure!=PioneerQwen35::DataflowFailureEmbeddingWeightRead
		|| mailbox.phase!=5
		|| ((mailbox.reserved[1]>>8)&0xffU)
			!=PioneerQwen35::DataflowNocTransferCounterTimeout
		|| (mailbox.reserved[1]&0xffU)<0x80U
		|| (mailbox.reserved[1]&0xffU)>=0x88U) return false;
	expected=mailbox.reserved[3];
	return true;
}

bool
PioneerSessionIO::ReadWorkerFailureNoC(unsigned core,unsigned& state,
	unsigned& outstandingIdMask)
{
	state=outstandingIdMask=0;
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	if (core>=PioneerQwen35::DataflowCoreCount
		|| !PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)
		|| mailbox.state!=PioneerQwen35::DataflowWorkerFailed
		|| mailbox.failure!=PioneerQwen35::DataflowFailureEmbeddingWeightRead
		|| mailbox.phase!=5
		|| ((mailbox.reserved[1]>>8)&0xffU)
			!=PioneerQwen35::DataflowNocTransferCounterTimeout
		|| (mailbox.reserved[1]&0xffU)<0x80U
		|| (mailbox.reserved[1]&0xffU)>=0x88U) return false;
	state=PioneerQwen35::DataflowEmbeddingFailureNoCState(
		mailbox.reserved[4]);
	outstandingIdMask=mailbox.reserved[5];
	return true;
}

bool
PioneerSessionIO::ReadWorkerFailureRequest(unsigned core,unsigned& address,
	unsigned& route,unsigned& bytes)
{
	address=route=bytes=0;
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	if (core>=PioneerQwen35::DataflowCoreCount
		|| !PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)
		|| mailbox.state!=PioneerQwen35::DataflowWorkerFailed
		|| mailbox.failure!=PioneerQwen35::DataflowFailureEmbeddingWeightRead
		|| mailbox.phase!=5
		|| ((mailbox.reserved[1]>>8)&0xffU)
			!=PioneerQwen35::DataflowNocTransferCounterTimeout
		|| (mailbox.reserved[1]&0xffU)<0x80U
		|| (mailbox.reserved[1]&0xffU)>=0x88U) return false;
	address=mailbox.reserved[0];
	route=PioneerQwen35::DataflowEmbeddingFailureRoute(mailbox.reserved[1]);
	bytes=PioneerQwen35::DataflowEmbeddingFailureBytes(mailbox.reserved[4]);
	return address>=PioneerQwen35::BankBase
		&& address<PioneerQwen35::BankLimit
		&& route<=0xfffU && bytes
		&& bytes<=PioneerQwen35::DataflowNocMaxTransfer;
}

bool
PioneerSessionIO::WaitWorkerReady(unsigned core,
	PioneerQwen35::DataflowWorkerSnapshot& snapshot)
{
	for (unsigned poll=0;poll<65536;poll++) {
		snapshot={};
		if (ReadWorkerSnapshot(core,snapshot)
			&& snapshot.state==PioneerQwen35::DataflowWorkerReady) return true;
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
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	return core<PioneerQwen35::DataflowCoreCount
		&& DataflowCoreIdleForHold(core)
		&& ResetRead()==0x46800 && ResetRead()==0x46800
		&& ReadWorkerSnapshot(core,observed)
		&& memcmp(&snapshot,&observed,sizeof(snapshot))==0
		&& PioneerQwen35::DataflowWorkerSnapshotReady(observed)
		&& PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox)
		&& DataflowPciMailboxMatchesReadySnapshot(mailbox,core,snapshot);
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
PioneerSessionIO::AuthorizeRms(
	const PioneerQwen35::DataflowRmsProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->AuthorizeRms(program);
}

bool
PioneerSessionIO::BeginRms(
	const PioneerQwen35::DataflowRmsProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->BeginRms(program);
}

bool
PioneerSessionIO::AuthorizeSilu(
	const PioneerQwen35::DataflowSiluProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->AuthorizeSilu(program);
}

bool
PioneerSessionIO::BeginSilu(
	const PioneerQwen35::DataflowSiluProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->BeginSilu(program);
}

bool
PioneerSessionIO::AuthorizeResidual(
	const PioneerQwen35::DataflowResidualProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->AuthorizeResidual(program);
}

bool
PioneerSessionIO::BeginResidual(
	const PioneerQwen35::DataflowResidualProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->BeginResidual(program);
}

bool
PioneerSessionIO::AuthorizeConv(
	const PioneerQwen35::DataflowConvProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->AuthorizeConv(program);
}

bool
PioneerSessionIO::BeginConv(
	const PioneerQwen35::DataflowConvProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->BeginConv(program);
}

bool
PioneerSessionIO::AuthorizeDelta(
	const PioneerQwen35::DataflowDeltaProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->AuthorizeDelta(program);
}

bool
PioneerSessionIO::BeginDelta(
	const PioneerQwen35::DataflowDeltaProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->BeginDelta(program);
}

bool
PioneerSessionIO::AuthorizeDenseAttention(
	const PioneerQwen35::DataflowDenseAttentionProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->AuthorizeDenseAttention(program);
}

bool
PioneerSessionIO::BeginDenseAttention(
	const PioneerQwen35::DataflowDenseAttentionProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->BeginDenseAttention(program);
}

bool
PioneerSessionIO::AuthorizeArgmax(
	const PioneerQwen35::DataflowArgmaxProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->AuthorizeArgmax(program);
}

bool
PioneerSessionIO::BeginArgmax(
	const PioneerQwen35::DataflowArgmaxProgram& program)
{
	auto* context=qwen35DataflowContext;
	return context && context->qwen35DataflowService
		&& context->qwen35DataflowService->Ready()
		&& context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->BeginArgmax(program);
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
PioneerSessionIO::WriteRmsInvocation(unsigned core,
	const PioneerQwen35::DataflowRmsInvocation& invocation)
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
PioneerSessionIO::RmsInvocationMatches(unsigned core,
	const PioneerQwen35::DataflowRmsInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowRmsInvocation observed;
	return ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
		&observed,sizeof(observed))
		&& memcmp(&invocation,&observed,sizeof(invocation))==0;
}

bool
PioneerSessionIO::WriteSiluInvocation(unsigned core,
	const PioneerQwen35::DataflowSiluInvocation& invocation)
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
PioneerSessionIO::SiluInvocationMatches(unsigned core,
	const PioneerQwen35::DataflowSiluInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowSiluInvocation observed{};
	return ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
		&observed,sizeof(observed))
		&& memcmp(&invocation,&observed,sizeof(invocation))==0;
}

bool
PioneerSessionIO::WriteResidualInvocation(unsigned core,
	const PioneerQwen35::DataflowResidualInvocation& invocation)
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
PioneerSessionIO::ResidualInvocationMatches(unsigned core,
	const PioneerQwen35::DataflowResidualInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowResidualInvocation observed{};
	return ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
		&observed,sizeof(observed))
		&& memcmp(&invocation,&observed,sizeof(invocation))==0;
}

bool
PioneerSessionIO::WriteConvInvocation(unsigned core,
	const PioneerQwen35::DataflowConvInvocation& invocation)
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
PioneerSessionIO::ConvInvocationMatches(unsigned core,
	const PioneerQwen35::DataflowConvInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowConvInvocation observed{};
	return ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
		&observed,sizeof(observed))
		&& memcmp(&invocation,&observed,sizeof(invocation))==0;
}

bool
PioneerSessionIO::WriteDeltaInvocation(unsigned core,
	const PioneerQwen35::DataflowDeltaInvocation& invocation)
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
PioneerSessionIO::DeltaInvocationMatches(unsigned core,
	const PioneerQwen35::DataflowDeltaInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowDeltaInvocation observed{};
	return ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
		&observed,sizeof(observed))
		&& memcmp(&invocation,&observed,sizeof(invocation))==0;
}

bool
PioneerSessionIO::WriteDenseAttentionInvocation(unsigned core,
	const PioneerQwen35::DataflowDenseAttentionInvocation& invocation)
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
PioneerSessionIO::DenseAttentionInvocationMatches(unsigned core,
	const PioneerQwen35::DataflowDenseAttentionInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowDenseAttentionInvocation observed{};
	return ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
		&observed,sizeof(observed))
		&& memcmp(&invocation,&observed,sizeof(invocation))==0;
}

bool
PioneerSessionIO::WriteArgmaxInvocation(unsigned core,
	const PioneerQwen35::DataflowArgmaxInvocation& invocation)
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
PioneerSessionIO::ArgmaxInvocationMatches(unsigned core,
	const PioneerQwen35::DataflowArgmaxInvocation& invocation)
{
	if (core>=PioneerQwen35::DataflowCoreCount || !invocation.Valid()
		|| invocation.core!=core) return false;
	PioneerQwen35::DataflowArgmaxInvocation observed{};
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
	} else if (magic==PioneerQwen35::DataflowRmsMagic) {
		PioneerQwen35::DataflowRmsInvocation invocation;
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation))
			|| !invocation.Valid() || invocation.core!=core) return false;
	} else if (magic==PioneerQwen35::DataflowSiluMagic) {
		PioneerQwen35::DataflowSiluInvocation invocation{};
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation))
			|| !invocation.Valid() || invocation.core!=core) return false;
	} else if (magic==PioneerQwen35::DataflowResidualMagic) {
		PioneerQwen35::DataflowResidualInvocation invocation{};
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation))
			|| !invocation.Valid() || invocation.core!=core) return false;
	} else if (magic==PioneerQwen35::DataflowConvMagic) {
		PioneerQwen35::DataflowConvInvocation invocation{};
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation))
			|| !invocation.Valid() || invocation.core!=core) return false;
	} else if (magic==PioneerQwen35::DataflowDeltaMagic) {
		PioneerQwen35::DataflowDeltaInvocation invocation{};
		if (!ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
			&invocation,sizeof(invocation))
			|| !invocation.Valid() || invocation.core!=core) return false;
	} else if (magic==PioneerQwen35::DataflowDenseAttentionMagic) {
		PioneerQwen35::DataflowDenseAttentionInvocation invocation{};
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
	if (!context->qwen35DataflowAuthority->PendingProjectionMatches(program))
		return false;
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
PioneerSessionIO::AuditRms(
	const PioneerQwen35::DataflowRmsProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !program.Valid()
		|| !sequence || !context->qwen35DataflowAuthority->PendingRmsMatches(program))
		return false;
	const auto* manifest=context->qwen35DataflowAuthority->Manifest();
	PioneerQwen35::DataflowRmsProgram expectedProgram;
	if (!manifest || !PioneerQwen35::DataflowRmsProgramBuilder::Build(
		*manifest,program.layer,program.role,program.inputSlot,program.outputSlot,
		context->qwen35DataflowRoutes,expectedProgram)
		|| expectedProgram.scratchRow!=program.scratchRow
		|| !PioneerQwen35::DataflowRowSpaceMatches(expectedProgram.input,program.input)
		|| !PioneerQwen35::DataflowRowSpaceMatches(expectedProgram.weight,program.weight)
		|| !PioneerQwen35::DataflowRowSpaceMatches(expectedProgram.output,program.output))
		return false;
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
		PioneerQwen35::DataflowRmsInvocation invocation;
		PioneerQwen35::DataflowRmsInvocation expected;
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))
			|| !PioneerQwen35::DataflowRmsProgramBuilder::BindCore(
				expectedProgram,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::AuditSilu(
	const PioneerQwen35::DataflowSiluProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !program.Valid()
		|| !sequence
		|| !context->qwen35DataflowAuthority->PendingSiluMatches(program))
		return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (receipts[core].commandSequence!=sequence
			|| receipts[core].completedSequence!=sequence
			|| receipts[core].state!=PioneerQwen35::DataflowWorkerComplete
			|| receipts[core].failure || receipts[core].phase!=2
			|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
		PioneerQwen35::DataflowWorkerSnapshot observed{};
		PioneerQwen35::DataflowSiluInvocation invocation{},expected{};
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))
			|| !PioneerQwen35::DataflowSiluProgramBuilder::BindCore(
				program,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::AuditResidual(
	const PioneerQwen35::DataflowResidualProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !program.Valid()
		|| !sequence
		|| !context->qwen35DataflowAuthority->PendingResidualMatches(program))
		return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (receipts[core].commandSequence!=sequence
			|| receipts[core].completedSequence!=sequence
			|| receipts[core].state!=PioneerQwen35::DataflowWorkerComplete
			|| receipts[core].failure || receipts[core].phase!=2
			|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
		PioneerQwen35::DataflowWorkerSnapshot observed{};
		PioneerQwen35::DataflowResidualInvocation invocation{},expected{};
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))
			|| !PioneerQwen35::DataflowResidualProgramBuilder::BindCore(
				program,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::AuditConv(
	const PioneerQwen35::DataflowConvProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !program.Valid()
		|| !sequence
		|| !context->qwen35DataflowAuthority->PendingConvMatches(program))
		return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (receipts[core].commandSequence!=sequence
			|| receipts[core].completedSequence!=sequence
			|| receipts[core].state!=PioneerQwen35::DataflowWorkerComplete
			|| receipts[core].failure || receipts[core].phase!=2
			|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
		PioneerQwen35::DataflowWorkerSnapshot observed{};
		PioneerQwen35::DataflowConvInvocation invocation{},expected{};
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))
			|| !PioneerQwen35::DataflowConvProgramBuilder::BindCore(
				program,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::AuditDelta(
	const PioneerQwen35::DataflowDeltaProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !program.Valid()
		|| !sequence
		|| !context->qwen35DataflowAuthority->PendingDeltaMatches(program))
		return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (receipts[core].commandSequence!=sequence
			|| receipts[core].completedSequence!=sequence
			|| receipts[core].state!=PioneerQwen35::DataflowWorkerComplete
			|| receipts[core].failure || receipts[core].phase!=2
			|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
		PioneerQwen35::DataflowWorkerSnapshot observed{};
		PioneerQwen35::DataflowDeltaInvocation invocation{},expected{};
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))
			|| !PioneerQwen35::DataflowDeltaProgramBuilder::BindCore(
				program,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::AuditDenseAttention(
	const PioneerQwen35::DataflowDenseAttentionProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount])
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !program.Valid()
		|| !sequence
		|| !context->qwen35DataflowAuthority->PendingDenseAttentionMatches(program))
		return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (receipts[core].commandSequence!=sequence
			|| receipts[core].completedSequence!=sequence
			|| receipts[core].state!=PioneerQwen35::DataflowWorkerComplete
			|| receipts[core].failure || receipts[core].phase!=2
			|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
		PioneerQwen35::DataflowWorkerSnapshot observed{};
		PioneerQwen35::DataflowDenseAttentionInvocation invocation{},expected{};
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))
			|| !PioneerQwen35::DataflowDenseAttentionProgramBuilder::BindCore(
				program,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
	}
	return SelectRuntimeCore(0);
}

bool
PioneerSessionIO::ReadArgmaxResult(unsigned sequence,
	PioneerQwen35::DataflowArgmaxResult& result)
{
	if (!sequence) return false;
	PioneerQwen35::DataflowWorkerMailbox mailbox{};
	return PioneerQwen35DataflowReadStableMailbox(*this,0,mailbox)
		&& PioneerQwen35::DataflowArgmaxReadResult(mailbox,sequence,result);
}

bool
PioneerSessionIO::AuditArgmax(
	const PioneerQwen35::DataflowArgmaxProgram& program,unsigned sequence,
	const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount],
	const PioneerQwen35::DataflowArgmaxResult& result)
{
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !program.Valid()
		|| !sequence || !PioneerQwen35::DataflowArgmaxResultValid(result)
		|| result.sequence!=sequence
		|| !context->qwen35DataflowAuthority->PendingArgmaxMatches(program))
		return false;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (receipts[core].commandSequence!=sequence
			|| receipts[core].completedSequence!=sequence
			|| receipts[core].state!=PioneerQwen35::DataflowWorkerComplete
			|| receipts[core].failure || receipts[core].phase!=2
			|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
			|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
		PioneerQwen35::DataflowWorkerSnapshot observed{};
		PioneerQwen35::DataflowArgmaxInvocation invocation{},expected{};
		if (!ReadWorkerSnapshot(core,observed)
			|| memcmp(&receipts[core],&observed,sizeof(observed))!=0
			|| !ReadCoreMemory(core,PioneerQwen35::DataflowProgramAddress,
				&invocation,sizeof(invocation))
			|| !PioneerQwen35::DataflowArgmaxProgramBuilder::BindCore(
				program,core,expected)
			|| memcmp(&expected,&invocation,sizeof(expected))!=0) return false;
		PioneerQwen35::DataflowWorkerMailbox mailbox{};
		if (!PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox))
			return false;
		if (core==0) {
			PioneerQwen35::DataflowArgmaxResult observedResult{};
			if (!PioneerQwen35::DataflowArgmaxReadResult(mailbox,sequence,
				observedResult)
				|| memcmp(&result,&observedResult,sizeof(result))!=0) return false;
		} else {
			for (unsigned i=0;i<6;i++) if (mailbox.reserved[i]) return false;
			if (mailbox.logicalCore!=core) return false;
		}
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
PioneerSessionIO::CompleteRms(
	const PioneerQwen35::DataflowRmsProgram& program,unsigned sequence)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->CompleteRms(program);
}

bool
PioneerSessionIO::CompleteSilu(
	const PioneerQwen35::DataflowSiluProgram& program,unsigned sequence)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->CompleteSilu(program);
}

bool
PioneerSessionIO::CompleteResidual(
	const PioneerQwen35::DataflowResidualProgram& program,unsigned sequence)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->CompleteResidual(program);
}

bool
PioneerSessionIO::CompleteConv(
	const PioneerQwen35::DataflowConvProgram& program,unsigned sequence)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->CompleteConv(program);
}

bool
PioneerSessionIO::CompleteDelta(
	const PioneerQwen35::DataflowDeltaProgram& program,unsigned sequence)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->CompleteDelta(program);
}

bool
PioneerSessionIO::CompleteDenseAttention(
	const PioneerQwen35::DataflowDenseAttentionProgram& program,unsigned sequence)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->CompleteDenseAttention(program);
}

bool
PioneerSessionIO::CompleteArgmax(
	const PioneerQwen35::DataflowArgmaxProgram& program,unsigned sequence,
	unsigned token)
{
	auto* context=qwen35DataflowContext;
	return sequence && context && context->qwen35DataflowAuthority
		&& context->qwen35DataflowAuthority->CompleteArgmax(program,token);
}

bool
PioneerSessionIO::AuditAndRecoverAll(unsigned sequence,
	PioneerQwen35::DataflowWorkerSnapshot (&snapshots)[PioneerQwen35::DataflowCoreCount],
	unsigned& failureStage,unsigned& failureCore,unsigned& failureDetail)
{
	failureStage=PioneerQwen35::DataflowRecoveryAuditNone;
	failureCore=PioneerQwen35::DataflowCoreCount;
	failureDetail=0;
	auto fail=[&](unsigned stage,unsigned core,unsigned detail=0) {
		failureStage=stage;
		failureCore=core;
		failureDetail=detail;
		return false;
	};
	auto* context=qwen35DataflowContext;
	if (!context || !sequence || !context->qwen35DataflowAuthority)
		return fail(PioneerQwen35::DataflowRecoveryAuditInvalidContext,
			PioneerQwen35::DataflowCoreCount);
	if (!context->qwen35DataflowAuthority->Pending())
		return fail(PioneerQwen35::DataflowRecoveryAuditNoPendingSequence,
			PioneerQwen35::DataflowCoreCount);
	auto inspect=[&](unsigned core,PioneerQwen35::DataflowWorkerSnapshot& observed,
		bool& terminal) {
		terminal=false;
		if (!ReadWorkerSnapshot(core,observed))
			return fail(PioneerQwen35::DataflowRecoveryAuditSnapshotUnreadable,core);
		if (PioneerQwen35::DataflowWorkerSnapshotReady(observed)) {
			if (observed.commandSequence>sequence)
				return fail(PioneerQwen35::DataflowRecoveryAuditSequenceAhead,core);
			unsigned runtimeDetail=0;
			if (!PioneerQwen35DataflowCoreRuntimeStable(*this,core,&runtimeDetail))
				return fail(PioneerQwen35::DataflowRecoveryAuditRuntimeUnstable,core,
					runtimeDetail);
			if (!PioneerQwen35DataflowQueuesDrained(*this,core))
				return fail(PioneerQwen35::DataflowRecoveryAuditReadyQueuesNotDrained,
					core);
			return true;
		}
		if (observed.commandSequence!=sequence || observed.completedSequence!=sequence
			|| observed.acknowledgedSequence>=sequence
			|| (observed.state!=PioneerQwen35::DataflowWorkerComplete
				&& observed.state!=PioneerQwen35::DataflowWorkerFailed)
			|| (observed.state==PioneerQwen35::DataflowWorkerComplete
				&& (observed.failure || observed.phase!=2))
			|| (observed.state==PioneerQwen35::DataflowWorkerFailed
				&& (!observed.failure || observed.phase!=5)))
			return fail(PioneerQwen35::DataflowRecoveryAuditInvalidReceipt,core);
		if (!PioneerQwen35DataflowQueuesBalanced(*this,core,true))
			return fail(PioneerQwen35::DataflowRecoveryAuditQueuesUnbalanced,core);
		// A failed embedding weight read is read-only and can leave its NoC
		// completion pending after the worker has published its failure receipt.
		// Wait only for those read status registers to drain; never clear or reset
		// hardware state. Other failure classes retain the immediate strict audit.
		if (observed.state==PioneerQwen35::DataflowWorkerFailed
			&& observed.failure==PioneerQwen35::DataflowFailureEmbeddingWeightRead) {
			unsigned idleDetail=0;
			if (!PioneerQwen35::DataflowRuntimeWaitForNoCIdle(*this,core,
				PioneerQwen35::DataflowRuntimeRecoveryPollLimit,idleDetail)) {
				if (idleDetail>=PioneerQwen35::DataflowRuntimeRecoveryNoCIdleDetailBase
					&& idleDetail<PioneerQwen35::DataflowRuntimeRecoveryNoCIdleDetailBase
						+PioneerQwen35::DataflowRuntimeRecoveryNoCStatusCount
					&& observed.failureDetail)
					idleDetail=PioneerQwen35::DataflowRuntimeRecoveryPackEmbeddingFailure(
						observed.failureDetail);
				return fail(PioneerQwen35::DataflowRecoveryAuditRuntimeUnstable,core,
					idleDetail);
			}
		}
		unsigned runtimeDetail=0;
		if (!PioneerQwen35DataflowCoreRuntimeStable(*this,core,&runtimeDetail))
			return fail(PioneerQwen35::DataflowRecoveryAuditRuntimeUnstable,core,
				runtimeDetail);
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
			if (current.state==PioneerQwen35::DataflowWorkerFatal)
				return fail(PioneerQwen35::DataflowRecoveryAuditFatalWorker,core);
			if (current.commandSequence>sequence)
				return fail(PioneerQwen35::DataflowRecoveryAuditSequenceAhead,core);
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
		if (!settled)
			return fail(PioneerQwen35::DataflowRecoveryAuditWorkerWaitTimeout,core);
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
				return fail(PioneerQwen35::DataflowRecoveryAuditReceiptChanged,core);
		}
		if (!pass) Pause();
	}

	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		if (!terminal[core]) continue;
		const uint32_t acknowledgement=sequence;
		const unsigned address=PioneerQwen35::DataflowWorkerMailboxAddress
			+offsetof(PioneerQwen35::DataflowWorkerMailbox,acknowledgedSequence);
		if (!WriteCoreMemory(core,address,&acknowledgement,sizeof(acknowledgement)))
			return fail(PioneerQwen35::DataflowRecoveryAuditAcknowledgementWriteFailed,
				core);
	}
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		PioneerQwen35::DataflowWorkerSnapshot ready{};
		if (!WaitWorkerReady(core,ready)
			|| ready.completedSequence>sequence)
			return fail(PioneerQwen35::DataflowRecoveryAuditReadyAfterAckFailed,core);
		snapshots[core]=ready;
	}
	if (!SelectRuntimeCore(0))
		return fail(PioneerQwen35::DataflowRecoveryAuditRuntimeSelectFailed,
			PioneerQwen35::DataflowCoreCount);
	return true;
}

bool
PioneerSessionIO::AbortPendingProjection()
{
	return qwen35DataflowContext
		&& qwen35DataflowContext->qwen35DataflowAuthority
		&& qwen35DataflowContext->qwen35DataflowAuthority->AbortPendingProjection();
}

bool
PioneerSessionIO::StopAndAuditAll(unsigned& failedStage,unsigned& failedCore)
{
	return qwen35DataflowContext && qwen35DataflowContext->qwen35DataflowMemory
		&& qwen35DataflowContext->qwen35DataflowMemory->StopAndAuditAll(
			failedStage,failedCore);
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
PioneerSessionIO::ResumeRestoredFleet(unsigned& failedStage,unsigned& failedCore,
	unsigned& failedDetail)
{
	if (!qwen35DataflowContext || !qwen35DataflowContext->qwen35DataflowMemory) {
		failedStage=0;
		failedCore=PioneerQwen35::DataflowCoreCount;
		failedDetail=0;
		return false;
	}
	return qwen35DataflowContext->qwen35DataflowMemory->ResumeRestoredFleet(
		failedStage,failedCore,failedDetail);
}
#endif
#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
bool
PioneerSessionIO::AuditGraphCore(unsigned core,const PioneerQwen35::DataflowGraphRunDescriptor& expected,
	const PioneerQwen35::DataflowWorkerSnapshot& receipt,bool decode)
{
	using namespace PioneerQwen35;
	auto* context=qwen35DataflowContext;
	if (!context || !context->qwen35DataflowAuthority || !DataflowGraphRunValid(expected,core)
		|| decode!=(expected.ticket.count==DataflowGraphDecodeSteps)
		|| (!decode && expected.ticket.count!=DataflowGraphPromptSteps)
		|| receipt.commandSequence!=expected.triggerSequence || receipt.completedSequence!=expected.triggerSequence
		|| receipt.state!=DataflowWorkerComplete || receipt.failure || receipt.phase!=2
		|| !PioneerQwen35DataflowCoreRuntimeStable(*this,core)
		|| !PioneerQwen35DataflowQueuesBalanced(*this,core,false)) return false;
	auto& dram=context->baseline.baseline.baseline.baseline.dram;
	struct NoC1Reader {
		PioneerSessionIO& io;
		uint32_t Read(unsigned offset) { return io.ResidentNocRead(0x10000+offset); }
		void Pause() { io.Pause(); }
	} noC1{*this};
	if (!PioneerQwen35DataflowNoC1Audit(noC1,dram,core)) return false;
	DataflowWorkerSnapshot observed{};DataflowGraphRunDescriptor descriptor{};
	if (!ReadWorkerSnapshot(core,observed) || memcmp(&receipt,&observed,sizeof(receipt))
		|| !ReadCoreMemory(core,DataflowGraphRunAddress,&descriptor,sizeof(descriptor))
		|| memcmp(&descriptor,&expected,sizeof(descriptor))) return false;
	const auto* manifest=context->qwen35DataflowAuthority->Manifest();
	const auto* graph=context->qwen35DataflowAuthority->GraphWorkspace();
	if (!manifest || !graph) return false;
	if (decode) {
		DataflowArgmaxProgram program{};DataflowArgmaxInvocation want{},actual{};
		if (!DataflowArgmaxProgramBuilder::Build(*manifest,*graph,expected.routes,program)
			|| !DataflowArgmaxProgramBuilder::BindCore(program,core,want)
			|| !ReadCoreMemory(core,DataflowProgramAddress,&actual,sizeof(actual))
			|| memcmp(&want,&actual,sizeof(want))) return false;
	} else {
		DataflowResidualProgram program{};DataflowResidualInvocation want{},actual{};
		if (!DataflowResidualProgramBuilder::Build(*manifest,*graph,DataflowResidualAdd,
				expected.inputSlot,expected.routes,program)
			|| !DataflowResidualProgramBuilder::BindCore(program,core,want)
			|| !ReadCoreMemory(core,DataflowProgramAddress,&actual,sizeof(actual))
			|| memcmp(&want,&actual,sizeof(want))) return false;
	}
	DataflowWorkerMailbox mailbox{};
	if (!PioneerQwen35DataflowReadStableMailbox(*this,core,mailbox) || mailbox.logicalCore!=core) return false;
	if (decode && !core) {
		DataflowArgmaxResult result{};
		return DataflowArgmaxReadResult(mailbox,expected.ticket.firstSequence+expected.ticket.count-1,result);
	}
	for (unsigned i=0;i<6;i++) if (mailbox.reserved[i]) return false;
	return true;
}

bool
PioneerSessionIO::StageGraphRun(unsigned core,const PioneerQwen35::DataflowGraphRunDescriptor& descriptor)
{
	using namespace PioneerQwen35;
	DataflowWorkerSnapshot snapshot{};
	if (!DataflowGraphRunValid(descriptor,core) || !ReadWorkerSnapshot(core,snapshot)
		|| !DataflowWorkerSnapshotReady(snapshot) || !CoreReady(core,snapshot)
		|| descriptor.triggerSequence!=snapshot.completedSequence+1) return false;
	if (snapshot.completedSequence) {
		if (!qwen35DataflowContext || !qwen35DataflowContext->qwen35DataflowMemory
			|| !qwen35DataflowContext->qwen35DataflowMemory->UpdateGraphRun(
				core,snapshot.completedSequence,descriptor)) return false;
	} else if (!WriteCoreMemory(core,DataflowGraphRunAddress,&descriptor,sizeof(descriptor)))
		return false;
	// Every descriptor word has either been read back after writing or was
	// verified unchanged by UpdateGraphRun. The locked owner records the
	// exact staged descriptor before the publisher may issue any command.
	const uint32_t command=DataflowGraphRunCommand;
	return WriteCoreMemory(core,DataflowProgramAddress,&command,sizeof(command));
}

bool
PioneerSessionIO::PublishGraphCommand(unsigned core,const PioneerQwen35::DataflowGraphRunDescriptor& descriptor)
{
	using namespace PioneerQwen35;
	const unsigned sequence=descriptor.triggerSequence;
	DataflowWorkerSnapshot snapshot{};uint32_t marker=0;
	if (!ReadWorkerSnapshot(core,snapshot) || !DataflowWorkerSnapshotReady(snapshot)
		|| !CoreReady(core,snapshot) || sequence!=snapshot.completedSequence+1
		|| !DataflowGraphRunValid(descriptor,core)
		|| !ReadCoreMemory(core,DataflowProgramAddress,&marker,sizeof(marker))
		|| marker!=DataflowGraphRunCommand) return false;
	return WriteCoreMemory(core,DataflowWorkerMailboxAddress
		+offsetof(DataflowWorkerMailbox,commandSequence),&sequence,sizeof(sequence));
}

// Non-loadable backend witness. Caller must hold the global session lock and
// retain this owner for as long as the bank claim exists. This binds the plan
// to the already-validated manifest and the driver's own initialized ledger.
class PioneerQ35StreamPageIO {
	PioneerStoreContext& context;
public:
	explicit PioneerQ35StreamPageIO(PioneerStoreContext& c):context(c) {}
	bool Reserve(const PioneerQwen35::DataflowStreamPlan& plan,const void* owner) {
		using namespace PioneerQwen35;
		if (!owner || !context.bankStorageOnly || context.bytes!=PioneerResidentBanks::Total
			|| !context.initialized || !context.qwen35DataflowAuthority
			|| context.qwen35DataflowService || context.Unsafe()
			|| !context.qwen35DataflowRoutesAudited) return false;
		const auto& authority=*context.qwen35DataflowAuthority;
		const auto* manifest=authority.Manifest();const auto* graph=authority.GraphWorkspace();
		DataflowBoundWorkspace workspace;
		if (!manifest || !graph || !workspace.Initialize(authority)
			|| !authority.PlanningRoutesMatch(context.qwen35DataflowRoutes)
			|| !DataflowStreamPlanBuilder::Validate(workspace,*manifest,*graph,
				authority.PlanningRope(),context.qwen35DataflowRoutes,plan)) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) if (!context.io.CoreIdle(core)) return false;
		if (!context.io.SelectRuntimeCore(0) || !context.lease.ready
			|| context.lease.pending || context.lease.recovery_required) return false;
		PioneerQ35StreamBankRange ranges[DataflowCoreCount];
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			const auto& r=plan.lease[core];
			if (r.address<BankBase) return false;
			ranges[core]={r.bank,r.address-BankBase,r.bytes};
		}
		const auto& baseline=context.baseline.baseline.baseline.baseline.dram;
		return context.banks.ReserveQ35Stream(baseline,owner,ranges,DataflowCoreCount,
			context.initialized,PioneerResidentBanks::Pages);
	}
	bool WriteVerifiedPage(const void* owner,unsigned bank,uint64_t offset,uint32_t* words) {
		if (!owner || !words || context.qwen35DataflowService || context.Unsafe()
			|| !context.io.SelectRuntimeCore(0)) return false;
		auto& dram=context.baseline.baseline.baseline.baseline.dram;
		auto& noc=context.baseline.baseline.baseline.noc;
		tt_resident_store_result result{};
		return PioneerResidentStorePage(context.io,dram,noc,context.host,context.lease,
			PioneerResidentBanks::Capacity,offset,true,words,result,&context.banks,bank,owner);
	}
	bool Seal(const void* owner) { return context.banks.SealQ35Stream(owner); }
	bool SealedMatches(const PioneerQwen35::DataflowStreamPlan& plan,const void* owner) const {
		using namespace PioneerQwen35;
		auto& baseline=context.baseline.baseline.baseline.baseline.dram;
		if (!context.banks.Q35SealedOwner(baseline,owner,DataflowCoreCount)) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			const auto& r=plan.lease[core];
			if (r.address<BankBase) return false;
			const PioneerQ35StreamBankRange expected{r.bank,r.address-BankBase,r.bytes};
			if (!context.banks.Q35SealedRange(owner,core,expected)) return false;
		}
		return true;
	}
	void Quarantine() {
		context.banks.Quarantine();context.lease.recovery_required=true;
		context.io.QuarantineHostPage();sColdRecoveryRequired=true;
	}
};

// Compile-only composition of the real authority, recorder, protected page
// sink, and resident bank transport. This object is large and must be heap
// owned by the session, retained through all uncertain work and destroyed
// only after the full graph/session release audit. No such lifecycle or
// public entry exists yet, so it cannot authorize or launch the graph.
class PioneerQ35StreamRecordingWitness {
	PioneerStoreContext& context;
	PioneerQ35StreamPageIO io;
	PioneerQwen35::DataflowStreamPageSink<PioneerQ35StreamPageIO> sink;
	PioneerQwen35::DataflowProjectionAuthority candidate;
	PioneerQwen35::DataflowStreamPlan plan;
	PioneerQwen35::DataflowStreamRecord record;
	PioneerQwen35::DataflowProjectionProgram scratch;
	PioneerQwen35::DataflowGraphStageLedger stageLedger;
	bool attempted=false;
	bool recorded=false;
	unsigned recordedSlot=PioneerQwen35::ActivationBuffers;
	unsigned recordedRoutes[2][PioneerQwen35::Banks]{};
	const PioneerQwen35::DataflowGraphSession* graphSession=nullptr;
public:
	explicit PioneerQ35StreamRecordingWitness(PioneerStoreContext& c)
		:context(c),io(c),sink(io) {}
	bool Build(unsigned epoch,unsigned position,unsigned token,unsigned inputSlot,
		bool selectNextToken,unsigned& stage) {
		using namespace PioneerQwen35;
		if (attempted || !context.qwen35DataflowAuthority) return false;
		attempted=true;
		const auto& live=*context.qwen35DataflowAuthority;
		const auto* manifest=live.Manifest();const auto* graph=live.GraphWorkspace();
		DataflowBoundWorkspace workspace;
		if (!manifest || !graph || !workspace.Initialize(live)) return false;
		static_assert(Layers==32,"record count requires the Qwen35 32-layer sequence");
		const unsigned count=selectNextToken?548U:545U;
		if (!DataflowStreamPlanBuilder::Build(workspace,*manifest,*graph,live.PlanningRope(),
			context.qwen35DataflowRoutes,epoch,count,plan)) return false;
		recorded=DataflowStreamRecording::Build(workspace,live,candidate,plan,
			context.qwen35DataflowRoutes,position,token,inputSlot,selectNextToken,
			sink,record,scratch,stage);
		if (recorded) {
			recordedSlot=inputSlot;
			for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++)
				recordedRoutes[n][b]=context.qwen35DataflowRoutes[n][b];
		}
		return recorded;
	}
	bool SealedMatches(const PioneerQwen35::DataflowStreamPlan& requested,unsigned inputSlot) const {
		using namespace PioneerQwen35;
		// The service itself makes context.Unsafe() true while installed. This
		// proves only the sealed stream identity; the future session owner
		// must separately prove service state and all other context safety.
		if (!recorded || recordedSlot!=inputSlot
			|| !context.qwen35DataflowAuthority || !context.qwen35DataflowRoutesAudited
			|| !context.qwen35DataflowAuthority->PlanningRoutesMatch(context.qwen35DataflowRoutes)
			|| !sink.SealedMatches(requested)) return false;
		for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++)
			if (recordedRoutes[n][b]!=context.qwen35DataflowRoutes[n][b]) return false;
		return true;
	}
	const PioneerQwen35::DataflowStreamPlan* SealedPlan() const { return recorded?&plan:nullptr; }
	unsigned SealedSlot() const { return recordedSlot; }
	bool ReserveGraphSession(const PioneerQwen35::DataflowGraphSession* owner,
		const PioneerQwen35::DataflowStreamPlan& requested,unsigned slot) {
		if (!owner || graphSession || !SealedMatches(requested,slot)) return false;
		graphSession=owner;return true;
	}
	bool OwnsGraphSession(const PioneerQwen35::DataflowGraphSession* owner,
		const PioneerQwen35::DataflowStreamPlan& requested,unsigned slot) const {
		return owner && graphSession==owner && SealedMatches(requested,slot);
	}
	PioneerQwen35::DataflowGraphStageLedger& StageLedger() { return stageLedger; }
};

// Compile-only adapter for the real session transport and sealed bank claim.
// The caller must hold the global probe lock and retain all graph resources.
class PioneerQ35GraphBackend {
	PioneerStoreContext& context;
	PioneerQ35StreamRecordingWitness& stream;
	const PioneerDriverRecoveryProof* resetProof;
	uint64_t resetTicket;
public:
	PioneerQ35GraphBackend(PioneerStoreContext& c,PioneerQ35StreamRecordingWitness& s,
		const PioneerDriverRecoveryProof* proof=nullptr,uint64_t ticket=0)
		:context(c),stream(s),resetProof(proof),resetTicket(ticket) {}
	bool GraphSessionSafe(const PioneerQwen35::DataflowServiceOwner& resident) {
		return context.qwen35DataflowService==&resident && context.io.qwen35DataflowContext==&context
			&& !sColdRecoveryRequired && !sReloadQuiesced && sReloadGeneration
			&& sRuntimeOwner && sRuntimeOwner->magic==0x50313030 && sRuntimeOwner->context==&context
			&& sBankStorageOnly && context.bankStorageOnly && context.bytes==PioneerResidentBanks::Total
			&& context.initialized && context.qwen35DataflowAuthority && context.qwen35DataflowMemory
			&& context.qwen35DataflowProgramScratch && context.qwen35DataflowRoutesAudited
			&& (resident.Ready() || resident.GraphRunning()) && !context.Unsafe(true);
	}
	bool SealedStreamMatches(const PioneerQwen35::DataflowStreamPlan& p,unsigned slot) { return stream.SealedMatches(p,slot); }
	bool ReserveGraphSession(const PioneerQwen35::DataflowGraphSession* owner,
		const PioneerQwen35::DataflowStreamPlan& p,unsigned slot) { return stream.ReserveGraphSession(owner,p,slot); }
	bool OwnsGraphSession(const PioneerQwen35::DataflowGraphSession* owner,
		const PioneerQwen35::DataflowStreamPlan& p,unsigned slot) { return stream.OwnsGraphSession(owner,p,slot); }
	bool GraphFaultSessionSafe(const PioneerQwen35::DataflowGraphSession* owner,
		const PioneerQwen35::DataflowServiceOwner& resident,
		const PioneerQwen35::DataflowStreamPlan& p,unsigned slot) {
		return owner && context.qwen35GraphOwner && context.qwen35DataflowService==&resident
			&& resident.Faulted() && context.io.qwen35DataflowContext==&context
			&& !sColdRecoveryRequired && !sReloadQuiesced && sReloadGeneration
			&& sRuntimeOwner && sRuntimeOwner->magic==0x50313030 && sRuntimeOwner->context==&context
			&& sBankStorageOnly && context.bankStorageOnly && context.bytes==PioneerResidentBanks::Total
			&& context.qwen35DataflowMemory && context.qwen35DataflowAuthority
			&& stream.OwnsGraphSession(owner,p,slot) && !context.Unsafe(false,true);
	}
	// Same-device nonposted read drains earlier host writes before inspecting
	// the retained fleet. Do not reprogram the TLB before that read.
	bool FenceGraphHostWrites() {
		if (!context.io.DataflowPanelMappingStable()) return false;
		const auto magic=context.io.Q38PanelRead(PioneerQwen35::DataflowWorkerMailboxAddress);
		return context.io.DataflowPanelMappingStable()
			&& magic==PioneerQwen35::DataflowWorkerMailboxMagic;
	}
	void Pause() { context.io.Pause(); }
	bool AuditGraphRecoveryCore(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& snapshot) {
		using namespace PioneerQwen35;
		if (!PioneerQwen35DataflowCoreRuntimeStable(context.io,core)
			|| !PioneerQwen35DataflowQueuesBalanced(context.io,core,true)) return false;
		auto& dram=context.baseline.baseline.baseline.baseline.dram;
		struct NoC1Reader {
			PioneerSessionIO& io;
			uint32_t Read(unsigned offset) { return io.ResidentNocRead(0x10000+offset); }
			void Pause() { io.Pause(); }
		} noC1{context.io};
		DataflowWorkerMailbox mailbox{};
		if (!PioneerQwen35DataflowNoC1Audit(noC1,dram,core)
			|| !PioneerQwen35DataflowReadStableMailbox(context.io,core,mailbox)) return false;
		return mailbox.magic==DataflowWorkerMailboxMagic && mailbox.version==DataflowWorkerMailboxVersion
			&& mailbox.logicalCore==core && !mailbox.shutdownSequence
			&& mailbox.commandSequence==snapshot.commandSequence
			&& mailbox.acknowledgedSequence==snapshot.acknowledgedSequence
			&& mailbox.completedSequence==snapshot.completedSequence
			&& mailbox.state==snapshot.state && mailbox.failure==snapshot.failure
			&& mailbox.phase==snapshot.phase;
	}
	bool AcknowledgeFault(unsigned core,unsigned sequence) {
		using namespace PioneerQwen35;
		DataflowWorkerSnapshot s{};
		if (!sequence || !context.io.ReadWorkerSnapshot(core,s)
			|| s.commandSequence!=sequence || s.completedSequence!=sequence
			|| s.acknowledgedSequence>=sequence
			|| !((s.state==DataflowWorkerComplete && !s.failure && s.phase==2)
				|| (s.state==DataflowWorkerFailed && s.failure && s.phase==5))) return false;
		const uint32_t acknowledgement=sequence;
		return context.io.WriteCoreMemory(core,DataflowWorkerMailboxAddress
			+offsetof(DataflowWorkerMailbox,acknowledgedSequence),&acknowledgement,sizeof(acknowledgement));
	}
	bool ReadWorkerSnapshot(unsigned c,PioneerQwen35::DataflowWorkerSnapshot& s) { return context.io.ReadWorkerSnapshot(c,s); }
	bool CoreReady(unsigned c,const PioneerQwen35::DataflowWorkerSnapshot& s) { return context.io.CoreReady(c,s); }
	bool StageGraphRun(unsigned c,const PioneerQwen35::DataflowGraphRunDescriptor& d) {
		auto& ledger=stream.StageLedger();
		return ledger.CanStage(c,d) && context.io.StageGraphRun(c,d) && ledger.Record(c,d);
	}
	bool GraphRunMatches(unsigned c,const PioneerQwen35::DataflowGraphRunDescriptor& d) {
		return stream.StageLedger().Verify(c,d);
	}
	bool PublishGraphCommand(unsigned c,unsigned seq) {
		const auto* descriptor=stream.StageLedger().BeginPublish(c,seq);
		return descriptor && context.io.PublishGraphCommand(c,*descriptor);
	}
	bool WaitForCompletion(unsigned c,unsigned seq,PioneerQwen35::DataflowWorkerSnapshot& s) { return context.io.WaitForCompletion(c,seq,s); }
	bool AuditGraphCore(unsigned c,const PioneerQwen35::DataflowGraphRunDescriptor& d,
		const PioneerQwen35::DataflowWorkerSnapshot& s,bool decode) { return context.io.AuditGraphCore(c,d,s,decode); }
	bool ReadArgmaxResult(unsigned seq,PioneerQwen35::DataflowArgmaxResult& r) { return context.io.ReadArgmaxResult(seq,r); }
	bool Acknowledge(unsigned c,unsigned seq) { return context.io.Acknowledge(c,seq); }
	PioneerQwen35::DataflowGraphSessionRun* NewGraphRun() {
		return PioneerQ35NewAligned<PioneerQwen35::DataflowGraphSessionRun>();
	}
	PioneerQwen35::DataflowProjectionAuthority* NewGraphCandidate() {
		// The model bind and context destructor already use ordinary new/delete.
		static_assert(alignof(PioneerQwen35::DataflowProjectionAuthority)<=alignof(void*),"authority needs ordinary kernel allocation");
		return new(std::nothrow) PioneerQwen35::DataflowProjectionAuthority;
	}
	void DeleteGraphRun(PioneerQwen35::DataflowGraphSessionRun* p) { PioneerQ35DeleteAligned(p); }
	void DeleteGraphCandidate(PioneerQwen35::DataflowProjectionAuthority* p) { delete p; }
	bool GraphFleetRestored() {
		return resetProof && context.recoveryAttempted && !context.recoveryMapping
			&& context.io.qwen35DataflowContext==&context && sColdRecoveryRequired
			&& !sReloadQuiesced && sRuntimeOwner && sRuntimeOwner->context==&context
			&& PioneerQ35GraphRecoveryProofMatches(*resetProof,resetTicket,true,
				context.io.postResetDisposeComplete);
	}
	bool GraphFleetRestoredAfterClose(const PioneerQwen35::DataflowGraphSession& session,
		const PioneerQwen35::DataflowGraphFaultAudit& audit) {
		auto* run=session.RetainedRun();
		return run && audit.SettledFor(run->publisher) && context.qwen35DataflowService
			&& context.qwen35DataflowService->Closed() && !context.qwen35DataflowMemory
			&& context.io.qwen35DataflowContext==&context && !sColdRecoveryRequired
			&& !sReloadQuiesced && sRuntimeOwner && sRuntimeOwner->context==&context
			&& !context.Unsafe(false,true);
	}
	PioneerQwen35::DataflowServiceResult CloseFaultedGraphService() {
		return context.CloseDataflowRewrite();
	}
};

// This context-owned object is compiled only in the non-loadable graph
// build. Record before service install, bind after install, and retain the
// object until the service has been cleanly restored or a reset is proved.
class PioneerQ35GraphOwner {
	PioneerStoreContext& context;
	PioneerQ35StreamRecordingWitness stream;
	PioneerQwen35::DataflowGraphSession session;
	PioneerQwen35::DataflowGraphOwnedRunner runner;
	PioneerQwen35::DataflowGraphFaultAudit faultAudit;
	bool recorded=false,bound=false,retiredAfterFault=false;
public:
	explicit PioneerQ35GraphOwner(PioneerStoreContext& c):context(c),stream(c) {}
	bool Record(unsigned slot,unsigned& stage) {
		if (recorded || bound || context.qwen35DataflowService) return false;
		recorded=stream.Build(1,0,0,slot,true,stage);return recorded;
	}
	bool Bind() {
		const auto* plan=stream.SealedPlan();
		if (!recorded || bound || !plan || !context.qwen35DataflowService) return false;
		PioneerQ35GraphBackend backend(context,stream);
		bound=session.Bind(*plan,context.qwen35DataflowRoutes,stream.SealedSlot(),
			context.qwen35DataflowAuthority,*context.qwen35DataflowService,backend);
		return bound;
	}
	bool ReadyForSlot(unsigned slot) const { return bound && !Retained() && stream.SealedSlot()==slot; }
	PioneerQwen35::DataflowGraphSessionResult Run(unsigned position,unsigned token,bool decode,unsigned polls,
		PioneerQwen35::DataflowArgmaxResult& output,unsigned& stage) {
		output={PioneerQwen35::Vocabulary,0,0,0};stage=PioneerQwen35::DataflowDecoderSequenceInvalidRequest;
		if (!bound || !context.qwen35DataflowProgramScratch) return PioneerQwen35::DataflowGraphSessionInvalid;
		PioneerQ35GraphBackend backend(context,stream);
		return runner.Run(session,context.qwen35DataflowAuthority,position,token,decode,polls,
			*context.qwen35DataflowProgramScratch,backend,output,stage);
	}
	bool Retained() const { return runner.Retained() || (session.Faulted() && !retiredAfterFault); }
	PioneerQwen35::DataflowServiceResult RecoverFaultForClose() {
		using namespace PioneerQwen35;
		if (!bound || retiredAfterFault || !runner.Retained() || !session.Faulted())
			return DataflowServiceInvalidState;
		faultAudit.ResetUnsettled();
		PioneerQ35GraphBackend backend(context,stream);
		const auto result=DataflowGraphFaultTeardown::Run(session,runner,faultAudit,
			context.qwen35DataflowAuthority,65536,backend);
		if (result!=DataflowServiceClosed) return result;
		retiredAfterFault=true;bound=false;
		return DataflowServiceClosed;
	}
#ifdef PIONEER_QWEN35_RECOVERY_RUN_ENABLE
	bool ReleaseAfterVerifiedReset(const PioneerDriverRecoveryProof& proof,uint64_t ticket) {
		if (!runner.Retained()) return true;
		PioneerQ35GraphBackend backend(context,stream,&proof,ticket);
		return runner.ReleaseRetainedAfterRestoration(context.qwen35DataflowAuthority,backend);
	}
#endif
};

static void
PioneerQ35GraphOwnerDestroy(PioneerQ35GraphOwner* owner)
{
	PioneerQ35DeleteAligned(owner);
}

static bool
PioneerQ35GraphOwnerRetained(const PioneerQ35GraphOwner* owner)
{
	return owner && owner->Retained();
}

#ifdef PIONEER_QWEN35_RECOVERY_RUN_ENABLE
static bool
PioneerQ35GraphOwnerReleaseAfterReset(PioneerQ35GraphOwner* owner,
	const PioneerDriverRecoveryProof& proof,uint64_t ticket)
{
	return !owner || owner->ReleaseAfterVerifiedReset(proof,ticket);
}
#endif

// Ownership is installed before recording can issue any DMA. If recording
// fails after a claim, the context keeps this owner and its quarantine state.
__attribute__((used,noinline)) static bool
PioneerQ35GraphOwnerRecord(PioneerStoreContext& context,unsigned slot,unsigned& stage)
{
	if (context.qwen35GraphOwner || context.qwen35DataflowService) return false;
	context.qwen35GraphOwner=PioneerQ35NewAligned<PioneerQ35GraphOwner>(context);
	return context.qwen35GraphOwner && context.qwen35GraphOwner->Record(slot,stage);
}

__attribute__((used,noinline)) static bool
PioneerQ35GraphOwnerBind(PioneerStoreContext& context)
{
	return context.qwen35GraphOwner && context.qwen35GraphOwner->Bind();
}

__attribute__((used,noinline)) static PioneerQwen35::DataflowGraphSessionResult
PioneerQ35GraphOwnerRun(PioneerStoreContext& context,unsigned position,unsigned token,
	bool decode,unsigned polls,PioneerQwen35::DataflowArgmaxResult& output,unsigned& stage)
{
	output={PioneerQwen35::Vocabulary,0,0,0};stage=PioneerQwen35::DataflowDecoderSequenceInvalidRequest;
	return context.qwen35GraphOwner?context.qwen35GraphOwner->Run(position,token,decode,polls,output,stage)
		:PioneerQwen35::DataflowGraphSessionInvalid;
}

__attribute__((used,noinline)) static bool
PioneerQ35GraphSessionBindCompileWitness(PioneerStoreContext& context,PioneerQ35StreamRecordingWitness& stream,
	PioneerQwen35::DataflowGraphSession& session)
{
	const auto* plan=stream.SealedPlan();
	if (!plan || !context.qwen35DataflowService) return false;
	PioneerQ35GraphBackend backend(context,stream);
	return session.Bind(*plan,context.qwen35DataflowRoutes,stream.SealedSlot(),
		context.qwen35DataflowAuthority,*context.qwen35DataflowService,backend);
}

__attribute__((used,noinline)) static PioneerQwen35::DataflowGraphSessionResult
PioneerQ35GraphSessionRunCompileWitness(PioneerStoreContext& context,PioneerQ35StreamRecordingWitness& stream,
	PioneerQwen35::DataflowGraphSession& session,PioneerQwen35::DataflowGraphSessionRun& run,
	PioneerQwen35::DataflowProjectionAuthority& candidate,unsigned position,unsigned token,bool decode,unsigned polls,
	PioneerQwen35::DataflowArgmaxResult& output,PioneerQwen35::DataflowProjectionAuthority*& retired,unsigned& stage)
{
	if (!context.qwen35DataflowProgramScratch) return PioneerQwen35::DataflowGraphSessionInvalid;
	PioneerQ35GraphBackend backend(context,stream);
	return session.Run(run,candidate,position,token,decode,polls,*context.qwen35DataflowProgramScratch,
		backend,output,retired,stage);
}

__attribute__((used,noinline)) static bool
PioneerQ35StreamRecordingCompileWitness(PioneerQ35StreamRecordingWitness& graph,
	unsigned epoch,unsigned position,unsigned token,unsigned inputSlot,bool select,unsigned& stage)
{
	return graph.Build(epoch,position,token,inputSlot,select,stage);
}

__attribute__((used,noinline)) static bool
PioneerQ35SealedStreamCompileWitness(const PioneerQ35StreamRecordingWitness& graph,
	const PioneerQwen35::DataflowStreamPlan& plan,unsigned inputSlot)
{
	return graph.SealedMatches(plan,inputSlot);
}

__attribute__((used,noinline)) static bool
PioneerQ35GraphModelPrepareCompileWitness(PioneerQwen35::DataflowGraphModelTransaction& transaction,
	PioneerQwen35::DataflowProjectionAuthority*& live,PioneerQwen35::DataflowProjectionAuthority& after,
	const PioneerQwen35::DataflowGraphPublisher& publisher,
	const PioneerQwen35::DataflowGraphRunTicket& ticket,const PioneerQwen35::DataflowDynamicTokenContext& token,
	unsigned slot,PioneerQwen35::DataflowProjectionProgram& scratch,unsigned& stage)
{
	return transaction.Prepare(live,after,publisher,ticket,token,slot,scratch,stage);
}

__attribute__((used,noinline)) static bool
PioneerQ35GraphModelCommitCompileWitness(PioneerQwen35::DataflowGraphModelTransaction& transaction,
	const PioneerQwen35::DataflowGraphPublisher& publisher,const PioneerQwen35::DataflowGraphCompletion& completion)
{
	return transaction.Commit(publisher,completion);
}

__attribute__((used,noinline)) static bool
PioneerQ35GraphServiceBeginCompileWitness(PioneerStoreContext& context,
	const PioneerQwen35::DataflowGraphPublisher& publisher,unsigned& trigger)
{
	return context.qwen35DataflowService
		&& context.qwen35DataflowService->BeginGraph(publisher,trigger);
}

__attribute__((used,noinline)) static bool
PioneerQ35GraphServiceCancelCompileWitness(PioneerStoreContext& context,
	const PioneerQwen35::DataflowGraphPublisher& publisher)
{
	return context.qwen35DataflowService
		&& context.qwen35DataflowService->CancelGraphBeforePublication(publisher);
}

__attribute__((used,noinline)) static bool
PioneerQ35StreamPageBackendCompileWitness(PioneerQ35StreamPageIO& io,
	PioneerQwen35::DataflowStreamPageSink<PioneerQ35StreamPageIO>& sink,
	const PioneerQwen35::DataflowStreamPlan& plan,const PioneerQwen35::DataflowStreamRecord& record)
{
	(void)io;
	return sink.Begin(plan) && sink.StageRecord(plan.binding[0],plan.lease[0],0,record)
		&& sink.Seal(plan);
}
#endif
