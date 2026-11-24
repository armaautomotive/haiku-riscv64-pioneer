/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_ARC_QUEUE_MODEL_ONLY
#include "pioneer_q38_arc_queue_test.cpp"

// Simulated queue 0; no firmware or hardware access. Capture the exact
// argument while the request is published, before the payload is restored.
struct FabricQueue : StrictQueue {
    uint32_t observedMessage=0,observedArgument=0;
    using StrictQueue::StrictQueue;
    uint32_t CsmRead(uint32_t address) {
        io++;Check(false);
        if (notified && address==base && ++polls==3 && !timeout) {
            observedMessage=At(request);observedArgument=At(request+4);
            At(base+16)=At(base);
            At(base+20)=(At(base+4)+1)%8;At(base+24)++;
            At(response)=badReply?0xff:0;
            for (unsigned w=1;w<8;w++) At(response+4*w)=0;
            if (changed) At(base+4)=(At(base+4)+2)%8;
        }
        return At(address);
    }
};
using Queue=PioneerQ38ArcQueue<FabricQueue,false,false,true>;

int main()
{
    const auto arc=FabricQueue::Arc();const auto health=FabricQueue::Health();
    unsigned totalWrites=0,totalTlb=0;
    const uint32_t commands[][2]={{0x33,250},{0xaf,0},{0x20,0},{0x33,0}};
    for (const auto& command:commands) for (unsigned req=0;req<8;req++) for (unsigned resp=0;resp<8;resp++) {
        FabricQueue io(req,resp,0);Queue queue(io);tt_arc_queue_result r{};
        assert(queue.Message(arc,health,r,command[0],command[1]));
        assert(!queue.Unsafe() && r.base==io.base && io.Restored());
        assert(r.submitted && r.responded && r.acknowledged && r.request_restored && r.tlb_restored);
        assert(io.observedMessage==command[0] && io.observedArgument==command[1]);
        totalWrites=io.writes;totalTlb=io.tlbWrites;
    }
    {
        FabricQueue io(7,7,0);Queue queue(io);tt_arc_queue_result r{};
        for (const auto& command:commands) {
            assert(queue.Message(arc,health,r,command[0],command[1]));
            assert(io.observedMessage==command[0] && io.observedArgument==command[1]
                && io.Restored());
        }
        assert(io.notifications==4 && !queue.Unsafe());
    }
    const uint32_t invalid[][2]={{0x33,1},{0x33,251},{0x20,1},{0xaf,250},
        {0x34,0},{0x52,0},{0x54,0},{0x56,3},{0xa0,0},{0x90,0}};
    for (const auto& command:invalid) {
        FabricQueue io(0,0,0);Queue queue(io);tt_arc_queue_result r{};
        assert(!queue.Message(arc,health,r,command[0],command[1]));
        assert(!queue.Unsafe() && !io.io);
    }
    for (unsigned drop=1;drop<=totalWrites;drop++) {
        FabricQueue io(0,0,0);io.dropWrite=drop;Queue queue(io);tt_arc_queue_result r{};
        assert(!queue.Message(arc,health,r,0x33,250) && queue.Unsafe() && r.recovery_required);
        const unsigned before=io.io;const auto saved=r;
        assert(!queue.Message(arc,health,r,0xaf) && io.io==before && !memcmp(&saved,&r,sizeof(r)));
    }
    for (unsigned drop=1;drop<=totalTlb;drop++) {
        FabricQueue io(0,0,0);io.dropTlb=drop;Queue queue(io);tt_arc_queue_result r{};
        const bool okay=queue.Message(arc,health,r,0x20);
        if (io.effectiveDroppedTlb) {
            assert(!okay && queue.Unsafe() && r.recovery_required && io.tlbWrites==drop);
            const unsigned before=io.io;
            assert(!queue.Message(arc,health,r,0x20) && io.io==before);
        } else assert(okay && !queue.Unsafe() && io.Restored());
    }
    for (unsigned fault=0;fault<4;fault++) {
        FabricQueue io(0,0,0);
        if (!fault) io.timeout=true;else if (fault==1) io.badReply=true;
        else if (fault==2) io.changed=true;else io.msiStatus=0;
        Queue queue(io);tt_arc_queue_result r{};
        assert(!queue.Message(arc,health,r,0xaf) && queue.Unsafe() && r.recovery_required);
        const unsigned before=io.io;
        assert(!queue.Message(arc,health,r,0xaf) && io.io==before);
    }
    // Existing application/other recovery queue modes never accept these commands.
    StrictQueue app;PioneerQ38ArcQueue<StrictQueue> appQueue(app);tt_arc_queue_result r{};
    assert(!appQueue.Message(arc,health,r,0x33,250) && !app.io);
    assert(!appQueue.Message(arc,health,r,0xaf) && !app.io);
    puts("PASS Q35 fixed fabric ARC messages, arguments, wraparound and uncertain-failure retention; SIMULATED ONLY");
}
