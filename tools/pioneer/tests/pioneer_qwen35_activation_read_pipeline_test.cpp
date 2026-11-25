/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowActivationRead.h"
#include "../PioneerQwen35DataflowNocReadPipeline.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;

struct Transfer { uint32_t remote,route,local,bytes; };
struct Pipeline {
    std::vector<Transfer> transfers;
    unsigned retired=0,maxPending=0,operations=0,failAt=0;
    bool started=false,finished=false;
    bool Step() { return ++operations!=failAt; }
    bool Start() { if (!Step()) return false;started=true;return true; }
    bool Issue(uint32_t sequence,uint32_t remote,uint32_t route,uint32_t local,unsigned bytes) {
        assert(started && !finished && sequence==transfers.size() && transfers.size()-retired<2);
        if (!Step()) return false;
        transfers.push_back({remote,route,local,bytes});
        const unsigned pending=unsigned(transfers.size())-retired;
        if (pending>maxPending) maxPending=pending;
        return true;
    }
    bool Complete(uint32_t sequence) {
        assert(started && !finished && sequence==retired && retired<transfers.size());
        if (!Step()) return false;
        ++retired;return true;
    }
    bool Finish() { assert(started && retired==transfers.size());if (!Step()) return false;finished=true;return true; }
};
// Compose the actual NoC pipeline with a register-level model. In-flight
// counts are cleared only when the scheduler asks to retire that ID.
struct NocRegisters {
    uint32_t regs[0x300/4]{},counters[8]{},length[2]{};
    unsigned issues=0,maxPending=0,dropAt=0;
    NocRegisters() { regs[0x100/4]=1U<<16;regs[0x64/4]=0x10101010;regs[0x148/4]=0x81; }
    uint32_t Read(unsigned offset) { assert(!(offset&3) && offset<sizeof(regs));return regs[offset/4]; }
    void Write(unsigned offset,uint32_t value) {
        assert(!(offset&3) && offset<sizeof(regs));regs[offset/4]=value;
        if (offset==0x40 && value==1) {
            const unsigned id=(regs[0x18/4]>>10)&15;assert(id<2 && !regs[(0x240+id*4)/4]);
            ++issues;
            if (issues==dropAt) { regs[0x40/4]=0;return; }
            length[id]=regs[0x20/4];regs[(0x240+id*4)/4]=1;
            counters[2]++;counters[6]++;regs[0x40/4]=0;
            const unsigned pending=unsigned(regs[0x240/4]!=0)+unsigned(regs[0x244/4]!=0);
            if (pending>maxPending) maxPending=pending;
        }
    }
    void Fence() {}
    uint32_t Counter(unsigned i) { assert(DataflowNocCounterSupported(i));return counters[i]; }
    bool Quiet() { return !regs[0x40/4] && !regs[0x240/4] && !regs[0x244/4]; }
    void CompleteId(unsigned id) {
        assert(id<2 && regs[(0x240+id*4)/4]);regs[(0x240+id*4)/4]=0;
        for (unsigned i=0;i<8;i++) if (DataflowNocCounterSupported(i) && i!=2 && i!=6)
            counters[i]+=DataflowNocCounterDelta(false,length[id],i);
    }
};
struct RegisterPipeline {
    NocRegisters io;DataflowNocReadPipeline<NocRegisters> reads;
    RegisterPipeline():reads(io) {}
    bool Start() { return reads.Start(); }
    bool Issue(uint32_t s,uint32_t remote,uint32_t route,uint32_t local,unsigned bytes) {
        return reads.Issue(s,remote,route,local,bytes,8);
    }
    bool Complete(uint32_t s) { io.CompleteId(s%2);return reads.Complete(s,8); }
    bool Finish() { return reads.Finish(); }
};
int main() {
    RowSpace source{};source.rows=9216;source.rowBytes=4;source.unit=16;source.count=3;
    source.parts[0]={0,BankBase,0,2048};
    source.parts[1]={2,BankBase+0x20000,2048,4096};
    source.parts[2]={5,BankBase+0x40000,6144,3072};
    unsigned routes[Banks]={2,3,4,5,6,7,8};assert(source.Valid());
    Pipeline p;
    assert(DataflowActivationReadPipelined(source,1024,7168,DataflowActivationCacheAddress,routes,p));
    assert(p.finished && p.maxPending==2 && p.transfers.size()==3);
    assert(p.transfers[0].remote==BankBase+4096 && p.transfers[0].route==2 && p.transfers[0].bytes==4096);
    assert(p.transfers[1].remote==BankBase+0x20000 && p.transfers[1].route==4 && p.transfers[1].bytes==16384);
    assert(p.transfers[2].remote==BankBase+0x40000 && p.transfers[2].route==7 && p.transfers[2].bytes==8192);
    unsigned bytes=0;
    for (const auto& t:p.transfers) { assert(t.local==DataflowActivationCacheAddress+bytes);bytes+=t.bytes; }
    assert(bytes==7168*sizeof(float));
    RegisterPipeline actual;
    assert(DataflowActivationReadPipelined(source,1024,7168,DataflowActivationCacheAddress,routes,actual)
        && actual.io.issues==3 && actual.io.maxPending==2 && !actual.reads.Faulted());
    RegisterPipeline dropped;dropped.io.dropAt=2;
    assert(!DataflowActivationReadPipelined(source,1024,7168,DataflowActivationCacheAddress,routes,dropped)
        && dropped.io.issues==2 && dropped.reads.Faulted() && dropped.reads.Issued()==2);
    RowSpace large{};large.rows=9216;large.rowBytes=4;large.unit=16;large.count=1;
    large.parts[0]={0,BankBase,0,9216};Pipeline split;
    assert(DataflowActivationReadPipelined(large,0,9216,DataflowActivationCacheAddress,routes,split)
        && split.transfers.size()==3 && split.transfers[0].bytes==16384
        && split.transfers[1].bytes==16384 && split.transfers[2].bytes==4096
        && split.transfers[2].remote==BankBase+32768 && split.maxPending==2);
    // Each failed setup, issue, completion or final audit terminates the read;
    // the role must retain its resources rather than replaying an uncertain IO.
    for (unsigned failure=1;failure<=p.operations;failure++) {
        Pipeline failed;failed.failAt=failure;
        assert(!DataflowActivationReadPipelined(source,1024,7168,DataflowActivationCacheAddress,routes,failed));
        assert(failed.operations==failure && !failed.finished);
    }
    auto invalid=source;invalid.parts[1].firstRow++;
    Pipeline noIO;
    assert(!DataflowActivationReadPipelined(invalid,1024,7168,DataflowActivationCacheAddress,routes,noIO)
        && !noIO.operations);
    routes[5]=0;
    assert(!DataflowActivationReadPipelined(source,1024,7168,DataflowActivationCacheAddress,routes,noIO)
        && !noIO.operations);
    routes[5]=7;
    assert(!DataflowActivationReadPipelined(source,1,16,DataflowActivationCacheAddress,routes,noIO)
        && !noIO.operations);
    assert(!DataflowActivationReadPipelined(source,0,9216,DataflowActivationCacheAddress+64,routes,noIO)
        && !noIO.operations);
    std::puts("PASS activation shard pipeline: ordered two-request overlap, exact ranges, failures; SIMULATED ONLY");
}
