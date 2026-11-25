/* SPDX-License-Identifier: MIT */
// Compare the two-stage fleet schedule with the existing full-head worker.
// These host-side mocks are not a runtime or permission to access hardware.
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowDeltaShards.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace PioneerQwen35;
struct Scratch {
    float query[DeltaWidth]{},key[DeltaWidth]{},value[16]{};
    float stateRead[16*DeltaWidth]{},stateWrite[16*DeltaWidth]{};
    float scalars[DataflowDeltaScalarCount]{},head[DeltaWidth]{},norm[DeltaWidth]{},gate[DeltaWidth]{};
    DataflowDeltaShardScratch View() {
        return {query,key,value,stateRead,stateWrite,scalars,head,norm,gate};
    }
};
struct Memory {
    std::vector<float> qkv=std::vector<float>(ConvChannels);
    std::vector<float> gates=std::vector<float>(DeltaInner);
    std::vector<float> oldState=std::vector<float>(DeltaHeads*DeltaStateWords);
    std::vector<float> newState=std::vector<float>(DeltaHeads*DeltaStateWords,-99.0f);
    std::array<float,DeltaHeads> negativeA{},bias{},alpha{},beta{};
    std::array<float,DeltaWidth> norm{};
    Memory() {
        for (unsigned i=0;i<qkv.size();i++) qkv[i]=float(int((i*19)%103)-51)/64.0f;
        for (unsigned i=0;i<gates.size();i++) gates[i]=float(int((i*23)%61)-30)/32.0f;
        for (unsigned i=0;i<oldState.size();i++) oldState[i]=float(int((i*11)%97)-48)/512.0f;
        for (unsigned i=0;i<DeltaHeads;i++) {
            negativeA[i]=-0.25f-float(i%4)/16.0f;bias[i]=float(int(i%7)-3)/16.0f;
            alpha[i]=float(int(i%5)-2)/8.0f;beta[i]=float(int(i%9)-4)/8.0f;
        }
        for (unsigned i=0;i<DeltaWidth;i++) norm[i]=0.5f+float(i%13)/32.0f;
    }
};
struct IO {
    Memory& m;unsigned stateWrites=0,valueWrites=0,binds=0;bool failState=false,failValue=false;
    bool Bind(const DataflowDeltaInvocation& invocation) { ++binds;return invocation.Valid(); }
    bool BindShardProgram(const DataflowDeltaProgram& program) { ++binds;return program.ShapeValid(); }
    bool ReadParameters(float* a,float* b,float* c,float* d) {
        std::copy(m.negativeA.begin(),m.negativeA.end(),a);std::copy(m.bias.begin(),m.bias.end(),b);
        std::copy(m.alpha.begin(),m.alpha.end(),c);std::copy(m.beta.begin(),m.beta.end(),d);return true;
    }
    bool ReadDeltaNorm(float* p) { std::copy(m.norm.begin(),m.norm.end(),p);return true; }
    bool ReadGateHead(unsigned head,float* p) {
        std::copy_n(m.gates.data()+head*DeltaWidth,DeltaWidth,p);return true;
    }
    bool ReadQueryKey(unsigned head,float* q,float* k) {
        const unsigned first=(head%DeltaKeyHeads)*DeltaWidth;
        std::copy_n(m.qkv.data()+first,DeltaWidth,q);
        std::copy_n(m.qkv.data()+DataflowDeltaQChannels+first,DeltaWidth,k);return true;
    }
    bool ReadValuePanel(unsigned row,float* p) {
        std::copy_n(m.qkv.data()+DataflowDeltaVBase+row,16,p);return true;
    }
    bool ReadStatePanel(unsigned head,unsigned panel,float* p) {
        std::copy_n(m.oldState.data()+head*DeltaStateWords+panel*16*DeltaWidth,16*DeltaWidth,p);return true;
    }
    bool WriteStatePanel(unsigned head,unsigned panel,const float* p) {
        if (failState) return false;
        std::copy_n(p,16*DeltaWidth,m.newState.data()+head*DeltaStateWords+panel*16*DeltaWidth);
        ++stateWrites;return true;
    }
    bool WriteValuePanel(unsigned row,const float* p) {
        if (failValue) return false;
        std::copy_n(p,16,m.qkv.data()+DataflowDeltaVBase+row);++valueWrites;return true;
    }
};
static RowSpace Rows(unsigned rows,unsigned bytes,unsigned unit,unsigned address) {
    return {rows,bytes,1,unit,{{0,address,0,rows}}};
}
static TensorSpace Vector(unsigned inputs,unsigned address) {
    TensorSpace result{};result.inputs=inputs;result.place=Rows(1,inputs*4,1,address);return result;
}
static DataflowDeltaProgram Program(unsigned position) {
    DataflowDeltaProgram p{};p.layer=0;p.position=position;
    const unsigned routes[2][Banks]={{786,978,1170,785,977,1169,1361},{786,978,1170,785,977,1169,1361}};
    std::memcpy(p.bankRoute,routes,sizeof(routes));
    p.qkv=Rows(ConvChannels,4,16,0x10000000);p.gate=Rows(DeltaInner,4,16,0x11000000);
    p.alpha=Rows(DeltaHeads,4,16,0x12000000);p.beta=Rows(DeltaHeads,4,16,0x13000000);
    p.negativeA=Vector(DeltaHeads,0x14000000);p.deltaBias=Vector(DeltaHeads,0x15000000);
    p.deltaNorm=Vector(DeltaWidth,0x16000000);
    p.stateRead=Rows(DeltaHeads,DeltaStateWords*4,1,0x20000000);
    p.stateWrite=Rows(DeltaHeads,DeltaStateWords*4,1,0x30000000);
    assert(p.Valid());return p;
}
static void Compare(unsigned position) {
    const auto program=Program(position);
    Memory whole,split;IO oldIO{whole},newIO{split};Scratch scratch;
    std::array<unsigned,DeltaHeads*DataflowDeltaPanelsPerHead> coverage{};
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowDeltaShardAssignment a{};
        assert(DataflowDeltaShardAssign(core,DataflowDeltaShardUpdate,a) && a.panelCount);
        DataflowDeltaShardInvocation shard{};
        assert(DataflowDeltaShardBind(program,core,DataflowDeltaShardUpdate,shard) && shard.Valid());
        auto forged=shard;forged.firstPanel++;
        assert(!forged.Valid());
        for (unsigned p=a.firstPanel;p<a.firstPanel+a.panelCount;p++) ++coverage[a.head*8+p];
        DataflowDeltaInvocation invocation{};
        assert(DataflowDeltaProgramBuilder::BindCore(program,core,invocation));
        assert(PioneerQwen35DataflowDeltaWorker(invocation,scratch.query,scratch.key,scratch.value,
            scratch.stateRead,scratch.stateWrite,scratch.scalars,scratch.head,scratch.norm,scratch.gate,oldIO));
    }
    for (unsigned n:coverage) assert(n==1);
    // The caller must place a fleet barrier between these two loops. Reverse
    // update order also checks that no shard depends on another's writes.
    for (unsigned i=DataflowCoreCount;i>0;i--)
        assert(PioneerQwen35DataflowDeltaShardWorker(program,i-1,DataflowDeltaShardUpdate,scratch.View(),newIO));
    assert(newIO.binds==120 && newIO.stateWrites==256 && newIO.valueWrites==256);
    for (unsigned core=0;core<DataflowCoreCount;core++)
        assert(PioneerQwen35DataflowDeltaShardWorker(program,core,DataflowDeltaShardNormalize,scratch.View(),newIO));
    assert(newIO.binds==152 && newIO.stateWrites==256 && newIO.valueWrites==512);
    assert(std::memcmp(whole.qkv.data(),split.qkv.data(),whole.qkv.size()*sizeof(float))==0);
    assert(std::memcmp(whole.newState.data(),split.newState.data(),whole.newState.size()*sizeof(float))==0);
    Memory failed;IO failure{failed};failure.failState=true;
    assert(!PioneerQwen35DataflowDeltaShardWorker(program,0,DataflowDeltaShardUpdate,scratch.View(),failure));
    assert(!failure.stateWrites && !failure.valueWrites);
    failure.failState=false;failure.failValue=true;
    assert(!PioneerQwen35DataflowDeltaShardWorker(program,0,DataflowDeltaShardUpdate,scratch.View(),failure));
    assert(failure.stateWrites==1 && !failure.valueWrites);
    assert(!PioneerQwen35DataflowDeltaShardWorker(program,120,DataflowDeltaShardUpdate,scratch.View(),failure));
    assert(!PioneerQwen35DataflowDeltaShardWorker(program,0,static_cast<DataflowDeltaShardPhase>(0),scratch.View(),failure));
    auto misaligned=program;misaligned.stateRead.parts[0].address+=4;
    assert(!misaligned.Valid());
    const unsigned binds=failure.binds;
    assert(!PioneerQwen35DataflowDeltaShardWorker(misaligned,0,DataflowDeltaShardUpdate,scratch.View(),failure));
    assert(failure.binds==binds);
}
int main() {
    Compare(0);Compare(1);
    std::puts("PASS 120-worker two-stage delta witness matches full-head worker for fresh and nonzero state; NO HARDWARE");
    return 0;
}
