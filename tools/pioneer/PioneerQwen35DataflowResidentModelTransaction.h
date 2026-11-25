/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_MODEL_TRANSACTION_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_MODEL_TRANSACTION_H

#include "PioneerQwen35DataflowGraphMetadata.h"
#include "PioneerQwen35DataflowResidentLifecycle.h"
#include <string.h>

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {
// One resident token's CPU-side authority metadata; no tensor inference is
// performed here. Host must be the real one-shot root transport, and the
// caller must retain it, both authorities and the session on any uncertain
// publication, audit, acknowledgement or failed promotion.
class DataflowResidentModelTransaction {
    enum Phase { Empty,Prepared,Committed,Rejected } phase=Empty;
    DataflowProjectionAuthority** liveSlot=nullptr;
    DataflowProjectionAuthority *original=nullptr,*candidate=nullptr,*retired=nullptr,*promoted=nullptr;
    const void* hostOwner=nullptr;
    DataflowResidentCommand request{};
    DataflowDynamicTokenContext token{};
    DataflowGraphAuthorityState originalState{},candidateState{};
    bool BaseUnchanged() const {
        return phase==Prepared && liveSlot && *liveSlot==original && original && candidate
            && original->GraphStateMatches(originalState) && candidate->GraphStateMatches(candidateState);
    }
    bool RequestMatches(const DataflowResidentCommand& command) const {
        return DataflowResidentCommandValid(command) && !memcmp(&request,&command,sizeof(command));
    }
public:
    DataflowResidentModelTransaction()=default;
    DataflowResidentModelTransaction(const DataflowResidentModelTransaction&)=delete;
    DataflowResidentModelTransaction& operator=(const DataflowResidentModelTransaction&)=delete;
    template<class Host> bool Prepare(DataflowProjectionAuthority*& live,DataflowProjectionAuthority& after,
        const DataflowResidentCommand& command,Host& host,DataflowProjectionProgram& scratch,unsigned& stage) {
        if (phase!=Empty) return false;
        phase=Rejected;
        if (!live || &after==live || host.Issued() || host.IsRetained() || host.IsAcknowledged()
            || !DataflowResidentCommandValid(command) || command.trigger>DataflowResidentRunLimit
            || command.firstSequence>DataflowResidentRunLimit
            || command.count-1>DataflowResidentRunLimit-command.firstSequence) return false;
        const auto* manifest=live->Manifest();DataflowDynamicTokenContext canonical{};
        if (!manifest || !DataflowDynamicTokenContext::Build(*manifest,command.token,command.position,canonical)
            || !live->CaptureGraphState(originalState)
            || !DataflowGraphMetadata::Prepare(*live,after,command.position,command.token,command.inputSlot,
                command.count==DataflowGraphDecodeSteps,scratch,stage)
            || !after.CaptureGraphState(candidateState)) return false;
        liveSlot=&live;original=live;candidate=&after;hostOwner=&host;request=command;token=canonical;
        phase=Prepared;return true;
    }
    template<class Host> bool Matches(const DataflowResidentCommand& command,const Host& host) const {
        return BaseUnchanged() && hostOwner==&host && RequestMatches(command)
            && token.token==command.token && token.position==command.position;
    }
    // Only the exact host transaction can reach Acknowledged, after its
    // independent fleet audit. Output remains private until state promotion.
    template<class Host> bool Commit(const Host& host,DataflowArgmaxResult& result) {
        result={Vocabulary,0,0,0};
        if (!Matches(host.Request(),host) || !host.IsAcknowledged() || host.IsRetained()) return false;
        DataflowArgmaxResult observed{};
        if (request.count==DataflowGraphDecodeSteps && !host.Output(observed)) return false;
        retired=original;promoted=candidate;*liveSlot=candidate;candidate=nullptr;phase=Committed;
        if (request.count==DataflowGraphDecodeSteps) result=observed;
        return true;
    }
    DataflowProjectionAuthority* Retired() const { return phase==Committed?retired:nullptr; }
    template<class Host> bool CommittedFor(const DataflowResidentCommand& command,const Host& host) const {
        return phase==Committed && liveSlot && promoted && *liveSlot==promoted && retired==original
            && hostOwner==&host && RequestMatches(command) && RequestMatches(host.Request())
            && host.IsAcknowledged() && !host.IsRetained()
            && original->GraphStateMatches(originalState) && promoted->GraphStateMatches(candidateState);
    }
};
}
#endif
#endif
