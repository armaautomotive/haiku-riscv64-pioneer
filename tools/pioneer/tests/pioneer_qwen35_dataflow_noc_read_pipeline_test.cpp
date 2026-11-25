/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowNocReadPipeline.h"
#include <cassert>
#include <cstdint>
#include <cstring>
using namespace PioneerQwen35;

struct MockNoc {
    uint32_t regs[0x300/4]{};
    uint32_t counters[8]{};
    unsigned issues=0,fences=0;
    bool failReadback=false,stuckDoorbell=false,dropDoorbell=false,completeImmediately=false,extraResponse=false,quiet=true;
    MockNoc() { regs[0x100/4]=1U<<16;regs[0x64/4]=0x10101010;regs[0x148/4]=0x81; }
    uint32_t Read(unsigned off) {
        assert(!(off&3) && off<sizeof(regs));
        if (failReadback && off==4) return regs[off/4]^1;
        return regs[off/4];
    }
    void Write(unsigned off,uint32_t value) {
        assert(!(off&3) && off<sizeof(regs));
        regs[off/4]=value;
        if (off==0x40 && value==1) {
            if (dropDoorbell) { regs[off/4]=0;return; }
            const unsigned id=(regs[0x18/4]>>10)&15;
            assert(id<2 && !regs[(0x240+id*4)/4]);
            regs[(0x240+id*4)/4]=1;issues++;
            if (!stuckDoorbell) { counters[2]++;counters[6]++;regs[off/4]=0; }
            if (completeImmediately) Complete(id);
            if (extraResponse) counters[0]+=2;
        }
    }
    void Fence() { fences++; }
    uint32_t Counter(unsigned i) { assert(DataflowNocCounterSupported(i));return counters[i]; }
    bool Quiet() {
        return quiet && !regs[0x40/4] && !regs[0x240/4] && !regs[0x244/4]
            && regs[0x64/4]==0x10101010 && !regs[0x68/4];
    }
    void Complete(unsigned id) {
        assert(id<2 && regs[(0x240+id*4)/4]==1);
        regs[(0x240+id*4)/4]=0;
        for (unsigned i=0;i<8;i++) if (DataflowNocCounterSupported(i) && i!=2 && i!=6)
            counters[i]+=DataflowNocCounterDelta(false,64,i);
    }
};

int main() {
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start());
        assert(p.Issue(0,0x8000000,0x82,0x90000,2304,4));
        assert(p.Issue(1,0x8000900,0x82,0x98000,2304,4));
        assert(io.issues==2 && p.Issued()==2 && p.Retired()==0);
        // The second ID may finish first, but the queue is published in order.
        io.Complete(1);io.Complete(0);
        assert(p.Complete(0,4) && p.Complete(1,4));
        assert(p.Issue(2,0x8001200,0x82,0x90000,2304,4));
        io.Complete(0);assert(p.Complete(2,4) && p.Finish() && !p.Faulted());
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,4));
        assert(!p.Complete(1,1) && p.Faulted());
        io.Complete(0);assert(!p.Complete(0,1) && !p.Finish());
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,4));
        io.regs[0x64/4]=0x1010100f; // FIFO entry hides a possible request.
        io.Complete(0);assert(!p.Complete(0,2) && p.Faulted());
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start());io.stuckDoorbell=true;
        assert(!p.Issue(0,0x8000000,0x82,0x90000,64,2));
        assert(p.Faulted() && p.Issued()==1 && io.issues==1);
        assert(!p.Issue(0,0x8000000,0x82,0x90000,64,2));
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start());io.failReadback=true;
        assert(!p.Issue(0,0x8000000,0x82,0x90000,64,2));
        assert(p.Faulted() && !io.issues);
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,2));
        io.Complete(0);assert(p.Complete(0,2));io.counters[0]++;
        assert(!p.Finish() && p.Faulted());
    }
    {
        MockNoc io;io.regs[0x100/4]=0; // FIFO disabled is also supported.
        DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,2));
        io.Complete(0);assert(p.Complete(0,2) && p.Finish());
    }
    {
        MockNoc io;
        for (unsigned i=0;i<8;i++) io.counters[i]=UINT32_MAX;
        DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,2));
        io.Complete(0);assert(p.Complete(0,2) && p.Finish());
        assert(!p.Start() && p.Faulted()); // ownership is one-shot
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,2)
            && p.Issue(1,0x8000040,0x82,0x98000,64,2));
        assert(!p.Issue(2,0x8000080,0x82,0x90000,64,2));
        assert(p.Faulted() && io.issues==2); // no in-flight page reuse
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start());io.regs[0x68/4]=1;
        assert(!p.Issue(0,0x8000000,0x82,0x90000,64,2));
        assert(p.Faulted() && io.issues==0);
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start());io.dropDoorbell=true;
        assert(!p.Issue(0,0x8000000,0x82,0x90000,64,2));
        assert(p.Faulted() && p.Issued()==1 && !io.issues);
        assert(!p.Complete(0,2)); // a dropped request never becomes a ready page
    }
    {
        MockNoc io;io.completeImmediately=true;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,2));
        assert(p.Complete(0,2) && p.Finish() && io.counters[0]==1);
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,2));
        io.Complete(0);io.dropDoorbell=true;
        assert(!p.Issue(1,0x8000040,0x82,0x98000,64,2));
        assert(p.Faulted() && io.issues==1); // previous response cannot prove this submission
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start());io.extraResponse=true;
        assert(!p.Issue(0,0x8000000,0x82,0x90000,64,2));
        assert(p.Faulted() && io.issues==1); // impossible response count poisons the pipeline
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start());
        for (unsigned sequence=0;sequence<512;sequence+=2) {
            assert(p.Issue(sequence,0x8000000+sequence*64,0x82,0x90000,64,2));
            assert(p.Issue(sequence+1,0x8000040+sequence*64,0x82,0x98000,64,2));
            if (sequence&2) { io.Complete(1);io.Complete(0); }
            else { io.Complete(0);io.Complete(1); }
            assert(p.Complete(sequence,2) && p.Complete(sequence+1,2));
        }
        assert(p.Finish() && io.issues==512 && !p.Faulted());
    }
    {
        MockNoc io;DataflowNocReadPipeline<MockNoc> p(io);
        assert(p.Start() && p.Issue(0,0x8000000,0x82,0x90000,64,2));
        io.failReadback=true;
        assert(!p.Issue(1,0x8000040,0x82,0x98000,64,2));
        assert(p.Faulted() && p.Issued()==1 && io.regs[0x240/4]==1);
        assert(!p.Complete(0,2) && !p.Finish()); // first page remains retained
    }
    return 0;
}
