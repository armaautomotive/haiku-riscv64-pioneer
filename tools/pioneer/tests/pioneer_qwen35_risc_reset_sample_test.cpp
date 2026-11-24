/* SPDX-License-Identifier: MIT */
#define main existing_l2cpu_reset_test_main
#include "pioneer_qwen35_dataflow_l2cpu_reset_probe_test.cpp"
#undef main
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35RiscResetSample.h"

struct ResetIO : FakeIO {
    uint32_t words[8]={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,
        UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
    unsigned reads[8]{};int drift=-1;
    uint32_t SharedArcRiscResetRead(unsigned index) {
        assert(index<8 && ArcSelected());
        return words[index]+unsigned(int(index)==drift && reads[index]++!=0);
    }
};
int main() {
    {
        ResetIO io;Scope scope;Guard guard{io};uint32_t result[8]{};bool restoreFailed=true;
        assert(PioneerQ35SampleRiscResetWords(io,scope,guard,result,restoreFailed));
        for (unsigned i=0;i<8;i++) assert(result[i]==UINT32_MAX);
        assert(io.Restored() && io.writes==6 && !restoreFailed);
    }
    {
        ResetIO io;Scope scope;Guard guard{io};uint32_t result[8]{};bool restoreFailed=false;
        io.words[2]=0x13579bdf;io.drift=2;
        assert(!PioneerQ35SampleRiscResetWords(io,scope,guard,result,restoreFailed));
        for (uint32_t value:result) assert(!value);
        assert(io.Restored() && io.writes==6 && !restoreFailed);
    }
    {
        ResetIO io;Scope scope;Guard guard{io};uint32_t result[8]{};bool restoreFailed=false;
        io.boot=7;assert(!PioneerQ35SampleRiscResetWords(io,scope,guard,result,restoreFailed));
        assert(io.Restored() && !restoreFailed);
    }
    for (int drop=0;drop<6;drop++) {
        ResetIO io;Scope scope;Guard guard{io};uint32_t result[8]{};bool restoreFailed=false;
        io.dropWrite=drop;assert(!PioneerQ35SampleRiscResetWords(io,scope,guard,result,restoreFailed));
        assert(restoreFailed && io.writes==unsigned(drop+1));
        for (uint32_t value:result) assert(!value);
    }
    {
        ResetIO io;Scope scope;Guard guard{io};uint32_t result[8]{};bool restoreFailed=false;
        scope.allowed=false;assert(!PioneerQ35SampleRiscResetWords(io,scope,guard,result,restoreFailed));
        assert(!restoreFailed && !io.writes && io.Restored());
    }
    std::puts("PASS fixed eight RISC reset-word observations and fail-stop route restoration; SIMULATED ONLY");
}
