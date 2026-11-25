/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PROJECTION_QUALIFICATION_SERVICE_H
#define PIONEER_QWEN35_DATAFLOW_PROJECTION_QUALIFICATION_SERVICE_H

#include "PioneerQwen35DataflowProjectionQualification.h"
#include "PioneerQwen35DataflowServiceOwner.h"

namespace PioneerQwen35 {
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
// This diagnostic's single program owns one common input and one Q4_K
// packing for every worker. Build its exact fleet membership from that
// program; do not reuse this shortcut for multi-program graph recordings.
inline bool DataflowQualificationBindSharing(const DataflowProjectionProgram& program,
    unsigned core,const DataflowCoreInvocation& original,DataflowCoreInvocation& output)
{
    using Fixture=DataflowProjectionQualification;
    if (core>=DataflowCoreCount || program.type!=12 || program.inputs!=Fixture::Inputs
        || program.outputRows!=Fixture::Rows || program.activeCores!=DataflowCoreCount
        || !DataflowProgramBuilder::BindCore(program,core,output)
        || memcmp(&original,&output,sizeof(output))!=0) return false;
    for (unsigned worker=0;worker<DataflowCoreCount;worker++) {
        if (!program.core[worker].active || program.core[worker].groups!=1) return false;
        output.sharing.members[worker/32]|=uint32_t(1)<<(worker%32);
    }
    output.sharing.leader=0;output.sharing.packing=DataflowActivationSharePacking(output.type);
    output.sharing.inputKey=DataflowActivationShareInputKey(output);
    return DataflowCoreHeaderValid(output);
}
#endif
// Heap-owned, one-shot fixed-projection adapter. The caller must hold the
// exclusive session lock and retain this object, scratch and its scope until
// service close and scratch retirement. Scope supplies real claim, routing,
// runtime and firmware checks, and its audit must inspect the actual fleet.
// A completed run means audited commands, not verified numerical output.
class DataflowProjectionQualificationService {
    enum Phase { Empty, Bound, Running, Audited, Failed };
    Phase phase=Empty;
    DataflowProjectionProgram program{};
    PioneerQ35ProjectionProgram schedule{};
    const void* scratchIdentity=nullptr;
    unsigned scratchBank=0,scratchAddress=0;
    bool receiptAudited=false;

    template<class IO,class Scope,class Scratch> struct Adapter {
        DataflowProjectionQualificationService& owner;IO& io;Scope& scope;Scratch& scratch;
        bool publicationAttempted=false;
        bool Safe() { return owner.scratchIdentity==&scratch
            && (publicationAttempted
                ?scope.QualificationProjectionInFlightSafe(scratch,owner.program)
                :scope.QualificationExecutionSafe(scratch,owner.program)); }
        bool AfterIO(bool okay) {
            // A successful per-core access can leave a nonzero route selected.
            // An unsuccessful access may have left a partial route write, so
            // never issue another selection under this transaction's authority.
            if (okay && scope.QualificationRestoreRuntimeRoute(scratch,owner.program,io)) return true;
            scope.RetainQualification(&owner,&scratch);return false;
        }
        bool AuthorizeProjection(const DataflowProjectionProgram& p) {
            return owner.Admits(p,scratch,scope);
        }
        bool BeginProjection(const DataflowProjectionProgram& p) {
            if (!owner.Admits(p,scratch,scope) || !scratch.MarkRunning(scope)) return false;
            owner.phase=Running;return true;
        }
        bool ReadWorkerSnapshot(unsigned c,DataflowWorkerSnapshot& s) {
            return Safe() && AfterIO(io.ReadWorkerSnapshot(c,s));
        }
        bool CoreReady(unsigned c,const DataflowWorkerSnapshot& s) { return Safe() && AfterIO(io.CoreReady(c,s)); }
        bool WriteInvocation(unsigned c,const DataflowCoreInvocation& v) {
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
            DataflowCoreInvocation shared{};
            return Safe() && DataflowQualificationBindSharing(owner.program,c,v,shared)
                && AfterIO(io.WriteInvocation(c,shared));
#else
            return Safe() && AfterIO(io.WriteInvocation(c,v));
#endif
        }
        bool InvocationMatches(unsigned c,const DataflowCoreInvocation& v) {
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
            DataflowCoreInvocation shared{};
            return Safe() && DataflowQualificationBindSharing(owner.program,c,v,shared)
                && AfterIO(io.InvocationMatches(c,shared));
#else
            return Safe() && AfterIO(io.InvocationMatches(c,v));
#endif
        }
        bool PublishCommand(unsigned c,unsigned sequence) {
            // A successful publication makes a worker runnable immediately.
            // Switch before attempting the write: even a failed call can
            // have published it, so an idle-fleet proof is no longer valid.
            if (!publicationAttempted && !Safe()) return false;
            publicationAttempted=true;
            return Safe() && AfterIO(io.PublishCommand(c,sequence));
        }
        bool WaitForCompletion(unsigned c,unsigned sequence,DataflowWorkerSnapshot& s) {
            return Safe() && AfterIO(io.WaitForCompletion(c,sequence,s));
        }
        bool AuditProjection(const DataflowProjectionProgram& p,unsigned sequence,
            const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) {
            if (owner.phase!=Running || &p!=&owner.program || !Safe()
                || !AfterIO(scope.AuditQualification(io,p,sequence,receipts))) return false;
            owner.receiptAudited=true;return true;
        }
        bool Acknowledge(unsigned c,unsigned sequence) { return Safe() && AfterIO(io.Acknowledge(c,sequence)); }
        bool CompleteProjection(const DataflowProjectionProgram& p,unsigned sequence) {
            if (owner.phase!=Running || &p!=&owner.program || !sequence
                || !owner.receiptAudited || !Safe()) return false;
            owner.phase=Audited;return true;
        }
    };
public:
    DataflowProjectionQualificationService()=default;
    DataflowProjectionQualificationService(const DataflowProjectionQualificationService&)=delete;
    DataflowProjectionQualificationService& operator=(const DataflowProjectionQualificationService&)=delete;
    bool IsAudited() const { return phase==Audited; }
    bool IsFailed() const { return phase==Failed; }
    bool IsRunning() const { return phase==Running; }
    bool IsUnstarted() const { return phase==Empty || phase==Bound; }
    const DataflowProjectionProgram* Program() const { return phase==Empty?nullptr:&program; }

    template<class Scratch,class Scope>
    bool Bind(Scratch& scratch,Scope& scope,unsigned bank,unsigned address,
        const unsigned (&routes)[2][Banks]) {
        if (phase!=Empty || !scratch.IsStaged()
            || !scratch.MatchesStagedLease(bank,address,DataflowProjectionQualification::ScratchBytes)
            || !scope.QualificationLeaseMatches(scratch,bank,address,
                DataflowProjectionQualification::ScratchBytes,routes)
            || !DataflowProjectionQualification::Build(bank,address,
                DataflowProjectionQualification::ScratchBytes,routes,program,schedule)) return false;
        scratchIdentity=&scratch;scratchBank=bank;scratchAddress=address;phase=Bound;return true;
    }
    template<class Scratch,class Scope>
    bool Admits(const DataflowProjectionProgram& candidate,Scratch& scratch,Scope& scope) const {
        return phase==Bound && &candidate==&program && scratchIdentity==&scratch
            && scratch.IsStaged() && candidate.Valid()
            && scratch.MatchesStagedLease(scratchBank,scratchAddress,DataflowProjectionQualification::ScratchBytes)
            && scope.QualificationLeaseMatches(scratch,scratchBank,scratchAddress,
                DataflowProjectionQualification::ScratchBytes,program.bankRoute)
            && scope.QualificationExecutionSafe(scratch,program);
    }
    template<class IO,class Scratch,class Scope>
    DataflowServiceResult Run(DataflowServiceOwner& service,IO& io,Scratch& scratch,Scope& scope) {
        if (!service.Ready() || !Admits(program,scratch,scope)) return DataflowServiceInvalidState;
        Adapter<IO,Scope,Scratch> adapter{*this,io,scope,scratch};
        const auto result=service.RunProjection(program,adapter);
        if (result!=DataflowServiceComplete || phase!=Audited) {
            phase=Failed;scope.RetainQualification(this,&scratch);return DataflowServiceProjectionFailed;
        }
        return DataflowServiceComplete;
    }
};
}
#endif
