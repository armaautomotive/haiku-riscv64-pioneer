/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_CONTROL_MOCK_ONLY
#include "pioneer_q38_dispatch_control_test.cpp"
#ifndef PIONEER_Q38_WORK_OUTPUT_DMA
#error "This test must exercise the actual Work DMA controller"
#endif
static void Retained(PioneerProbeCookie& cookie)
{
	auto& c=*cookie.context;
	assert(c.Unsafe() && sColdRecoveryRequired && c.wideHost.allocated);
	unsigned writes=c.io.Writes(),selects=c.io.selects,pciReads=c.io.pciReads,pciWrites=c.io.pciWrites,loads=c.io.wideLoads;
	tt_resident_session_result result={}; bool known=false;
	assert(!EndQ38DispatchOwnership(c,result,known) && c.dispatchOwnerRetained && c.bulkHost.quarantined && c.wideHost.quarantined);
	auto request=Request(); assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,request.get(),sizeof(*request))==B_DEV_NOT_READY);
	assert(writes==c.io.Writes() && selects==c.io.selects && pciReads==c.io.pciReads && pciWrites==c.io.pciWrites && loads==c.io.wideLoads);
	End(cookie,false);
	assert(!sRuntimeOwner); // disposal here is test-fabric ONLY
}
int main(int argc,char** argv)
{
	assert(argc==2); (void)&End; (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	for (unsigned step=1;step<=4;step++) for (bool readFault:{false,true}) {
		PioneerProbeCookie cookie; Begin(cookie,argv[1]); auto& c=*cookie.context; auto job=Request();
		if (readFault) c.io.failPciWriteRead=step; else c.io.failPciWrite=step;
		assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_OK);
		assert(job->result.recovery_required && c.wideHost.pending && c.io.pciWrites==step && c.io.pciWriteReads==step);
		assert(!c.io.rootStops && !c.io.rootHolds && c.io.wideLoads==4*TT_WIDE_OUTPUT_WORDS);
		for (const auto& rows:job->output) for (auto value:rows) assert(!value);
		Retained(cookie); sColdRecoveryRequired=false;
	}
	for (unsigned packet:{1U,
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
		2U,4U
#else
		30U,60U
#endif
		}) for (unsigned fault=0;fault<4;fault++) {
		if ((fault==1 && !(packet&1)) || (fault==2 && (packet&1))) continue;
		PioneerProbeCookie cookie; Begin(cookie,argv[1]); auto& c=*cookie.context; auto job=Request();
		job->peers=119; for (unsigned w=0;w<119;w++) job->part[w]={w%7,1,64+uint64_t(w/7)*0x00800000};
		if (!fault) c.io.failDma=packet; else if (fault==1) c.io.dropDma=packet;
		else if (fault==2) c.io.corruptDma=packet; else c.io.changeSourceDma=packet;
		assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_OK);
		assert(job->result.recovery_required && c.wideHost.pending && c.io.pciWrites==2 && c.io.dmaPackets>=packet);
		assert(!c.io.rootStops && !c.io.rootHolds && c.io.wideLoads==4*TT_WIDE_OUTPUT_WORDS); // CPU preparation ONLY
		Retained(cookie); sColdRecoveryRequired=false;
	}
	for (unsigned phase:{3U,4U}) {
		PioneerProbeCookie cookie; Begin(cookie,argv[1]); auto& c=*cookie.context; auto job=Request();
		// Preparation maps three times plus its owner proof;then four RUN proofs.
		c.io.lateMap=4+phase;
		assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_OK);
		assert(job->result.recovery_required && c.wideHost.pending && c.io.pciWrites==(phase==3?2U:4U));
		Retained(cookie); sColdRecoveryRequired=false;
	}
	{
		PioneerProbeCookie cookie; Begin(cookie,argv[1]); auto& c=*cookie.context; auto job=Request();
		assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_OK && !c.Unsafe());
		constexpr unsigned expectedPackets=
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
			2;
#else
			4;
#endif
		assert(c.io.pciWrites==4 && !c.wideHost.pending && !c.io.outputReads && c.io.dmaPackets==expectedPackets);
		assert(PioneerQ38WorkSafety(*job,c.q38Dispatch->authority));
		for (unsigned word=49;word<
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
			58 // marker is exact;cycle deltas are DATA,not bit-exact authority
#else
			64
#endif
			;word++) for (unsigned bit=0;bit<32;bit++) {
			job->result.stats[word]^=1U<<bit;
			assert(!PioneerQ38WorkSafety(*job,c.q38Dispatch->authority));
			job->result.stats[word]^=1U<<bit;
		}
#ifdef PIONEER_Q38_ROOT_CYCLE_PROFILE
		for (unsigned phase=0;phase<3;phase++) {
			auto& lo=job->result.stats[58+phase*2]; auto& hi=job->result.stats[59+phase*2];
			auto savedLo=lo,savedHi=hi; lo=hi=0;
			assert(!PioneerQ38WorkSafety(*job,c.q38Dispatch->authority)); lo=savedLo; hi=savedHi;
			lo^=0x80000000; assert(PioneerQ38WorkSafety(*job,c.q38Dispatch->authority)); lo=savedLo;
		}
#endif
		for (unsigned w=0;w<7;w++) for (unsigned r=0;r<256;r++) {
			if (r>=16) assert(!job->output[w][r]);
			else assert(job->output[w][r]==c.io.dmaData[w*
#ifdef PIONEER_Q38_WORK_COMPACT_OUTPUT
				16
#else
				256
#endif
				+r]);
		}
		assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK && !c.Unsafe());
		assert(c.io.pciWrites==4 && c.io.outputReads && c.io.dmaPackets==expectedPackets); // CHECKED keeps PIO;no new BME/DMA
		failCopy=copyCalls+2;
		assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==-99 && !c.Unsafe() && !c.wideHost.pending);
		failCopy=0; End(cookie,true);
	}
	puts("PASS ACTUAL kernel Work DMA: private three-owner address authority,delayed writes,zero PIO output reads,checked isolation,all BME write/read failures+first/middle/last DMA failures+late map faults retain ALL owners without disable/reset/retry;copyout failure after completed transfer allows explicit END;NO native claim");
}
