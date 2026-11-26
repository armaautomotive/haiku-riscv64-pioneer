/* SPDX-License-Identifier: MIT */
// Compare temporal GQA partials with the existing online attention row.
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowDenseAttentionPartial.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
using namespace PioneerQwen35;
struct Partial {
    std::array<float,DenseHeadWidth> sums{};
    float maximum=0,denominator=0;
    DataflowDenseAttentionRow Row() {return {sums.data(),maximum,denominator};}
    void Store(const DataflowDenseAttentionRow& row) {maximum=row.maximum;denominator=row.denominator;}
};
static float Sample(unsigned index,unsigned salt) {
    return float(int((index*17+salt*23)%193)-96)*0.003f;
}
int main() {
    for (unsigned position:{0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,13U,14U,15U,16U,
        27U,28U,29U,30U,31U,32U,33U,63U,127U,MaxPositions-1}) {
        unsigned coverage[DenseKeyHeads]={},active=0;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            DataflowFlashPartition part{};assert(DataflowFlashAssign(core,position,part));
            assert(part.kvHead==core/DataflowFlashWorkersPerKv && part.firstPosition==coverage[part.kvHead]);
            coverage[part.kvHead]+=part.positionCount;active+=part.positionCount!=0;
        }
        for (unsigned count:coverage) assert(count==position+1);
        assert(active==DenseKeyHeads*((position+1)<DataflowFlashWorkersPerKv?position+1:DataflowFlashWorkersPerKv));
        bool blocks[DenseKeyHeads][DataflowFlashWorkersPerKv][DataflowFlashBlocksPerKv]{};
        unsigned positionsSeen[DenseKeyHeads][DataflowFlashWorkersPerKv]{},hybridReads=0;
        DataflowFlashHybridPartition last{};
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            DataflowFlashHybridPartition part{};assert(DataflowFlashHybridAssign(core,position,part));
            assert(part.positionCount && part.blockCount);
            if (!part.outputShard) {
                positionsSeen[part.kvHead][part.positionShard]=part.positionCount;
            }
            for (unsigned block=part.firstBlock;block<part.firstBlock+part.blockCount;block++) {
                assert(!blocks[part.kvHead][part.positionShard][block]);
                blocks[part.kvHead][part.positionShard][block]=true;
            }
            hybridReads+=part.positionCount;last=part;
        }
        for (unsigned kv=0;kv<DenseKeyHeads;kv++) {
            unsigned nextPosition=0;
            for (unsigned shard=0;shard<last.positionShards;shard++) {
                DataflowFlashHybridPartition part{};
                assert(DataflowFlashHybridAssign(kv*DataflowFlashWorkersPerKv+shard*last.outputShards,position,part));
                assert(part.firstPosition==nextPosition && positionsSeen[kv][shard]==part.positionCount);
                nextPosition+=part.positionCount;
                for (bool covered:blocks[kv][shard]) assert(covered);
            }
            assert(nextPosition==position+1);
        }
        assert(hybridReads==DenseKeyHeads*(position+1)*last.outputShards);
    }
    DataflowFlashPartition bad{};
    assert(!DataflowFlashAssign(DataflowCoreCount,0,bad) && !DataflowFlashAssign(0,MaxPositions,bad));
    DataflowFlashHybridPartition badHybrid{};
    assert(!DataflowFlashHybridAssign(DataflowCoreCount,0,badHybrid)
        && !DataflowFlashHybridAssign(0,MaxPositions,badHybrid));
    unsigned invalidFirst=0,invalidCount=0;
    assert(!DataflowFlashHybridRange(badHybrid,0,invalidFirst,invalidCount));
    constexpr unsigned positions=128,words=DenseHeadWidth/2;
    std::vector<uint32_t> keys(DenseKeyHeads*positions*words),values(keys.size());
    std::array<std::array<float,DenseHeadWidth>,DenseHeads> queries{},gates{};
    for (unsigned head=0;head<DenseHeads;head++) for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
        queries[head][lane]=Sample(lane,head+1)*4;gates[head][lane]=Sample(lane,head+11);
    }
    for (unsigned head=0;head<DenseKeyHeads;head++) for (unsigned pos=0;pos<positions;pos++)
        for (unsigned pair=0;pair<words;pair++) {
            const unsigned index=(head*positions+pos)*words+pair;
            keys[index]=PioneerAttentionToHalf(Sample(pair*2,head+pos+3)*3)
                | (unsigned(PioneerAttentionToHalf(Sample(pair*2+1,head+pos+3)*3))<<16);
            values[index]=PioneerAttentionToHalf(Sample(pair*2,head+pos+13)*3)
                | (unsigned(PioneerAttentionToHalf(Sample(pair*2+1,head+pos+13)*3))<<16);
        }
    float maxError=0,maxHybridError=0;double maxIndependentError=0;
    for (unsigned position:{0U,1U,2U,28U,29U,30U,31U,32U,33U,63U,127U}) {
        std::array<Partial,DenseHeads> expected{};
        for (unsigned head=0;head<DenseHeads;head++) {
            auto row=expected[head].Row();const unsigned kv=head/DataflowFlashQueriesPerKv;
            for (unsigned pos=0;pos<=position;pos++) {
                const unsigned index=(kv*positions+pos)*words;
                assert(row.Add(queries[head].data(),keys.data()+index,values.data()+index));
            }
            expected[head].Store(row);assert(row.Finish(gates[head].data()));
        }
        std::vector<Partial> partials(DataflowCoreCount*DataflowFlashQueriesPerKv);
        unsigned tileReads=0;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            DataflowFlashPartition part{};assert(DataflowFlashAssign(core,position,part));
            DataflowDenseAttentionRow rows[4]={partials[core*4].Row(),partials[core*4+1].Row(),
                partials[core*4+2].Row(),partials[core*4+3].Row()};
            for (unsigned pos=part.firstPosition;pos<part.firstPosition+part.positionCount;pos++) {
                const unsigned index=(part.kvHead*positions+pos)*words;++tileReads;
                for (unsigned q=0;q<4;q++) {
                    const unsigned head=part.kvHead*4+q;
                    assert(rows[q].Add(queries[head].data(),keys.data()+index,values.data()+index));
                }
            }
            for (unsigned q=0;q<4;q++) partials[core*4+q].Store(rows[q]);
        }
        assert(tileReads==DenseKeyHeads*(position+1));
        // Exercise the hybrid schedule with real softmax partials. Each
        // worker reads its temporal KV range once and updates only its
        // assigned aligned slices, including slices crossing head boundaries.
        std::vector<Partial> hybrid(DataflowCoreCount*DataflowFlashQueriesPerKv);
        unsigned hybridReads=0;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            DataflowFlashHybridPartition part{};assert(DataflowFlashHybridAssign(core,position,part));
            DataflowDenseAttentionRow rows[4]={hybrid[core*4].Row(),hybrid[core*4+1].Row(),
                hybrid[core*4+2].Row(),hybrid[core*4+3].Row()};
            for (unsigned pos=part.firstPosition;pos<part.firstPosition+part.positionCount;pos++) {
                const unsigned index=(part.kvHead*positions+pos)*words;++hybridReads;
                for (unsigned q=0;q<4;q++) {
                    unsigned first=0,count=0;assert(DataflowFlashHybridRange(part,q,first,count));
                    if (count) assert(rows[q].AddSlice(queries[part.kvHead*4+q].data(),
                        keys.data()+index,values.data()+index,first,count));
                }
            }
            for (unsigned q=0;q<4;q++) hybrid[core*4+q].Store(rows[q]);
        }
        DataflowFlashHybridPartition layout{};assert(DataflowFlashHybridAssign(0,position,layout));
        assert(hybridReads==DenseKeyHeads*(position+1)*layout.outputShards);
        for (unsigned kv=0;kv<DenseKeyHeads;kv++) for (unsigned slice=0;slice<layout.outputShards;slice++) {
            DataflowFlashHybridPartition part{};
            assert(DataflowFlashHybridAssign(kv*DataflowFlashWorkersPerKv+slice,position,part));
            for (unsigned q=0;q<4;q++) {
                unsigned firstLane=0,count=0;assert(DataflowFlashHybridRange(part,q,firstLane,count));
                if (!count) continue;
                Partial merged{};auto row=merged.Row();
                for (unsigned shard=0;shard<layout.positionShards;shard++) {
                    const unsigned core=kv*DataflowFlashWorkersPerKv+shard*layout.outputShards+slice;
                    auto src=hybrid[core*4+q].Row();assert(DataflowFlashMerge(row,src));
                }
                const unsigned head=kv*4+q;
                assert(row.FinishSlice(gates[head].data(),firstLane,count));
                for (unsigned lane=firstLane;lane<firstLane+count;lane++) {
                    const float error=std::fabs(merged.sums[lane]-expected[head].sums[lane]);
                    if (error>maxHybridError) maxHybridError=error;
                    assert(std::isfinite(merged.sums[lane]) && error<2e-5f);
                }
            }
        }
        std::array<std::array<double,DenseHeadWidth>,DenseHeads> independent{};
        if (position==0 || position==33 || position==127) {
            for (unsigned head=0;head<DenseHeads;head++) {
                const unsigned kv=head/4;std::vector<double> scores(position+1);double maximum=-1e300;
                for (unsigned pos=0;pos<=position;pos++) {
                    const unsigned index=(kv*positions+pos)*words;double dot=0;
                    for (unsigned lane=0;lane<DenseHeadWidth;lane++)
                        dot+=double(queries[head][lane])*PioneerAttentionHalf(uint16_t(keys[index+lane/2]>>((lane&1)*16)));
                    scores[pos]=dot*0.0625;if (scores[pos]>maximum) maximum=scores[pos];
                }
                double denominator=0;
                for (unsigned pos=0;pos<=position;pos++) {
                    const unsigned index=(kv*positions+pos)*words;const double weight=std::exp(scores[pos]-maximum);
                    denominator+=weight;
                    for (unsigned lane=0;lane<DenseHeadWidth;lane++)
                        independent[head][lane]+=weight*PioneerAttentionHalf(uint16_t(values[index+lane/2]>>((lane&1)*16)));
                }
                for (unsigned lane=0;lane<DenseHeadWidth;lane++)
                    independent[head][lane]/=denominator*(1+std::exp(-double(gates[head][lane])));
            }
        }
        for (unsigned order=0;order<2;order++) for (unsigned head=0;head<DenseHeads;head++) {
            Partial merged{};auto row=merged.Row();const unsigned kv=head/4,q=head%4;
            for (unsigned rank=0;rank<DataflowFlashWorkersPerKv;rank++) {
                const unsigned ordered=order?DataflowFlashWorkersPerKv-1-rank:rank;
                auto part=partials[(kv*DataflowFlashWorkersPerKv+ordered)*4+q].Row();
                assert(DataflowFlashMerge(row,part));
            }
            assert(row.Finish(gates[head].data()));
            for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
                const float error=std::fabs(merged.sums[lane]-expected[head].sums[lane]);
                if (error>maxError) maxError=error;
                assert(std::isfinite(merged.sums[lane]) && error<2e-5f);
                if (position==0 || position==33 || position==127) {
                    const double independentError=std::fabs(double(merged.sums[lane])-independent[head][lane]);
                    if (independentError>maxIndependentError) maxIndependentError=independentError;
                    assert(independentError<2e-4);
                }
            }
        }
    }
    Partial a{},b{};auto dst=a.Row(),src=b.Row();
    assert(DataflowFlashMerge(dst,src));
    assert(!DataflowFlashMerge(dst,dst));
    auto overlap=dst;overlap.sums+=1;assert(!DataflowFlashMerge(dst,overlap));
    src.denominator=-1;assert(!DataflowFlashMerge(dst,src));
    src.denominator=1;src.maximum=std::numeric_limits<float>::quiet_NaN();assert(!DataflowFlashMerge(dst,src));
    src.maximum=0;src.sums[3]=std::numeric_limits<float>::quiet_NaN();assert(!DataflowFlashMerge(dst,src));
    std::printf("PASS GQA temporal and hybrid partitions; max sequential error %.9g, hybrid error %.9g, double-reference error %.9g; SIMULATED ONLY\n",maxError,maxHybridError,maxIndependentError);
}
