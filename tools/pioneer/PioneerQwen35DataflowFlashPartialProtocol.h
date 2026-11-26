/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FLASH_PARTIAL_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_FLASH_PARTIAL_PROTOCOL_H
#include "PioneerQwen35DataflowDenseAttentionHybrid.h"
#include "PioneerQwen35DataflowDeviceBarrier.h"

namespace PioneerQwen35 {
// These L1 regions overlap the activation cache by design. The fleet owner
// must prove the previous projection/activation users and role queues idle,
// reserve this layout through the final reducer barrier, and restore it before
// any later user. No live service or driver currently grants that authority.
enum : unsigned {
    DataflowFlashPartialMagic=0x31504641,
    DataflowFlashPartialHeaderBytes=64,
    DataflowFlashPartialDataBytes=DenseHeadWidth*sizeof(float),
    DataflowFlashPartialBytes=DataflowFlashPartialHeaderBytes+DataflowFlashPartialDataBytes,
    DataflowFlashPartialBase=DataflowActivationCacheAddress+DataflowFlashHybridScratchBytes,
    DataflowFlashPartialHeaderScratch=DataflowFlashPartialBase+DataflowFlashQueriesPerKv*DataflowFlashPartialBytes,
    DataflowFlashPartialHeaderAgain=DataflowFlashPartialHeaderScratch+DataflowFlashPartialHeaderBytes,
    DataflowFlashPartialLocalReceipt=DataflowFlashPartialHeaderAgain+DataflowFlashPartialHeaderBytes,
    DataflowFlashPartialIncoming=DataflowActivationCacheAddress+DataflowFlashHybridScratchBytes-DataflowFlashPartialDataBytes
};
static_assert(!(DataflowFlashPartialBase&63) && !(DataflowFlashPartialBytes&63)
    && !(DataflowFlashPartialIncoming&63)
    && DataflowFlashPartialLocalReceipt+DataflowFlashPartialHeaderBytes
        <=DataflowActivationCacheAddress+DataflowActivationCacheBytes,
    "hybrid scratch and partial packets must fit the fixed activation cache");
struct DataflowFlashPartialIdentity { unsigned epoch,step,layer,position,producer,query; };
inline bool DataflowFlashPartialProducerStep(unsigned invocationStep,bool reduce,unsigned& producerStep) {
    producerStep=0;
    if (!invocationStep || (reduce && invocationStep<2)) return false;
    producerStep=reduce?invocationStep-1:invocationStep;return true;
}
inline bool DataflowFlashPartialProducerMatches(const DataflowFlashPartialIdentity& id,unsigned producer) {
    return id.producer==producer;
}
struct alignas(64) DataflowFlashPartialHeader { uint32_t word[16]; };
static_assert(sizeof(DataflowFlashPartialHeader)==DataflowFlashPartialHeaderBytes,"partial header fits one packet");

// Per-core completion of the producer operation. The caller clears this
// before starting and seals it only after every local partial is published.
// It is not a fleet barrier or a proof of NoC/role quiescence.
inline bool DataflowFlashProducerReceiptBuild(unsigned epoch,unsigned step,unsigned layer,
    unsigned position,unsigned core,DataflowFlashPartialHeader& out) {
    DataflowFlashHybridPartition partition{};
    if (!epoch || !step || layer>=Layers || !Dense(layer)
        || !DataflowFlashHybridAssign(core,position,partition)) return false;
    DataflowFlashPartialHeader h{};
    h.word[0]=0x31524641;h.word[1]=epoch;h.word[2]=step;h.word[3]=layer;
    h.word[4]=position;h.word[5]=core;
    uint32_t checksum=2166136261U;
    for (unsigned i=0;i<6;i++) checksum=(checksum^h.word[i])*16777619U;
    h.word[6]=checksum;h.word[7]=~checksum;h.word[14]=epoch;h.word[15]=step;
    out=h;return true;
}
inline void DataflowFlashProducerReceiptClear(volatile uint32_t* words) {
    DataflowStoreRelease(words[14],0);DataflowStoreRelease(words[15],0);
    DataflowL1StoresVisible(words+15);
}
inline bool DataflowFlashProducerReceiptPublish(volatile uint32_t* words,unsigned epoch,unsigned step,
    unsigned layer,unsigned position,unsigned core) {
    DataflowFlashPartialHeader receipt{};
    if (!words || !DataflowFlashProducerReceiptBuild(epoch,step,layer,position,core,receipt)) return false;
    DataflowFlashProducerReceiptClear(words);
    for (unsigned i=0;i<14;i++) words[i]=receipt.word[i];
    DataflowL1StoresVisible(words+13);
    DataflowStoreRelease(words[14],epoch);DataflowStoreRelease(words[15],step);
    DataflowL1StoresVisible(words+15);return true;
}
inline bool DataflowFlashProducerReceiptMatches(const volatile uint32_t* words,unsigned epoch,unsigned step,
    unsigned layer,unsigned position,unsigned core) {
    DataflowFlashPartialHeader expected{};
    if (!words || !DataflowFlashProducerReceiptBuild(epoch,step,layer,position,core,expected)
        || DataflowLoadAcquire(words[14])!=epoch || DataflowLoadAcquire(words[15])!=step) return false;
    for (unsigned i=0;i<14;i++) if (DataflowLoadAcquire(words[i])!=expected.word[i]) return false;
    return DataflowLoadAcquire(words[14])==epoch && DataflowLoadAcquire(words[15])==step;
}

inline bool DataflowFlashPartialIdentityRange(const DataflowFlashPartialIdentity& id,
    unsigned& first,unsigned& count) {
    DataflowFlashHybridPartition part{};
    return id.epoch && id.step && id.layer<Layers && Dense(id.layer)
        && DataflowFlashHybridAssign(id.producer,id.position,part)
        && DataflowFlashHybridRange(part,id.query,first,count) && count;
}
inline uint32_t DataflowFlashPartialHash(const float* values) {
    uint32_t h=2166136261U;
    for (unsigned i=0;i<DenseHeadWidth;i++) h=(h^PioneerAttentionBits(values[i]))*16777619U;
    return h;
}
inline uint32_t DataflowFlashPartialHeaderHash(const DataflowFlashPartialHeader& h) {
    uint32_t value=2166136261U;
    for (unsigned i=0;i<12;i++) value=(value^h.word[i])*16777619U;
    return value;
}
inline bool DataflowFlashPartialHeaderBuild(const DataflowFlashPartialIdentity& id,
    const DataflowDenseAttentionRow& row,DataflowFlashPartialHeader& out) {
    unsigned first=0,count=0;
    if (!DataflowFlashPartialIdentityRange(id,first,count) || !row.sums
        || !(row.denominator>0) || !DataflowDenseFinite(row.denominator)
        || !DataflowDenseFinite(row.maximum)) return false;
    for (unsigned i=0;i<DenseHeadWidth;i++)
        if (!DataflowDenseFinite(row.sums[i]) || ((i<first || i>=first+count) && row.sums[i]!=0.0f)) return false;
    DataflowFlashPartialHeader h{};
    h.word[0]=DataflowFlashPartialMagic;h.word[1]=id.epoch;h.word[2]=id.step;
    h.word[3]=id.layer;h.word[4]=id.position;h.word[5]=id.producer;h.word[6]=id.query;
    h.word[7]=first;h.word[8]=count;h.word[9]=PioneerAttentionBits(row.maximum);
    h.word[10]=PioneerAttentionBits(row.denominator);h.word[11]=DataflowFlashPartialHash(row.sums);
    h.word[12]=DataflowFlashPartialHeaderHash(h);h.word[13]=~h.word[12];
    h.word[14]=id.epoch;h.word[15]=id.step;out=h;return true;
}
inline bool DataflowFlashPartialHeaderMatches(const DataflowFlashPartialIdentity& id,
    const DataflowFlashPartialHeader& h) {
    unsigned first=0,count=0;
    return DataflowFlashPartialIdentityRange(id,first,count)
        && h.word[0]==DataflowFlashPartialMagic && h.word[1]==id.epoch && h.word[2]==id.step
        && h.word[3]==id.layer && h.word[4]==id.position && h.word[5]==id.producer
        && h.word[6]==id.query && h.word[7]==first && h.word[8]==count
        && h.word[12]==DataflowFlashPartialHeaderHash(h) && h.word[13]==~h.word[12]
        && h.word[14]==id.epoch && h.word[15]==id.step
        && DataflowDenseFinite(PioneerAttentionFloat(h.word[9]))
        && DataflowDenseFinite(PioneerAttentionFloat(h.word[10]))
        && PioneerAttentionFloat(h.word[10])>0;
}
inline bool DataflowFlashPartialSnapshot(const DataflowFlashPartialIdentity& id,
    const DataflowFlashPartialHeader& first,const DataflowFlashPartialHeader& again,
    const float* values,DataflowDenseAttentionRow& out) {
    if (!values || !out.sums || !DataflowFlashPartialHeaderMatches(id,first)
        || !DataflowFlashPartialHeaderMatches(id,again)) return false;
    for (unsigned i=0;i<16;i++) if (first.word[i]!=again.word[i]) return false;
    if (DataflowFlashPartialHash(values)!=first.word[11]) return false;
    const unsigned lane=first.word[7],count=first.word[8];
    for (unsigned i=0;i<DenseHeadWidth;i++)
        if (!DataflowDenseFinite(values[i]) || ((i<lane || i>=lane+count) && values[i]!=0.0f)) return false;
    for (unsigned i=0;i<DenseHeadWidth;i++) out.sums[i]=values[i];
    out.maximum=PioneerAttentionFloat(first.word[9]);out.denominator=PioneerAttentionFloat(first.word[10]);
    return true;
}
// This only authorizes exact fixed peer reads for a reducer's own slice. It
// does not prove barriers, peer lifetime, NoC ownership or hardware safety.
inline bool DataflowFlashPartialReadShape(const DataflowFlashPartialIdentity& id,
    unsigned remote,unsigned local,unsigned bytes) {
    const unsigned base=DataflowFlashPartialBase+id.query*DataflowFlashPartialBytes;
    return (remote==base && bytes==DataflowFlashPartialHeaderBytes
                && (local==DataflowFlashPartialHeaderScratch || local==DataflowFlashPartialHeaderAgain))
        || (remote==base+DataflowFlashPartialHeaderBytes && bytes==DataflowFlashPartialDataBytes
                && local==DataflowFlashPartialIncoming);
}
inline bool DataflowFlashPartialPeerReadAllowed(unsigned consumer,const DataflowFlashPartialIdentity& id,
    unsigned route,unsigned remote,unsigned local,unsigned bytes) {
    DataflowFlashHybridPartition c{},p{};unsigned first=0,count=0;
    return consumer!=id.producer && DataflowFlashPartialIdentityRange(id,first,count)
        && DataflowFlashHybridAssign(consumer,id.position,c)
        && DataflowFlashHybridAssign(id.producer,id.position,p)
        && !c.positionShard && c.kvHead==p.kvHead && c.outputShard==p.outputShard
        && c.positionShards==p.positionShards && c.outputShards==p.outputShards
        && route==DataflowBarrierWorkerRoute(id.producer)
        && DataflowFlashPartialReadShape(id,remote,local,bytes);
}
inline bool DataflowFlashPartialLocalReadAllowed(unsigned consumer,const DataflowFlashPartialIdentity& id,
    unsigned remote,unsigned local,unsigned bytes) {
    DataflowFlashHybridPartition c{};unsigned first=0,count=0;
    return consumer==id.producer && DataflowFlashPartialIdentityRange(id,first,count)
        && DataflowFlashHybridAssign(consumer,id.position,c) && !c.positionShard
        && DataflowFlashPartialReadShape(id,remote,local,bytes);
}
// The source packet stays immutable until every consumer finishes. Clear the
// two commits first; make payload and header visible before committing them.
inline bool DataflowFlashPartialPublish(volatile uint32_t* target,const DataflowFlashPartialIdentity& id,
    const DataflowDenseAttentionRow& row) {
    DataflowFlashPartialHeader h{};
    if (!target || !DataflowFlashPartialHeaderBuild(id,row,h)) return false;
    DataflowStoreRelease(target[14],0);DataflowStoreRelease(target[15],0);
    DataflowL1StoresVisible(target+15);
    for (unsigned i=0;i<DenseHeadWidth;i++) target[16+i]=PioneerAttentionBits(row.sums[i]);
    DataflowL1StoresVisible(target+16+DenseHeadWidth-1);
    for (unsigned i=0;i<14;i++) target[i]=h.word[i];
    DataflowL1StoresVisible(target+13);
    DataflowStoreRelease(target[14],h.word[14]);DataflowStoreRelease(target[15],h.word[15]);
    DataflowL1StoresVisible(target+15);return true;
}

// The caller already proved the producer barrier and retains all source
// packets through the consumer barrier. The IO must validate its live graph
// context and perform checked reads (or local copies for the self packet).
// Any failed/uncertain read is terminal; this helper never retries it.
template<class IO>
bool DataflowFlashPartialReceive(unsigned consumer,const DataflowFlashPartialIdentity& id,
    IO& io,DataflowDenseAttentionRow& out) {
    DataflowFlashHybridPartition c{},p{};unsigned first=0,count=0;
    if (!out.sums || !DataflowFlashPartialIdentityRange(id,first,count)
        || !DataflowFlashHybridAssign(consumer,id.position,c)
        || !DataflowFlashHybridAssign(id.producer,id.position,p)
        || c.positionShard || c.kvHead!=p.kvHead || c.outputShard!=p.outputShard
        || c.positionShards!=p.positionShards || c.outputShards!=p.outputShards) return false;
    const unsigned base=DataflowFlashPartialBase+id.query*DataflowFlashPartialBytes;
    const unsigned route=DataflowBarrierWorkerRoute(id.producer);
    if (consumer!=id.producer
        && (!DataflowFlashPartialPeerReadAllowed(consumer,id,route,base,
                DataflowFlashPartialHeaderScratch,DataflowFlashPartialHeaderBytes)
            || !DataflowFlashPartialPeerReadAllowed(consumer,id,route,base+DataflowFlashPartialHeaderBytes,
                DataflowFlashPartialIncoming,DataflowFlashPartialDataBytes)
            || !DataflowFlashPartialPeerReadAllowed(consumer,id,route,base,
                DataflowFlashPartialHeaderAgain,DataflowFlashPartialHeaderBytes))) return false;
    if (consumer==id.producer
        && (!DataflowFlashPartialLocalReadAllowed(consumer,id,base,
                DataflowFlashPartialHeaderScratch,DataflowFlashPartialHeaderBytes)
            || !DataflowFlashPartialLocalReadAllowed(consumer,id,base+DataflowFlashPartialHeaderBytes,
                DataflowFlashPartialIncoming,DataflowFlashPartialDataBytes)
            || !DataflowFlashPartialLocalReadAllowed(consumer,id,base,
                DataflowFlashPartialHeaderAgain,DataflowFlashPartialHeaderBytes))) return false;
    DataflowFlashPartialHeader header{},again{};
    if (!io.ContextMatches(consumer,id)
        || !io.Read(consumer,id,base,DataflowFlashPartialHeaderScratch,DataflowFlashPartialHeaderBytes)) return false;
    for (unsigned i=0;i<16;i++) header.word[i]=io.Word(DataflowFlashPartialHeaderScratch,i);
    if (!DataflowFlashPartialHeaderMatches(id,header) || !io.ContextMatches(consumer,id)
        || !io.Read(consumer,id,base+DataflowFlashPartialHeaderBytes,DataflowFlashPartialIncoming,DataflowFlashPartialDataBytes)
        || !io.ContextMatches(consumer,id)
        || !io.Read(consumer,id,base,DataflowFlashPartialHeaderAgain,DataflowFlashPartialHeaderBytes)) return false;
    for (unsigned i=0;i<16;i++) again.word[i]=io.Word(DataflowFlashPartialHeaderAgain,i);
    return io.ContextMatches(consumer,id)
        && DataflowFlashPartialSnapshot(id,header,again,io.Values(),out);
}
}
#endif
