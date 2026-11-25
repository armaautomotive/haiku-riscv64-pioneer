/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_RESIDENT_HOST_PCI_H
#define PIONEER_QWEN35_RESIDENT_HOST_PCI_H
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Resident graph host PCI path is a non-loadable shared-role witness only
#endif
#include "../../../../../tools/pioneer/PioneerQwen35DataflowResidentReceipt.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowResidentLifecycle.h"
#include <string.h>

// A single root-only host transaction. Scope must bind the exact locked
// session/model/graph owner, selected root route, running image, PCI reservation
// and candidate-specific all-worker fault audit. Its predicates must not
// reroute the TLB. A partial selection or write may already have taken effect;
// it retains the owner and is never replayed. No scope here grants hardware
// authority or makes a gathered candidate a clean completion.
template<class Raw,class Scope> class PioneerQwen35ResidentHostPci {
    using Command=PioneerQwen35::DataflowResidentCommand;
    enum Phase { Empty,Published,Candidate,Audited,Acknowledged,Retained } phase=Empty;
    Raw& io;Scope& scope;Command command{};
    bool issued=false;
    PioneerQwen35::DataflowArgmaxResult output{};
    bool haveOutput=false;
    bool Retain() { phase=Retained;scope.RetainResident();return false; }
    bool Select(unsigned operation) {
        if (!scope.ResidentSessionAllowed(io,command,operation))
            return phase==Empty?false:Retain();
        if (!io.SelectRuntimeCore(0)) return Retain();
        if (!Ready(operation)) return Retain();
        return true;
    }
    bool Ready(unsigned operation) {
        return scope.ResidentSessionAllowed(io,command,operation) && io.DataflowPanelMappingStable()
            && scope.ResidentRootAllowed(io,command,operation)
            && io.ResetRead()==0x6000 && io.ResetRead()==0x6000
            && scope.ResidentRootAllowed(io,command,operation)
            && io.DataflowPanelMappingStable() && scope.ResidentSessionAllowed(io,command,operation);
    }
    bool Store(unsigned operation,unsigned address,uint32_t value) {
        if (!Ready(operation)) return false;
        issued=true; // before the potentially delivered write
        io.Q38PanelWrite(address,value);
        return Ready(operation) && io.Q38PanelRead(address)==value && Ready(operation);
    }
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
    // Only after an exclusive lifetime proof: write a bounded group, then
    // recheck ownership and read back every word before the final commit.
    bool StoreBatch(unsigned operation,unsigned address,const uint32_t* words,unsigned count) {
        if (!words || !count || !Ready(operation)) return false;
        issued=true;
        for (unsigned i=0;i<count;i++) io.Q38PanelWrite(address+4*i,words[i]);
        if (!Ready(operation)) return false;
        for (unsigned i=0;i<count;i++) if (io.Q38PanelRead(address+4*i)!=words[i]) return false;
        return Ready(operation);
    }
    bool ReservedPayloadClear() {
        if (!Ready(Publish)) return false;
        for (unsigned i=11;i<16;i++)
            if (io.Q38PanelRead(PioneerQwen35::DataflowBarrierReleasePayload+4*i)) return false;
        return Ready(Publish);
    }
#endif
    bool Read(unsigned operation,unsigned address,uint32_t& value) {
        if (!Ready(operation)) return false;
        value=io.Q38PanelRead(address);return Ready(operation);
    }
    bool ReadWords(unsigned operation,unsigned address,uint32_t* words,unsigned count) {
        if (!words || !count) return false;
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        // Like StoreBatch, this requires exclusive ownership for the whole
        // group, not just a point-in-time check at its boundaries.
        if (!Ready(operation)) return false;
        for (unsigned i=0;i<count;i++) words[i]=io.Q38PanelRead(address+4*i);
        return Ready(operation);
#else
        for (unsigned i=0;i<count;i++) if (!Read(operation,address+4*i,words[i])) return false;
        return true;
#endif
    }
    bool ReadOutput() {
        using namespace PioneerQwen35;
        uint32_t first[DataflowWorkerMailboxBytes/4]{},second[DataflowWorkerMailboxBytes/4]{};
        if (!ReadWords(Audit,DataflowWorkerMailboxAddress,first,DataflowWorkerMailboxBytes/4)) return false;
        io.Pause();
        if (!ReadWords(Audit,DataflowWorkerMailboxAddress,second,DataflowWorkerMailboxBytes/4)) return false;
        if (memcmp(first,second,sizeof(first))) return false;
        DataflowWorkerMailbox mailbox{};memcpy(&mailbox,second,sizeof(mailbox));
        DataflowArgmaxResult result{};
        if (mailbox.magic!=DataflowWorkerMailboxMagic || mailbox.version!=DataflowWorkerMailboxVersion
            || mailbox.logicalCore || mailbox.commandSequence!=command.trigger
            || mailbox.completedSequence!=command.trigger || mailbox.acknowledgedSequence!=command.trigger
            || mailbox.state!=DataflowWorkerReady || mailbox.failure || mailbox.phase || mailbox.shutdownSequence
            || !DataflowArgmaxReadResult(mailbox,command.firstSequence+command.count-1,result)) return false;
        output=result;haveOutput=true;return true;
    }
    bool CandidateStillMatches() {
        using namespace PioneerQwen35;
        uint32_t words[8]{};
        if (!ReadWords(Audit,DataflowResidentCandidateAddress,words,8)) return false;
        DataflowResidentCandidate candidate{};memcpy(&candidate,words,sizeof(candidate));
        return DataflowResidentCandidateMatches(candidate,command);
    }
public:
    enum Operation { Publish=1,ReadCandidate=2,Audit=3,Acknowledge=4 };
    enum CandidateResult { CandidateInvalid=0,CandidateWaiting=1,CandidateReady=2,CandidateRetained=3 };
    PioneerQwen35ResidentHostPci(Raw& raw,Scope& authority):io(raw),scope(authority) {}
    PioneerQwen35ResidentHostPci(const PioneerQwen35ResidentHostPci&)=delete;
    PioneerQwen35ResidentHostPci& operator=(const PioneerQwen35ResidentHostPci&)=delete;
    bool IsRetained() const { return phase==Retained; }
    bool Issued() const { return issued; }
    bool IsAcknowledged() const { return phase==Acknowledged; }
    bool Output(PioneerQwen35::DataflowArgmaxResult& result) const {
        if (phase!=Acknowledged || !haveOutput) return false;
        result=output;return true;
    }
    bool Expire() { return phase==Published?Retain():false; }
    const Command& Request() const { return command; }
    bool Submit(const Command& request) {
        using namespace PioneerQwen35;
        if (phase!=Empty || !DataflowResidentCommandValid(request)
            || request.trigger>DataflowResidentRunLimit || request.firstSequence>DataflowResidentRunLimit
            || request.count-1>DataflowResidentRunLimit-request.firstSequence) return false;
        command=request;
        if (!Select(Publish)) return false;
        const auto payload=DataflowResidentCommandPayload(command),commit=DataflowResidentCommandCommit(command);
        // Invalidate the marker before payload changes. Publish word zero last,
        // so the worker cannot accept a partly written sixteen-word commit.
        if (!Store(Publish,DataflowBarrierReleaseCommit,0)) return Retain();
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        if (!ReservedPayloadClear()
            || !StoreBatch(Publish,DataflowBarrierReleasePayload,payload.word,11)
            || !StoreBatch(Publish,DataflowBarrierReleaseCommit+4,commit.word+1,3)) return Retain();
#else
        for (unsigned i=0;i<16;i++)
            if (!Store(Publish,DataflowBarrierReleasePayload+4*i,payload.word[i])) return Retain();
        for (unsigned i=1;i<16;i++)
            if (!Store(Publish,DataflowBarrierReleaseCommit+4*i,commit.word[i])) return Retain();
#endif
        if (!Store(Publish,DataflowBarrierReleaseCommit,commit.word[0])) return Retain();
        phase=Published;return true;
    }
    CandidateResult PollCandidate() {
        using namespace PioneerQwen35;
        if (phase!=Published) return CandidateInvalid;
        if (!Select(ReadCandidate)) return phase==Retained?CandidateRetained:CandidateInvalid;
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        // The worker publishes the trigger last. While it is absent or from
        // an earlier token, one read is sufficient; validate the full record
        // twice only after this exact trigger appears.
        uint32_t marker=0;
        if (!Read(ReadCandidate,DataflowResidentCandidateAddress+7*4,marker)) {
            Retain();return CandidateRetained;
        }
        if (marker<command.trigger) return CandidateWaiting;
        if (marker>command.trigger) { Retain();return CandidateRetained; }
#endif
        uint32_t a[8]{},b[8]{};
        if (!ReadWords(ReadCandidate,DataflowResidentCandidateAddress,a,8)) {
            Retain();return CandidateRetained;
        }
        io.Pause();
        if (!ReadWords(ReadCandidate,DataflowResidentCandidateAddress,b,8)) {
            Retain();return CandidateRetained;
        }
        // An in-progress publication or a prior token is not a receipt.
        if (memcmp(a,b,sizeof(a)) || b[7]!=command.trigger) return CandidateWaiting;
        DataflowResidentCandidate result{};memcpy(&result,b,sizeof(result));
        if (!DataflowResidentCandidateMatches(result,command)) { Retain();return CandidateRetained; }
        phase=Candidate;return CandidateReady;
    }
    bool AuditCandidate() {
        if (phase!=Candidate || !Select(Audit)) return false;
        if (command.count==PioneerQwen35::DataflowGraphDecodeSteps && !ReadOutput()) return Retain();
        const auto firstOutput=output;
        // The scope must resolve the exact fleet, either through a complete
        // scan or a separately qualified tree proof, and retain any fault.
        if (!scope.AuditResidentFleet(io,command) || !Ready(Audit) || !CandidateStillMatches()) return Retain();
        if (haveOutput && (!ReadOutput() || output.token!=firstOutput.token || output.scoreBits!=firstOutput.scoreBits
            || output.sequence!=firstOutput.sequence || output.checksum!=firstOutput.checksum)) return Retain();
        phase=Audited;return true;
    }
    bool AcknowledgeCandidate() {
        using namespace PioneerQwen35;
        if (phase!=Audited || !Select(Acknowledge)) return false;
        const uint32_t words[]={command.trigger,~command.trigger,command.epoch,~command.epoch};
        // Clear word zero, write the other words, and commit the exact trigger
        // last. An old or partially written acknowledgement cannot match.
        if (!Store(Acknowledge,DataflowResidentCandidateAckAddress,0)) return Retain();
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        if (!StoreBatch(Acknowledge,DataflowResidentCandidateAckAddress+4,words+1,3)) return Retain();
#else
        for (unsigned i=1;i<4;i++)
            if (!Store(Acknowledge,DataflowResidentCandidateAckAddress+4*i,words[i])) return Retain();
#endif
        if (!Store(Acknowledge,DataflowResidentCandidateAckAddress,words[0])) return Retain();
        phase=Acknowledged;return true;
    }
};
#endif
