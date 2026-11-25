/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_OWNED_RUNNER_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_OWNED_RUNNER_H

#include "PioneerQwen35DataflowResidentGraphSession.h"
#include "PioneerQwen35DataflowServiceOwner.h"

#if !defined(PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS)
#error Resident owned runner requires the non-loadable resident service witness
#endif
namespace PioneerQwen35 {
// Handle owns a real host transport, its scope and a session run as `run`.
// IO allocates/deletes those objects, proves the locked service/graph policy
// and keeps any retained context and mappings alive. No recovery proof or
// hardware authority is supplied here.
template<class Handle> class DataflowResidentOwnedRunner {
    Handle* retained=nullptr;
    bool retiring=false;
    DataflowProjectionAuthority *retainedCandidate=nullptr,*retainedRetired=nullptr;
    template<class IO> DataflowResidentSessionResult Retain(DataflowResidentGraphSession& session,
        DataflowServiceOwner& service,Handle& handle,DataflowProjectionAuthority& candidate,
        DataflowProjectionAuthority* retired,IO& io) {
        if (!service.OwnsRetainedResidentRun(&session,handle.run))
            service.RetainResidentRun(&session,handle.run);
        retained=&handle;retainedCandidate=&candidate;retainedRetired=retired;
        // False requires the real owner to quarantine the entire context;
        // never discard the objects just because service identity was lost.
        io.RetainResidentOwnedRun(session,handle,service.OwnsRetainedResidentRun(&session,handle.run));
        return DataflowResidentSessionRetained;
    }
public:
    DataflowResidentOwnedRunner()=default;
    DataflowResidentOwnedRunner(const DataflowResidentOwnedRunner&)=delete;
    DataflowResidentOwnedRunner& operator=(const DataflowResidentOwnedRunner&)=delete;
    bool Retained() const { return retained!=nullptr; }
    const Handle* RetainedHandle() const { return retained; }
    const DataflowProjectionAuthority* RetainedCandidate() const { return retainedCandidate; }
    const DataflowProjectionAuthority* RetainedRetired() const { return retainedRetired; }
    // IO must prove complete restoration and exact ownership; observations of
    // individual stopped workers do not qualify. The owner lock spans this
    // proof and both state transitions. A false proof never releases objects.
    template<class IO> bool ReleaseRetainedAfterRestoration(DataflowResidentGraphSession& session,
        DataflowServiceOwner& service,DataflowProjectionAuthority*& live,IO& io) {
        if (retiring || !retained || !retainedCandidate || !live || !session.CanRetireRun(&retained->run)
            || !service.CanRetireResidentRun(&session,&retained->run)) return false;
        Handle* const handle=retained;
        auto* const candidate=retainedCandidate;auto* const retired=retainedRetired;auto* const originalLive=live;
        retiring=true;
        const bool proved=io.ResidentFleetRestored(session,service,*handle,live,candidate,retired);
        // The proof callback is not trusted to preserve in-memory identities.
        if (!proved || retained!=handle || retainedCandidate!=candidate || retainedRetired!=retired
            || live!=originalLive || !session.CanRetireRun(&handle->run)
            || !service.CanRetireResidentRun(&session,&handle->run)) { retiring=false;return false; }
        service.RetireResidentRunAfterProof();session.RetireRunAfterProof();
        io.DeleteResidentRun(handle);
        if (retired && retired!=live && retired!=candidate) io.DeleteResidentCandidate(retired);
        if (candidate!=live) io.DeleteResidentCandidate(candidate);
        retained=nullptr;retainedCandidate=nullptr;retainedRetired=nullptr;retiring=false;return true;
    }
    template<class IO> DataflowResidentSessionResult Run(DataflowResidentGraphSession& session,
        DataflowServiceOwner& service,DataflowProjectionAuthority*& live,unsigned position,unsigned token,
        bool decode,unsigned polls,DataflowProjectionProgram& scratch,IO& io,
        DataflowArgmaxResult& output,unsigned& stage) {
        output={Vocabulary,0,0,0};stage=DataflowDecoderSequenceInvalidRequest;
        if (Retained() || !live || !service.ResidentSessionIdle(&session)) return DataflowResidentSessionInvalid;
        Handle* handle=io.NewResidentRun();
        if (!handle) return DataflowResidentSessionPreflight;
        auto* candidate=io.NewResidentCandidate();
        if (!candidate) { io.DeleteResidentRun(handle);return DataflowResidentSessionPreflight; }
        if (candidate==live) { io.DeleteResidentRun(handle);return DataflowResidentSessionInvalid; }
        if (!service.BeginResidentRun(&session,handle->run,session.NextTrigger())) {
            io.DeleteResidentCandidate(candidate);io.DeleteResidentRun(handle);
            return DataflowResidentSessionPreflight;
        }
        DataflowArgmaxResult completed{};DataflowProjectionAuthority* retired=nullptr;
        const auto result=session.Run(handle->run,*candidate,position,token,decode,polls,scratch,
            io,completed,retired,stage);
        if (result==DataflowResidentSessionComplete) {
            if (!io.ResidentCommitAllowed(session,handle->run)
                || !service.CommitResidentRun(&session,handle->run))
                return Retain(session,service,*handle,*candidate,retired,io);
            output=completed;io.DeleteResidentRun(handle);io.DeleteResidentCandidate(retired);
            return DataflowResidentSessionComplete;
        }
        if (result==DataflowResidentSessionRetained || !service.CancelResidentRun(&session,handle->run))
            return Retain(session,service,*handle,*candidate,retired,io);
        io.DeleteResidentCandidate(candidate);io.DeleteResidentRun(handle);return result;
    }
};
}
#endif
