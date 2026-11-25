/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_DISPATCH_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_DISPATCH_H

#include "PioneerQwen35DataflowGraphRunProtocol.h"

namespace PioneerQwen35 {
// A single dynamic request is distributed down the existing four-way NoC
// tree. Each worker keeps its own separately installed, model-bound graph
// descriptor; the command changes only the fields which vary per token.
// This protocol alone does not establish ownership, completion or recovery.
enum : unsigned {
    DataflowResidentCommandMagic=0x31444335,
    DataflowResidentCommandVersion=1,
    DataflowResidentMarkerBit=0x40000000U,
    DataflowResidentMarkerLimit=DataflowResidentMarkerBit-1
};
struct alignas(64) DataflowResidentCommand {
    uint32_t magic,version,trigger,epoch,firstSequence,count,pollLimit,inputSlot,token,position,checksum;
    uint32_t reserved[5];
};
static_assert(sizeof(DataflowResidentCommand)==DataflowBarrierPacketBytes,
    "resident token command fits the existing 64-byte NoC packet");
inline uint32_t DataflowResidentCommandChecksum(const DataflowResidentCommand& c) {
    const uint32_t words[]={c.magic,c.version,c.trigger,c.epoch,c.firstSequence,c.count,
        c.pollLimit,c.inputSlot,c.token,c.position};
    uint32_t h=2166136261U;
    for (uint32_t word:words) h=(h^word)*16777619U;
    return h;
}
inline bool DataflowResidentCommandValid(const DataflowResidentCommand& c) {
    if (c.magic!=DataflowResidentCommandMagic || c.version!=DataflowResidentCommandVersion
        || !c.trigger || c.trigger>DataflowResidentMarkerLimit || !c.epoch
        || !c.firstSequence || c.firstSequence>DataflowResidentMarkerLimit
        || (c.count!=DataflowGraphPromptSteps && c.count!=DataflowGraphDecodeSteps)
        || c.count-1>DataflowResidentMarkerLimit-c.firstSequence
        || !c.pollLimit || c.pollLimit>DataflowGraphRunMaxPolls || c.inputSlot>=ActivationBuffers
        || c.token>=Vocabulary || c.checksum!=DataflowResidentCommandChecksum(c)) return false;
    for (uint32_t word:c.reserved) if (word) return false;
    return true;
}
inline bool DataflowResidentCommandBuild(const DataflowGraphRunDescriptor& run,
    DataflowResidentCommand& out) {
    if (!DataflowGraphRunValid(run,run.core)) return false;
    DataflowResidentCommand c{};
    c.magic=DataflowResidentCommandMagic;c.version=DataflowResidentCommandVersion;
    c.trigger=run.triggerSequence;c.epoch=run.ticket.epoch;c.firstSequence=run.ticket.firstSequence;
    c.count=run.ticket.count;c.pollLimit=run.pollLimit;c.inputSlot=run.inputSlot;
    c.token=run.token.token;c.position=run.token.position;c.checksum=DataflowResidentCommandChecksum(c);
    if (!DataflowResidentCommandValid(c)) return false;
    out=c;return true;
}
// Validate each worker's immutable, already installed descriptor and rebuild
// its dynamic fields locally. Prior sequence values belong to the resident
// session; this rejects stale commands and overlapping graph-step tickets.
inline bool DataflowResidentCommandExpand(const DataflowResidentCommand& c,
    const DataflowGraphRunDescriptor& installed,unsigned core,unsigned completed,
    unsigned priorEpoch,unsigned priorLastStep,DataflowGraphRunDescriptor& out) {
    if (!DataflowResidentCommandValid(c) || !DataflowGraphRunValid(installed,core)
        || c.trigger<=completed || c.epoch<=priorEpoch || c.firstSequence<=priorLastStep
        || c.inputSlot!=installed.inputSlot) return false;
    auto token=installed.token;token.token=c.token;token.position=c.position;
    return DataflowGraphRunBuild(installed.binding,installed.lease,installed.routes,token,
        {c.epoch,c.firstSequence,c.count},c.trigger,c.pollLimit,c.inputSlot,out);
}
inline DataflowBarrierPacket DataflowResidentCommandPayload(const DataflowResidentCommand& c) {
    DataflowBarrierPacket p{};
    const auto* bytes=reinterpret_cast<const uint8_t*>(&c);
    for (unsigned i=0;i<16;i++) p.word[i]=uint32_t(bytes[4*i]) | (uint32_t(bytes[4*i+1])<<8)
        | (uint32_t(bytes[4*i+2])<<16) | (uint32_t(bytes[4*i+3])<<24);
    return p;
}
inline DataflowBarrierPacket DataflowResidentCommandCommit(const DataflowResidentCommand& c) {
    return DataflowBarrierCommit(c.epoch,DataflowResidentMarkerBit|c.trigger);
}
// A valid commit distinguishes a new request from ordinary graph barriers.
// Old requests are ignored; a committed but malformed new request is fatal.
inline DataflowBarrierReadResult DataflowResidentCommandRead(const volatile uint32_t* source,
    const volatile uint32_t* marker,unsigned completed,unsigned priorEpoch,DataflowResidentCommand& out) {
    const unsigned epoch=DataflowLoadAcquire(marker[0]),step=DataflowLoadAcquire(marker[2]);
    if (!epoch || (step&0xc0000000U)!=DataflowResidentMarkerBit
        || !DataflowBarrierCommitMatches(marker,epoch,step)) return DataflowBarrierWaiting;
    const unsigned trigger=step&DataflowResidentMarkerLimit;
    if (trigger<=completed && epoch<=priorEpoch) return DataflowBarrierWaiting;
    DataflowResidentCommand c{};auto* bytes=reinterpret_cast<uint8_t*>(&c);
    for (unsigned i=0;i<16;i++) {
        const uint32_t word=DataflowLoadAcquire(source[i]);
        for (unsigned byte=0;byte<4;byte++) bytes[4*i+byte]=uint8_t(word>>(8*byte));
    }
    if (!DataflowBarrierCommitMatches(marker,epoch,step) || !DataflowResidentCommandValid(c)
        || c.epoch!=epoch || c.trigger!=trigger || c.epoch<=priorEpoch || c.trigger<=completed)
        return DataflowBarrierInvalid;
    out=c;return DataflowBarrierReady;
}
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
// Root inbox only: the host commits a four-word marker after publishing and
// verifying the command. Child inboxes retain the full 64-byte NoC marker.
inline bool DataflowResidentRootCommitMatches(const volatile uint32_t* marker,unsigned epoch,unsigned step) {
    return DataflowLoadAcquire(marker[0])==epoch && DataflowLoadAcquire(marker[1])==~epoch
        && DataflowLoadAcquire(marker[2])==step && DataflowLoadAcquire(marker[3])==~step;
}
inline DataflowBarrierReadResult DataflowResidentRootCommandRead(const volatile uint32_t* source,
    const volatile uint32_t* marker,unsigned completed,unsigned priorEpoch,DataflowResidentCommand& out) {
    const unsigned epoch=DataflowLoadAcquire(marker[0]),step=DataflowLoadAcquire(marker[2]);
    if (!epoch || (step&0xc0000000U)!=DataflowResidentMarkerBit
        || !DataflowResidentRootCommitMatches(marker,epoch,step)) return DataflowBarrierWaiting;
    const unsigned trigger=step&DataflowResidentMarkerLimit;
    if (trigger<=completed && epoch<=priorEpoch) return DataflowBarrierWaiting;
    DataflowResidentCommand c{};auto* bytes=reinterpret_cast<uint8_t*>(&c);
    for (unsigned i=0;i<16;i++) {
        const uint32_t word=DataflowLoadAcquire(source[i]);
        for (unsigned byte=0;byte<4;byte++) bytes[4*i+byte]=uint8_t(word>>(8*byte));
    }
    if (!DataflowResidentRootCommitMatches(marker,epoch,step) || !DataflowResidentCommandValid(c)
        || c.epoch!=epoch || c.trigger!=trigger || c.epoch<=priorEpoch || c.trigger<=completed)
        return DataflowBarrierInvalid;
    out=c;return DataflowBarrierReady;
}
#endif
// IO sends the payload followed by its commit to one fixed child. A failed
// send may have reached the peer: the operation is one-shot and must retain
// the fleet rather than replaying any part of the fanout.
class DataflowResidentFanout {
    bool attempted=false,complete=false;
public:
    DataflowResidentFanout()=default;
    DataflowResidentFanout(const DataflowResidentFanout&)=delete;
    DataflowResidentFanout& operator=(const DataflowResidentFanout&)=delete;
    template<class IO> bool Send(unsigned core,const DataflowResidentCommand& command,IO& io) {
        if (attempted || core>=DataflowBarrierWorkers || !DataflowResidentCommandValid(command)) return false;
        attempted=true;
        const auto payload=DataflowResidentCommandPayload(command);
        const auto commit=DataflowResidentCommandCommit(command);
        const unsigned first=core*DataflowBarrierReleaseFanout+1;
        for (unsigned child=first;child<first+DataflowBarrierReleaseFanout && child<DataflowBarrierWorkers;child++)
            if (!io.SendResidentDispatch(core,child,payload,commit)) return false;
        complete=true;return true;
    }
    bool Attempted() const { return attempted; }
    bool Complete() const { return complete; }
};
}
#endif
