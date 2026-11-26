/* SPDX-License-Identifier: MIT */
// Actual shard worker compared with separate host attention operators on fake IO.
#define PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define main q35_dense_unsharded_test_main
#include "pioneer_qwen35_dataflow_dense_attention_worker_test.cpp"
#undef main
#include "../PioneerQwen35DataflowDenseAttentionShards.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

struct ShardIO : MockAttentionIO {
    DataflowDenseAttentionInvocation bound{};
    unsigned* writes=nullptr;
    bool failWrite=false;unsigned kvReads=0;
    bool BindShardProgram(const DataflowDenseAttentionProgram& p) {
        return DataflowDenseAttentionProgramBuilder::BindCore(p,0,bound) && Bind(bound);
    }
    bool WriteContextSlice(unsigned first,unsigned rows,const float* input) {
        if (failWrite || !input || !rows || ((first|rows)&15)
            || first>=DenseHeads*DenseHeadWidth || rows>DenseHeads*DenseHeadWidth-first) return false;
        for (unsigned i=0;i<rows;i++) { assert(!writes[first+i]);++writes[first+i];context[first+i]=input[i]; }
        return true;
    }
    bool ReadKvTile(unsigned head,bool value,unsigned first,unsigned positions,float* output) {
        ++kvReads;return MockAttentionIO::ReadKvTile(head,value,first,positions,output);
    }
};
struct Scratch {
    float key[DataflowDenseAttentionTilePositions*DenseHeadWidth]{};
    float value[DataflowDenseAttentionTilePositions*DenseHeadWidth]{};
    float query[DenseHeadWidth]{},gate[DenseHeadWidth]{},norm[DenseHeadWidth]{},rope[Rotary]{},output[DenseHeadWidth]{};
    DataflowDenseAttentionShardScratch Get() { return {key,value,query,gate,norm,rope,output}; }
};
int main() {
    (void)&q35_dense_unsharded_test_main;
    std::array<unsigned,DenseHeads*DenseHeadWidth> coverage{};
    unsigned workers[DenseHeads]{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowDenseAttentionShard shard{};assert(DataflowDenseAttentionShardAssign(core,shard));
        assert(shard.head<DenseHeads && !(shard.firstLane&15) && !(shard.laneCount&15) && shard.laneCount);
        ++workers[shard.head];
        for (unsigned lane=shard.firstLane;lane<shard.firstLane+shard.laneCount;lane++)
            assert(++coverage[shard.head*DenseHeadWidth+lane]==1);
    }
    DataflowDenseAttentionShard invalid{};
    assert(!DataflowDenseAttentionShardAssign(DataflowCoreCount,invalid));
    for (unsigned head=0;head<DenseHeads;head++) assert(workers[head]==(head<8?8U:7U));
    for (unsigned count:coverage) assert(count==1);
    for (unsigned position:{0U,2U,33U,63U}) {
        constexpr unsigned contextLength=64;
        auto p=Program(DataflowDenseAttentionRun);p.position=position;p.context=contextLength;
        FlatRows(p.kv,2*DenseKeyHeads,contextLength*DenseHeadWidth*sizeof(uint16_t),1,0x20000000,1);
        p.rope.bytes=contextLength*Rotary*sizeof(float);assert(p.Valid());
        std::vector<float> qg(ConvChannels),keys(DeltaInner),values(DeltaInner),queryNorm(DenseHeadWidth),keyNorm(DenseHeadWidth),rope(Rotary);
        std::vector<uint16_t> kv(2*DenseKeyHeads*contextLength*DenseHeadWidth);
        std::vector<float> expected(DenseHeads*DenseHeadWidth),actual(expected.size());
        std::vector<unsigned> writes(expected.size());
        for (unsigned i=0;i<qg.size();i++) qg[i]=Sample(i,position+1);
        for (unsigned i=0;i<DenseHeadWidth;i++) {queryNorm[i]=0.75f+float(i%23)*0.002f;keyNorm[i]=0.8f+float(i%19)*0.002f;}
        for (unsigned i=0;i<Rotary;i++) rope[i]=float(int(i%13)-6)*0.025f;
        for (unsigned head=0;head<2*DenseKeyHeads;head++) for (unsigned at=0;at<contextLength;at++)
            for (unsigned lane=0;lane<DenseHeadWidth;lane++)
                kv[(head*contextLength+at)*DenseHeadWidth+lane]=PioneerAttentionToHalf(Sample(lane,head+at+5));
        auto scratch=std::make_unique<Scratch>();
        for (unsigned head=0;head<DenseHeads;head++) {
            float query[DenseHeadWidth];uint32_t key[DenseHeadWidth/2],value[DenseHeadWidth/2];
            const float* qgHead=qg.data()+2*head*DenseHeadWidth;
            std::memcpy(query,qgHead,sizeof(query));
            assert(Rms<DenseHeadWidth>(query,queryNorm.data(),query) && Rotate(query,rope.data()));
            AttentionRow row{expected.data()+head*DenseHeadWidth};
            const unsigned kvHead=head/(DenseHeads/DenseKeyHeads);
            for (unsigned at=0;at<=position;at++) {
                std::memcpy(key,kv.data()+(kvHead*contextLength+at)*DenseHeadWidth,sizeof(key));
                std::memcpy(value,kv.data()+((kvHead+DenseKeyHeads)*contextLength+at)*DenseHeadWidth,sizeof(value));
                assert(row.Add(query,key,value));
            }
            assert(row.Finish(qgHead+DenseHeadWidth));
        }
        ShardIO io;io.qg=qg.data();io.keys=keys.data();io.values=values.data();io.queryNorm=queryNorm.data();
        io.keyNorm=keyNorm.data();io.rope=rope.data();io.kv=kv.data();io.context=actual.data();
        io.position=position;io.contextLength=contextLength;io.writes=writes.data();
        for (unsigned core=DataflowCoreCount;core-->0;)
            assert(PioneerQwen35DataflowDenseAttentionShardWorker(p,core,scratch->Get(),io));
        assert(io.kvReads==DataflowCoreCount*2*((position+DataflowDenseAttentionTilePositions)/DataflowDenseAttentionTilePositions));
        for (unsigned i=0;i<expected.size();i++)
            assert(std::isfinite(actual[i]) && std::fabs(expected[i]-actual[i])<2e-5f);
        for (unsigned count:writes) assert(count==1);
        assert(!PioneerQwen35DataflowDenseAttentionShardWorker(p,DataflowCoreCount,scratch->Get(),io));
        auto invalidScratch=scratch->Get();invalidScratch.output=nullptr;
        assert(!PioneerQwen35DataflowDenseAttentionShardWorker(p,0,invalidScratch,io));
        io.failWrite=true;
        assert(!PioneerQwen35DataflowDenseAttentionShardWorker(p,0,scratch->Get(),io));
        p.mode=DataflowDenseAttentionPackValue;
        assert(!PioneerQwen35DataflowDenseAttentionShardWorker(p,0,scratch->Get(),io));
    }
    std::puts("PASS 120 dense-attention output shards: exact head coverage and host operator match across 0, 2, 33, 63; SIMULATED ONLY");
}
