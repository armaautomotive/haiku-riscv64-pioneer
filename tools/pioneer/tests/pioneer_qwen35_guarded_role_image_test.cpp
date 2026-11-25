/* SPDX-License-Identifier: MIT */
// The actual role views and mapper over simulated MMIO. Fail the retained
// recovery ticket on every pre/post word boundary, including within a chunk.
#define main qwen35_original_role_views_test_main
#include "pioneer_qwen35_dataflow_role_writable_views_test.cpp"
#undef main

struct ImageTicket : Guard {
    Platform& platform;
    unsigned calls=0,failAt=0,readsAtFailure=0;
    bool lost=false;
    explicit ImageTicket(Platform& p):platform(p) {}
    bool RecoveryWordAccessAllowed() {
        if (lost) return false;
        if (++calls==failAt) { lost=true;readsAtFailure=platform.reads;return false; }
        return true;
    }
};
int main() {
    Reset();Platform platform;ImageTicket guard(platform);platform.guard=&guard;
    DataflowRoleWritableViews<Platform> views(platform);
    assert(views.Open(BarStart,BarEnd,guard));
    constexpr unsigned count=130;
    uint32_t expected[count],output[count];
    for (unsigned i=0;i<count;i++)
        memory[DataflowRolePhysicalExtraCode][i]=expected[i]=0x12000000U+i;
    for (unsigned mode=0;mode<2;mode++) {
        for (unsigned fail=1;fail<=count*2;fail++) {
            guard.calls=0;guard.failAt=fail;guard.lost=false;platform.reads=0;
            for (auto& word:output) word=UINT32_MAX;
            const bool okay=mode==0?views.Matches(3,DataflowRolePhysicalExtraCode,0,expected,count,guard)
                :views.CopyWords(3,DataflowRolePhysicalExtraCode,0,output,count,guard);
            assert(!okay && guard.lost && guard.calls==fail && platform.reads==guard.readsAtFailure
                && platform.reads==fail/2);
            if (mode==1) for (unsigned i=0;i<count;i++)
                assert(output[i]==(i<(fail-1)/2?expected[i]:UINT32_MAX));
        }
        guard.calls=guard.failAt=0;guard.lost=false;platform.reads=0;
        assert((mode==0?views.Matches(3,DataflowRolePhysicalExtraCode,0,expected,count,guard)
            :views.CopyWords(3,DataflowRolePhysicalExtraCode,0,output,count,guard))
            && guard.calls==count*2 && platform.reads==count);
    }
    assert(views.Close());
    std::puts("PASS guarded image comparison and copy: every simulated word boundary");
}
