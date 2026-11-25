/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_GRAPH_SESSION_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_GRAPH_SESSION_H

#include "PioneerQwen35DataflowResidentModelTransaction.h"
#include "PioneerQwen35DataflowBoundWorkspace.h"
#include "PioneerQwen35DataflowStreamPlan.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {
enum DataflowResidentSessionResult { DataflowResidentSessionInvalid=0,DataflowResidentSessionPreflight=1,
    DataflowResidentSessionRetained=2,DataflowResidentSessionComplete=3 };

// The caller owns this run and its real one-shot host transport through any
// retained fault. It must never release either while the device may use them.
template<class Host> struct DataflowResidentSessionRun {
    Host& host;
    DataflowResidentModelTransaction model;
    DataflowResidentCommand command{};
    bool attempted=false;
    explicit DataflowResidentSessionRun(Host& transport):host(transport) {}
    DataflowResidentSessionRun(const DataflowResidentSessionRun&)=delete;
    DataflowResidentSessionRun& operator=(const DataflowResidentSessionRun&)=delete;
};

// Session policy must prove the exact locked model, immutable sealed stream,
// installed service and routes. Host's independent audit must prove fleet
// completion, including sender failures. This class supplies neither proof.
class DataflowResidentGraphSession {
    template<class Handle> friend class DataflowResidentOwnedRunner;
    const DataflowStreamPlan* plan=nullptr;
    DataflowProjectionAuthority** live=nullptr;
    unsigned routes[2][Banks]{},inputSlot=ActivationBuffers,nextTrigger=1;
    DataflowGraphRunIssuer issuer;
    const void* retainedRun=nullptr;
    bool bound=false,faulted=false;
    bool CanRetireRun(const void* run) const {
        return bound && run && (faulted?retainedRun==run:retainedRun==nullptr);
    }
    void RetireRunAfterProof() { bound=false;faulted=true;retainedRun=nullptr; }
    template<class Run,class IO> DataflowResidentSessionResult Retain(Run& run,IO& io) {
        faulted=true;retainedRun=&run;io.RetainResidentSession(*this,&run);
        return DataflowResidentSessionRetained;
    }
public:
    DataflowResidentGraphSession()=default;
    DataflowResidentGraphSession(const DataflowResidentGraphSession&)=delete;
    DataflowResidentGraphSession& operator=(const DataflowResidentGraphSession&)=delete;
    bool Faulted() const { return faulted; }
    unsigned NextTrigger() const { return nextTrigger; }
    const void* RetainedRun() const { return faulted?retainedRun:nullptr; }
    template<class IO> bool Bind(const DataflowStreamPlan& sealed,const unsigned (&bankRoutes)[2][Banks],
        unsigned slot,DataflowProjectionAuthority*& authority,IO& io) {
        if (bound || faulted || !authority || slot>=ActivationBuffers || !sealed.ready
            || sealed.count!=DataflowGraphDecodeSteps || !authority->PlanningRoutesMatch(bankRoutes)
            || !io.ResidentBindAllowed(*this,sealed,slot)) return false;
        const auto* manifest=authority->Manifest();const auto* graph=authority->GraphWorkspace();
        DataflowBoundWorkspace workspace;
        if (!manifest || !graph || !workspace.Initialize(*authority)
            || !DataflowStreamPlanBuilder::Validate(workspace,*manifest,*graph,authority->PlanningRope(),bankRoutes,sealed)
            || !io.ReserveResidentSession(*this,sealed,slot)) return false;
        plan=&sealed;live=&authority;inputSlot=slot;
        for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=bankRoutes[n][b];
        bound=true;return true;
    }
    template<class Host,class IO> DataflowResidentSessionResult Run(DataflowResidentSessionRun<Host>& run,
        DataflowProjectionAuthority& candidate,unsigned position,unsigned token,bool decode,unsigned polls,
        DataflowProjectionProgram& scratch,IO& io,DataflowArgmaxResult& output,
        DataflowProjectionAuthority*& retired,unsigned& stage) {
        output={Vocabulary,0,0,0};retired=nullptr;stage=DataflowDecoderSequenceInvalidRequest;
        if (!bound || faulted || run.attempted || !plan || !live || !*live || !polls
            || polls>DataflowGraphRunMaxPolls || !nextTrigger || nextTrigger>DataflowResidentRunLimit
            || !io.ResidentSessionSafe(*this,*plan,inputSlot)) return DataflowResidentSessionInvalid;
        run.attempted=true;
        const auto* manifest=(*live)->Manifest();DataflowDynamicTokenContext dynamic{};
        DataflowGraphRunTicket ticket{};DataflowGraphRunDescriptor descriptor{};
        const unsigned count=decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps;
        if (!manifest || !DataflowDynamicTokenContext::Build(*manifest,token,position,dynamic)
            || !issuer.Issue(count,ticket) || ticket.firstSequence>DataflowResidentRunLimit
            || count-1>DataflowResidentRunLimit-ticket.firstSequence
            || !DataflowGraphRunBuild(plan->binding[0],plan->lease[0],routes,dynamic,ticket,
                nextTrigger,polls,inputSlot,descriptor)
            || !DataflowResidentCommandBuild(descriptor,run.command)
            || !run.model.Prepare(*live,candidate,run.command,run.host,scratch,stage))
            return DataflowResidentSessionPreflight;
        if (!io.ResidentSessionSafe(*this,*plan,inputSlot)) return DataflowResidentSessionPreflight;
        if (!run.host.Submit(run.command)) {
            if (run.host.Issued() || run.host.IsRetained()) return Retain(run,io);
            return DataflowResidentSessionPreflight;
        }
        bool ready=false;
        for (unsigned poll=0;poll<polls;poll++) {
            if (!io.ResidentSessionSafe(*this,*plan,inputSlot) || !run.model.Matches(run.command,run.host))
                return Retain(run,io);
            const auto state=run.host.PollCandidate();
            if (state==Host::CandidateReady) { ready=true;break; }
            if (state!=Host::CandidateWaiting) return Retain(run,io);
            io.Pause();
        }
        if (!ready) { run.host.Expire();return Retain(run,io); }
        if (!run.host.AuditCandidate() || !io.ResidentSessionSafe(*this,*plan,inputSlot)
            || !run.model.Matches(run.command,run.host) || !run.host.AcknowledgeCandidate())
            return Retain(run,io);
        if (!io.ResidentSessionSafe(*this,*plan,inputSlot) || !run.model.Commit(run.host,output))
            return Retain(run,io);
        retired=run.model.Retired();
        if (!retired || *live!=&candidate) { output={Vocabulary,0,0,0};return Retain(run,io); }
        ++nextTrigger;
        return DataflowResidentSessionComplete;
    }
};
}
#endif
#endif
