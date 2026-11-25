/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ACTIVATION_SHARE_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_ACTIVATION_SHARE_PROTOCOL_H

#ifndef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
#error "activation sharing is confined to the non-loadable witness"
#endif
#include "PioneerQwen35DataflowActivationShareTree.h"
#include "PioneerQwen35DataflowActivationShareLayout.h"
#include "PioneerQwen35DataflowActivationReuse.h"
#include "PioneerQwen35DataflowActivationSums.h"

namespace PioneerQwen35 {

struct alignas(64) DataflowActivationSharePacket { uint32_t word[16]; };
static_assert(sizeof(DataflowActivationSharePacket)==DataflowActivationSharePacketBytes
    && DataflowActivationReuseAddress+sizeof(DataflowActivationReuse)<=DataflowActivationShareReadyAddress,
    "sharing packets must not overlap activation reuse metadata");

// An epoch/step pair must not recur until all ready packets have been cleared
// while the fleet is held. This header does not establish that prerequisite.
inline bool DataflowActivationSharePacketBuild(const DataflowCoreInvocation& program,
    unsigned epoch,unsigned step,unsigned producer,DataflowActivationSharePacket& packet)
{
    DataflowActivationShareTreePosition self{},peer{};
    if (!epoch || !step || !program.assignment.active
        || !DataflowActivationShareValid(program)
        || !DataflowActivationShareTreeLocate(program.sharing,program.core,self)
        || !DataflowActivationShareTreeLocate(program.sharing,producer,peer)) return false;
    for (unsigned i=0;i<16;i++) packet.word[i]=0;
    packet.word[0]=DataflowActivationShareReadyMagic;
    packet.word[1]=epoch;packet.word[2]=step;packet.word[3]=producer;
    packet.word[4]=program.sharing.leader;packet.word[5]=program.sharing.packing;
    packet.word[6]=program.sharing.inputKey;packet.word[7]=program.inputRows;
    for (unsigned i=0;i<DataflowActivationShareMaskWords;i++) packet.word[8+i]=program.sharing.members[i];
    packet.word[14]=epoch;packet.word[15]=step;
    return true;
}

inline bool DataflowActivationSharePacketMatches(const DataflowActivationSharePacket& actual,
    const DataflowCoreInvocation& program,unsigned epoch,unsigned step,unsigned producer)
{
    DataflowActivationSharePacket expected{};
    if (!DataflowActivationSharePacketBuild(program,epoch,step,producer,expected)) return false;
    for (unsigned i=0;i<16;i++) if (actual.word[i]!=expected.word[i]) return false;
    return true;
}

// The source cache and sums have already been prepared and remain immutable.
// L1 visibility precedes the two identifying commit words. A peer accepts a
// packet only when both commits and the complete group identity match.
inline void DataflowActivationSharePublish(volatile uint32_t* target,
    const DataflowActivationSharePacket& packet)
{
    DataflowStoreRelease(target[14],0);DataflowStoreRelease(target[15],0);
    DataflowL1StoresVisible(target+15);
    for (unsigned i=0;i<14;i++) DataflowStoreRelease(target[i],packet.word[i]);
    DataflowL1StoresVisible(target+13);
    DataflowStoreRelease(target[14],packet.word[14]);
    DataflowStoreRelease(target[15],packet.word[15]);
    DataflowL1StoresVisible(target+15);
}

// An individual peer read can only target this worker's tree parent and the
// exact ready packet, packed activation, or sums ranges. This predicate does
// not by itself prove NoC ownership, parent readiness or a safe buffer life.
inline bool DataflowActivationSharePeerReadAllowed(const DataflowCoreInvocation& program,
    unsigned parent,unsigned route,unsigned remote,unsigned local,unsigned bytes)
{
    DataflowActivationShareTreePosition self{};
    if (!program.assignment.active || !DataflowActivationShareValid(program)
        || !DataflowActivationShareTreeLocate(program.sharing,program.core,self)
        || self.parent!=parent || parent>=DataflowCoreCount
        || route!=DataflowBarrierWorkerRoute(parent) || !bytes || (bytes&63)
        || bytes>DataflowActivationShareMaxReadBytes || (remote&63) || (local&63)) return false;
    if (remote==DataflowActivationShareReadyAddress)
        return local==DataflowActivationShareScratchAddress && bytes==DataflowActivationSharePacketBytes;
    if (remote!=local || !program.inputs || (program.inputs%256)
        || program.inputs>PioneerQwen35Projection::MaxInputs) return false;
    const unsigned packedBytes=(program.inputs/256)*DataflowInputPageBytes;
    const unsigned sumsBytes=(program.inputs/256)*DataflowActivationSumsPerChunk*sizeof(int32_t);
    const auto within=[&](unsigned start,unsigned length) {
        return remote>=start && remote-start<length && bytes<=length-(remote-start);
    };
    return within(DataflowActivationCacheAddress,packedBytes)
        || within(DataflowActivationCacheAddress+DataflowActivationSumsOffset,sumsBytes);
}

// IO supplies the bounded, checked NoC peer read and access to the local
// scratch record. A failed read is uncertain and is never retried. A stale or
// incomplete ready record can be polled until the caller's bounded deadline.
template<class IO>
bool DataflowActivationShareReceive(const DataflowCoreInvocation& program,
    unsigned epoch,unsigned step,IO& io,unsigned pollLimit)
{
    DataflowActivationShareTreePosition tree{};
    if (!epoch || !step || !pollLimit || !program.assignment.active
        || !program.inputs || program.inputs%256 || program.inputs!=program.inputRows
        || program.inputs>PioneerQwen35Projection::MaxInputs
        || !DataflowActivationShareValid(program)
        || !DataflowActivationShareTreeLocate(program.sharing,program.core,tree)
        || tree.parent>=DataflowCoreCount) return false;
    bool ready=false;
    for (unsigned attempt=0;attempt<pollLimit;attempt++) {
        if (!io.ContextMatches(epoch,step)
            || !io.Read(tree.parent,DataflowActivationShareReadyAddress,
                DataflowActivationShareScratchAddress,DataflowActivationSharePacketBytes)) return false;
        DataflowActivationSharePacket packet{};
        for (unsigned i=0;i<16;i++) packet.word[i]=io.ScratchWord(i);
        if (DataflowActivationSharePacketMatches(packet,program,epoch,step,tree.parent)) { ready=true;break; }
    }
    if (!ready) return false;
    auto copy=[&](unsigned address,unsigned bytes) {
        while (bytes) {
            const unsigned chunk=bytes>DataflowActivationShareMaxReadBytes
                ?unsigned(DataflowActivationShareMaxReadBytes):bytes;
            if (!io.ContextMatches(epoch,step) || !io.Read(tree.parent,address,address,chunk)) return false;
            address+=chunk;bytes-=chunk;
        }
        return true;
    };
    return copy(DataflowActivationCacheAddress,(program.inputs/256)*DataflowInputPageBytes)
        && copy(DataflowActivationCacheAddress+DataflowActivationSumsOffset,
            (program.inputs/256)*DataflowActivationSumsPerChunk*sizeof(int32_t))
        && io.ContextMatches(epoch,step);
}

}
#endif
