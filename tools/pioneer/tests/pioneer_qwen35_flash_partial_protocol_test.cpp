/* SPDX-License-Identifier: MIT */
// Fixed L1 partial framing and permission checks; no physical NoC.
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowFlashPartialProtocol.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
struct MockRead {
    const std::array<uint32_t,DataflowFlashPartialBytes/4>& source;
    std::array<uint32_t,16> first{},again{};
    std::array<float,DenseHeadWidth> data{};
    int reads=0,fail=-1,failContext=-1;mutable int contexts=0;bool tear=false,context=true;
    bool ContextMatches(unsigned,const DataflowFlashPartialIdentity&) const {
        return context && contexts++!=failContext;
    }
    bool Read(unsigned,const DataflowFlashPartialIdentity&,unsigned remote,unsigned local,unsigned bytes) {
        if (reads++==fail) return false;
        if (local==DataflowFlashPartialHeaderScratch && bytes==64) {
            for (unsigned i=0;i<16;i++) first[i]=source[i];return true;
        }
        if (local==DataflowFlashPartialIncoming && bytes==DataflowFlashPartialDataBytes) {
            for (unsigned i=0;i<DenseHeadWidth;i++) data[i]=PioneerAttentionFloat(source[16+i]);return true;
        }
        if (local==DataflowFlashPartialHeaderAgain && bytes==64) {
            for (unsigned i=0;i<16;i++) again[i]=source[i];
            if (tear) again[15]^=1;return true;
        }
        (void)remote;return false;
    }
    uint32_t Word(unsigned address,unsigned i) const {
        return address==DataflowFlashPartialHeaderScratch?first[i]:again[i];
    }
    const float* Values() const {return data.data();}
};
int main() {
    unsigned producerStep=0;
    assert(DataflowFlashPartialProducerStep(31,false,producerStep) && producerStep==31);
    assert(DataflowFlashPartialProducerStep(32,true,producerStep) && producerStep==31);
    assert(!DataflowFlashPartialProducerStep(1,true,producerStep) && !producerStep);
    assert(!DataflowFlashPartialProducerStep(0,false,producerStep) && !producerStep);
    alignas(64) std::array<uint32_t,DataflowFlashPartialBytes/4> packet{};
    std::array<float,DenseHeadWidth> values{},output{};
    DataflowFlashPartialIdentity id{5,31,3,33,1,0};
    unsigned first=0,count=0;assert(DataflowFlashPartialIdentityRange(id,first,count));
    for (unsigned i=first;i<first+count;i++) values[i]=float(i+1)*0.0005f;
    DataflowDenseAttentionRow row{values.data(),1.25f,3.5f},result{output.data()};
    DataflowFlashPartialHeader header{};
    assert(DataflowFlashPartialHeaderBuild(id,row,header));
    assert(DataflowFlashPartialPublish(packet.data(),id,row));
    for (unsigned i=0;i<16;i++) assert(packet[i]==header.word[i]);
    for (unsigned i=0;i<DenseHeadWidth;i++) assert(packet[16+i]==PioneerAttentionBits(values[i]));
    assert(DataflowFlashPartialSnapshot(id,header,header,values.data(),result));
    assert(result.maximum==row.maximum && result.denominator==row.denominator && output==values);
    MockRead io{packet};output.fill(0);
    assert(DataflowFlashPartialReceive(0,id,io,result) && io.reads==3 && output==values);
    for (int failure=0;failure<3;failure++) {
        MockRead interrupted{packet};interrupted.fail=failure;output.fill(-1);
        assert(!DataflowFlashPartialReceive(0,id,interrupted,result)
            && interrupted.reads==failure+1 && output[0]==-1);
    }
    MockRead torn{packet};torn.tear=true;output.fill(-1);
    assert(!DataflowFlashPartialReceive(0,id,torn,result) && output[0]==-1);
    MockRead denied{packet};denied.context=false;
    assert(!DataflowFlashPartialReceive(0,id,denied,result) && denied.reads==0);
    for (int failure=0;failure<4;failure++) {
        MockRead changed{packet};changed.failContext=failure;output.fill(-1);
        assert(!DataflowFlashPartialReceive(0,id,changed,result) && output[0]==-1);
    }
    auto self=id;self.producer=0;
    assert(DataflowFlashPartialPublish(packet.data(),self,row));
    MockRead selfRead{packet};output.fill(0);
    assert(DataflowFlashPartialReceive(0,self,selfRead,result) && selfRead.reads==3 && output==values);
    assert(DataflowFlashPartialPublish(packet.data(),id,row));
    for (unsigned i=0;i<16;i++) {
        auto bad=header;bad.word[i]^=1;
        assert(!DataflowFlashPartialSnapshot(id,bad,header,values.data(),result));
        assert(!DataflowFlashPartialSnapshot(id,header,bad,values.data(),result));
    }
    for (unsigned i:{0U,first,first+count-1,DenseHeadWidth-1}) {
        auto corrupt=values;corrupt[i]+=0.25f;
        assert(!DataflowFlashPartialSnapshot(id,header,header,corrupt.data(),result));
    }
    auto stale=id;stale.epoch++;assert(!DataflowFlashPartialSnapshot(stale,header,header,values.data(),result));
    stale=id;stale.step++;assert(!DataflowFlashPartialSnapshot(stale,header,header,values.data(),result));
    stale=id;stale.position++;assert(!DataflowFlashPartialSnapshot(stale,header,header,values.data(),result));
    stale=id;stale.layer=0;assert(!DataflowFlashPartialSnapshot(stale,header,header,values.data(),result));
    row.denominator=0;assert(!DataflowFlashPartialHeaderBuild(id,row,header));row.denominator=3.5f;
    if (first+count<DenseHeadWidth) {
        values[first+count]=1;assert(!DataflowFlashPartialHeaderBuild(id,row,header));values[first+count]=0;
    }
    unsigned permitted=0;
    for (unsigned position:{0U,1U,2U,4U,7U,14U,29U,33U,127U,MaxPositions-1}) {
        for (unsigned consumer=0;consumer<DataflowCoreCount;consumer++) {
            DataflowFlashHybridPartition c{};assert(DataflowFlashHybridAssign(consumer,position,c));
            for (unsigned producer=0;producer<DataflowCoreCount;producer++) {
                DataflowFlashHybridPartition p{};assert(DataflowFlashHybridAssign(producer,position,p));
                for (unsigned q=0;q<DataflowFlashQueriesPerKv;q++) {
                    DataflowFlashPartialIdentity probe{5,31,3,position,producer,q};
                    unsigned lane=0,lanes=0;const bool present=DataflowFlashPartialIdentityRange(probe,lane,lanes);
                    const unsigned base=DataflowFlashPartialBase+q*DataflowFlashPartialBytes;
                    const bool expected=present && consumer!=producer && !c.positionShard
                        && c.kvHead==p.kvHead && c.outputShard==p.outputShard;
                    const unsigned route=DataflowBarrierWorkerRoute(producer);
                    const bool allowed=DataflowFlashPartialPeerReadAllowed(consumer,probe,route,base,
                        DataflowFlashPartialHeaderScratch,DataflowFlashPartialHeaderBytes);
                    assert(allowed==expected);
                    assert(DataflowFlashPartialLocalReadAllowed(consumer,probe,base,
                        DataflowFlashPartialHeaderScratch,DataflowFlashPartialHeaderBytes)
                        ==(present && consumer==producer && !c.positionShard));
                    if (expected) {
                        permitted++;
                        assert(DataflowFlashPartialPeerReadAllowed(consumer,probe,route,base,
                            DataflowFlashPartialHeaderAgain,DataflowFlashPartialHeaderBytes));
                        assert(DataflowFlashPartialPeerReadAllowed(consumer,probe,route,base+64,
                            DataflowFlashPartialIncoming,DataflowFlashPartialDataBytes));
                        assert(!DataflowFlashPartialPeerReadAllowed(consumer,probe,route^1,base,
                            DataflowFlashPartialHeaderScratch,DataflowFlashPartialHeaderBytes));
                        assert(!DataflowFlashPartialPeerReadAllowed(consumer,probe,route,base+64,
                            DataflowFlashPartialIncoming+64,DataflowFlashPartialDataBytes));
                        assert(!DataflowFlashPartialPeerReadAllowed(consumer,probe,route,base+64,
                            DataflowFlashPartialIncoming,DataflowFlashPartialDataBytes-64));
                    }
                }
            }
        }
    }
    assert(permitted>0);
    std::printf("PASS partial framing and %u permitted fixed peer routes; SIMULATED ONLY\n",permitted);
}
