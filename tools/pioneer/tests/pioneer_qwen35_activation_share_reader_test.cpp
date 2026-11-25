/* SPDX-License-Identifier: MIT */
// Actual reader control flow with simulated peer transfers and queue consumer.
// The host quantizer and test fixture do not enter the Work inference path.
#include "../PioneerQwen35DataflowProjectionQualification.h"
#include "../PioneerQwen35DataflowSharedActivationRoles.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

using namespace PioneerQwen35;
using Fixture=DataflowProjectionQualification;
struct Fabric {
    DataflowActivationSharePacket packet{};
    std::vector<uint8_t> cache=std::vector<uint8_t>(DataflowActivationCacheBytes);
    bool ready=false;
};
struct Reader {
    Fabric& fabric;uint8_t* cache;const DataflowCoreInvocation* program=nullptr;
    bool failReceive=false,failPublish=false;
    unsigned reads=0,packs=0,receives=0,publishes=0;
    bool Bind(const DataflowCoreInvocation& p) { program=&p;return p.Valid(); }
    bool ReadActivation(const RowSpace&,unsigned rows,uint8_t* out,unsigned bytes) {
        ++reads;return rows==Fixture::Inputs && out==cache && bytes==Fixture::InputBytes
            && Fixture::FillInputBytes(0,out,bytes);
    }
    bool PackActivation(unsigned type,unsigned inputs,unsigned chunk,const float* source,uint8_t* out,unsigned bytes) {
        ++packs;return bytes==sizeof(PioneerQ38ProjectionInput) && inputs==Fixture::Inputs
            && PioneerQ38ProjectionQuantize(type,source+chunk*256,256,
                reinterpret_cast<PioneerQ38ProjectionInput*>(out));
    }
    bool ReceiveSharedActivation(const DataflowCoreInvocation& p,const DataflowActivationReuse& reuse,uint8_t* out) {
        ++receives;DataflowActivationShareTreePosition position{};
        if (failReceive || !fabric.ready || out!=cache
            || !DataflowActivationShareTreeLocate(p.sharing,p.core,position)
            || !DataflowActivationSharePacketMatches(fabric.packet,p,reuse.epoch,reuse.step,position.parent)) return false;
        std::memcpy(out,fabric.cache.data(),fabric.cache.size());return true;
    }
    bool PublishSharedActivation(const DataflowCoreInvocation& p,const DataflowActivationReuse& reuse) {
        ++publishes;
        if (failPublish || !DataflowActivationSharePacketBuild(p,reuse.epoch,reuse.step,p.core,fabric.packet)) return false;
        std::memcpy(fabric.cache.data(),cache,fabric.cache.size());fabric.ready=true;return true;
    }
    struct WeightPipeline {
        unsigned issued=0,retired=0;
        explicit WeightPipeline(Reader&) {}
        bool Start() { return true; }
        bool Issue(uint32_t sequence,unsigned,uint32_t,uint8_t*,unsigned) {
            if (sequence!=issued || issued-retired>=2) return false;
            ++issued;return true;
        }
        bool Complete(uint32_t sequence) { if (sequence!=retired || retired>=issued) return false;++retired;return true; }
        bool Finish() { return issued==retired; }
        uint32_t Issued() const { return issued; }
        uint32_t Retired() const { return retired; }
    };
};
struct Worker {
    DataflowCoreInvocation program{};
    DataflowActivationReuse reuse{};
    DataflowQueueControl controls[3]{};
    std::vector<uint32_t> local=std::vector<uint32_t>((DataflowLocalBufferEnd-DataflowWeightBufferAddress)/4);
    std::vector<uint32_t> cache=std::vector<uint32_t>(DataflowActivationCacheBytes/4);
    DataflowQueueView queues[3]{};
    Reader reader;
    explicit Worker(Fabric& fabric):reader{fabric,reinterpret_cast<uint8_t*>(cache.data())} {}
    bool Run(unsigned epoch,unsigned step,bool expectSuccess) {
        for (auto& control:controls) control={};
        assert(DataflowBindQueues(program,controls,reinterpret_cast<uint8_t*>(local.data()),queues));
        DataflowActivationReuseSetStep(reuse,epoch,step);
        std::atomic<bool> done{false};
        std::thread drain([&] {
            for (;;) {
                const unsigned consumed=DataflowLoadAcquire(controls[0].consumed);
                const unsigned produced=DataflowLoadAcquire(controls[0].produced);
                if (consumed<produced) { assert(DataflowQueuePopFront(queues[0],consumed));continue; }
                if (done.load()) break;
                std::this_thread::yield();
            }
        });
        const bool okay=PioneerQwen35SharedActivationReader(program,queues[0],queues[1],
            reinterpret_cast<uint8_t*>(cache.data()),reader,&reuse);
        done.store(true);drain.join();
        assert(okay==expectSuccess);
        if (okay) assert(DataflowLoadAcquire(controls[1].produced)==1 && !DataflowQueueFailure(controls[0]));
        else assert(DataflowQueueFailure(controls[0]) && DataflowQueueFailure(controls[1])
            && !DataflowLoadAcquire(controls[1].produced));
        return okay;
    }
};
struct PeerIO {
    const DataflowCoreInvocation& program;
    DataflowActivationSharePacket ready{},scratch{};
    std::vector<uint8_t> source=std::vector<uint8_t>(DataflowActivationCacheBytes);
    std::vector<uint8_t> destination=std::vector<uint8_t>(DataflowActivationCacheBytes);
    unsigned reads=0,stale=0,failAt=0,loseContextAfter=UINT32_MAX;
    bool ContextMatches(unsigned epoch,unsigned step) const {
        return epoch==3 && step==4 && reads<loseContextAfter;
    }
    bool Read(unsigned parent,unsigned remote,unsigned local,unsigned bytes) {
        ++reads;
        assert(DataflowActivationSharePeerReadAllowed(program,parent,DataflowBarrierWorkerRoute(parent),remote,local,bytes));
        if (reads==failAt) return false;
        if (remote==DataflowActivationShareReadyAddress) {
            scratch=ready;if (stale) { --stale;scratch.word[2]=3; }
        } else {
            const unsigned offset=remote-DataflowActivationCacheAddress;
            assert(offset+bytes<=source.size());
            std::memcpy(destination.data()+offset,source.data()+offset,bytes);
        }
        return true;
    }
    uint32_t ScratchWord(unsigned index) const { assert(index<16);return scratch.word[index]; }
};

int main()
{
    unsigned routes[2][Banks]{};
    for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=b+1;
    auto program=std::make_unique<DataflowProjectionProgram>();
    auto schedule=std::make_unique<PioneerQ35ProjectionProgram>();
    assert(Fixture::Build(0,BankBase,Fixture::ScratchBytes,routes,*program,*schedule));
    Fabric fabric;Worker leader(fabric),follower(fabric),failure(fabric);
    Worker* workers[]={&leader,&follower,&failure};
    for (unsigned core=0;core<3;core++) {
        auto& p=workers[core]->program;
        assert(DataflowProgramBuilder::BindCore(*program,core,p));
        for (unsigned i=0;i<DataflowCoreCount;i++) p.sharing.members[i/32]|=uint32_t(1)<<(i%32);
        p.sharing.leader=0;p.sharing.packing=12;p.sharing.inputKey=DataflowActivationShareInputKey(p);
        assert(DataflowCoreHeaderValid(p));
    }
    PeerIO peer{follower.program};
    assert(DataflowActivationSharePacketBuild(leader.program,3,4,0,peer.ready));
    for (unsigned i=0;i<peer.source.size();i++) peer.source[i]=uint8_t(i*17U);
    peer.stale=2;
    assert(DataflowActivationShareReceive(follower.program,3,4,peer,4) && peer.reads==5);
    const unsigned packedBytes=(Fixture::Inputs/256)*DataflowInputPageBytes;
    const unsigned sumsBytes=(Fixture::Inputs/256)*DataflowActivationSumsPerChunk*sizeof(int32_t);
    assert(!std::memcmp(peer.source.data(),peer.destination.data(),packedBytes));
    assert(!std::memcmp(peer.source.data()+DataflowActivationSumsOffset,
        peer.destination.data()+DataflowActivationSumsOffset,sumsBytes));
    for (unsigned fail=1;fail<=3;fail++) {
        PeerIO error{follower.program};error.ready=peer.ready;error.failAt=fail;
        assert(!DataflowActivationShareReceive(follower.program,3,4,error,1) && error.reads==fail);
    }
    PeerIO timeout{follower.program};timeout.ready=peer.ready;timeout.stale=2;
    assert(!DataflowActivationShareReceive(follower.program,3,4,timeout,2) && timeout.reads==2);
    PeerIO changed{follower.program};changed.ready=peer.ready;changed.loseContextAfter=1;
    assert(!DataflowActivationShareReceive(follower.program,3,4,changed,2) && changed.reads==1);
    PeerIO root{leader.program};assert(!DataflowActivationShareReceive(leader.program,3,4,root,1) && !root.reads);
    // Max-width synthetic input spans two bounded packed-data transfers.
    auto wide=follower.program;
    wide.inputs=wide.inputRows=PioneerQwen35Projection::MaxInputs;
    wide.input.rows=wide.input.parts[0].rows=wide.inputRows;
    wide.sharing.inputKey=DataflowActivationShareInputKey(wide);
    PeerIO widePeer{wide};
    assert(DataflowActivationSharePacketBuild(wide,3,4,0,widePeer.ready));
    for (unsigned i=0;i<widePeer.source.size();i++) widePeer.source[i]=uint8_t(i*31U);
    assert(DataflowActivationShareReceive(wide,3,4,widePeer,1) && widePeer.reads==4);
    const unsigned widePacked=(wide.inputs/256)*DataflowInputPageBytes;
    const unsigned wideSums=(wide.inputs/256)*DataflowActivationSumsPerChunk*sizeof(int32_t);
    assert(!std::memcmp(widePeer.source.data(),widePeer.destination.data(),widePacked));
    assert(!std::memcmp(widePeer.source.data()+DataflowActivationSumsOffset,
        widePeer.destination.data()+DataflowActivationSumsOffset,wideSums));
    assert(leader.Run(1,1,true));
    assert(leader.reader.reads==1 && leader.reader.packs==Fixture::Blocks && !leader.reader.receives);
    const auto packed=fabric.cache;
    assert(follower.Run(1,1,true));
    assert(!follower.reader.reads && !follower.reader.packs && follower.reader.receives==1
        && std::memcmp(follower.cache.data(),packed.data(),packed.size())==0);
    // A valid next-step local cache can be published without another peer read.
    assert(follower.Run(1,2,true));
    assert(follower.reader.receives==1 && follower.reader.publishes==2 && !follower.reader.reads);
    // A failed receive or publication leaves both queues faulted and unready.
    failure.reader.failReceive=true;
    assert(!failure.Run(2,1,false) && failure.reader.receives==1 && !failure.reader.publishes);
    failure.reader.failReceive=false;failure.reader.failPublish=true;fabric.ready=false;
    assert(leader.Run(2,1,true));
    assert(!failure.Run(2,1,false) && failure.reader.publishes==1);
    std::puts("PASS actual shared reader selects leader, peer, reuse and retained failures; SIMULATED ONLY");
}
