/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_FLASH_HYBRID_IO_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_FLASH_HYBRID_IO_H
#if !defined(PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) \
    || defined(PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL)
#error Flash hybrid IO is an isolated non-loadable graph witness
#endif
#include "PioneerQwen35DataflowFlashPartialProtocol.h"
namespace PioneerQwen35 {

// The caller must reserve and hold this complete disjoint cache layout until
// every reducer and NoC transaction has settled. This function establishes
// addresses, not ownership or safety of repurposing activation-cache data.
inline DataflowFlashHybridScratch DataflowBlackholeFlashHybridScratch() {
    constexpr unsigned tile=DataflowFlashHybridTilePositions*DataflowDenseAttentionHeadBytes;
    constexpr unsigned q=DataflowFlashQueriesPerKv*DenseHeadWidth*sizeof(float);
    constexpr unsigned width=DenseHeadWidth*sizeof(float);
    auto ptr=[](unsigned address) {return reinterpret_cast<float*>(static_cast<uintptr_t>(address));};
    const unsigned base=DataflowActivationCacheAddress;
    return {ptr(base),ptr(base+tile),ptr(base+2*tile),ptr(base+2*tile+q),
        ptr(base+2*tile+2*q),ptr(base+2*tile+2*q+width),
        ptr(base+2*tile+2*q+width+Rotary*sizeof(float)),ptr(DataflowFlashPartialIncoming)};
}

// Context must independently hold the current graph, core, ticket, layer and
// position, prove the source packet is within the retained producer interval,
// and prove exclusive NoC1 use. No production context currently provides this
// authority; a false or changed context aborts without retry. Producer and
// reducer still require an intervening all-fleet visibility barrier and must
// preserve packets through the reducer barrier.
template<class Context> class DataflowBlackholeFlashHybridIO : public DataflowBlackholeDenseAttentionOps {
    Context& fContext;DataflowBlackholeNoc<1> fPeer;
    unsigned fCore,fEpoch,fStep,fProducerStep;bool fReduce;
    const DataflowDenseAttentionProgram* fProgram=nullptr;
    bool Matches(unsigned core,const DataflowFlashPartialIdentity& id) const {
        return fProgram && core==fCore && id.epoch==fEpoch && id.step==fProducerStep
            && id.layer==fProgram->layer && id.position==fProgram->position
            && fContext.FlashContextMatches(core,id.epoch,fStep,id.layer,id.position);
    }
public:
    // The reducer record must directly follow its producer record within the
    // same epoch; packet identity always names the producer's graph step.
    DataflowBlackholeFlashHybridIO(Context& context,unsigned core,unsigned epoch,unsigned step,bool reduce)
        : fContext(context),fCore(core),fEpoch(epoch),fStep(step),
          fProducerStep(0),fReduce(reduce) {DataflowFlashPartialProducerStep(step,reduce,fProducerStep);}
    bool BindShardProgram(const DataflowDenseAttentionProgram& program) {
        fProgram=nullptr;
        if (fCore>=DataflowCoreCount || !fEpoch || !fStep || !fProducerStep
            || !fContext.FlashContextMatches(fCore,fEpoch,fStep,program.layer,program.position)
            || !DataflowBlackholeDenseAttentionOps::BindShardProgram(program)) return false;
        fProgram=&program;return true;
    }
    bool PublishPartial(unsigned core,unsigned query,unsigned first,unsigned count,
        const DataflowDenseAttentionRow& row) {
        if (fReduce || !fProgram || core!=fCore || query>=DataflowFlashQueriesPerKv) return false;
        const DataflowFlashPartialIdentity id{fEpoch,fProducerStep,fProgram->layer,fProgram->position,core,query};
        unsigned expectedFirst=0,expectedCount=0;
        const auto scratch=DataflowBlackholeFlashHybridScratch();
        if (!Matches(core,id) || !DataflowFlashPartialIdentityRange(id,expectedFirst,expectedCount)
            || first!=expectedFirst || count!=expectedCount || row.sums!=scratch.sums+query*DenseHeadWidth) return false;
        auto* target=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
            DataflowFlashPartialBase+query*DataflowFlashPartialBytes));
        return DataflowFlashPartialPublish(target,id,row) && Matches(core,id);
    }
    bool ReadPartial(unsigned producer,unsigned query,unsigned first,unsigned count,DataflowDenseAttentionRow& row) {
        if (!fReduce || !fProgram || row.sums!=DataflowBlackholeFlashHybridScratch().incoming) return false;
        const DataflowFlashPartialIdentity id{fEpoch,fProducerStep,fProgram->layer,fProgram->position,producer,query};
        unsigned expectedFirst=0,expectedCount=0;
        return DataflowFlashPartialIdentityRange(id,expectedFirst,expectedCount)
            && first==expectedFirst && count==expectedCount && DataflowFlashPartialReceive(fCore,id,*this,row);
    }
    bool ContextMatches(unsigned consumer,const DataflowFlashPartialIdentity& id) const {return Matches(consumer,id);}
    bool Read(unsigned consumer,const DataflowFlashPartialIdentity& id,unsigned remote,unsigned local,unsigned bytes) {
        if (!Matches(consumer,id)) return false;
        const unsigned base=DataflowFlashPartialBase+id.query*DataflowFlashPartialBytes;
        const bool allowed=(remote==base && bytes==DataflowFlashPartialHeaderBytes
                && (local==DataflowFlashPartialHeaderScratch || local==DataflowFlashPartialHeaderAgain))
            || (remote==base+DataflowFlashPartialHeaderBytes && bytes==DataflowFlashPartialDataBytes
                && local==DataflowFlashPartialIncoming);
        if (!allowed) return false;
        if (consumer!=id.producer)
            return fPeer.ReadFlashPartialPeer(consumer,id.producer,id,remote,
                reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(local)),bytes) && Matches(consumer,id);
        if (!DataflowFlashPartialLocalReadAllowed(consumer,id,remote,local,bytes)) return false;
        const auto* source=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(remote));
        auto* destination=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(local));
        for (unsigned i=0;i<bytes/4;i++) destination[i]=DataflowLoadAcquire(source[i]);
        DataflowL1StoresVisible(destination+bytes/4-1);return Matches(consumer,id);
    }
    uint32_t Word(unsigned address,unsigned index) const {
        if ((address!=DataflowFlashPartialHeaderScratch && address!=DataflowFlashPartialHeaderAgain) || index>=16) return 0;
        const auto* words=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(address));
        return DataflowLoadAcquire(words[index]);
    }
    const float* Values() const {return reinterpret_cast<const float*>(static_cast<uintptr_t>(DataflowFlashPartialIncoming));}
};
}
#endif
