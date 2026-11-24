/* SPDX-License-Identifier: MIT */
// Actual host controller plus the generated Q35 RV32 image; device IO is simulated.
#define PIONEER_Q38_SPAN_SERVICE_MOCK_ONLY
#include "pioneer_q38_span_controller_test.cpp"

static bool UploadWork(SpanControlFixture& f,unsigned bank,unsigned bytes)
{
    std::vector<uint32_t> words(bytes/4);
    for (unsigned i=0;i<words.size();i++) words[i]=0x97e00000U^(i*101U);
    const bool okay=PioneerQ38SpanUpload<SpanControlIO,true>(f.io,f.owner.dram,f.owner.noc,
        f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.lease,
        f.initialized.data(),unsigned(f.initialized.size()),bank,
        PioneerResidentBanks::Capacity-bytes,bytes,words.data(),words.size(),f.result);
    if (okay) assert(f.io.lastDeviceWords==words);
    return okay;
}

int main(int argc,char** argv)
{
    assert(argc==2);
    (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;
    (void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
    {
        SpanControlFixture f(argv[1]);assert(f.PrepareSpan());
        for (unsigned bank=0;bank<7;bank++) for (unsigned bytes:{4096U,1048576U}) {
            assert(UploadWork(f,bank,bytes));
            assert(f.result.state==TT_RUNTIME_PASSED && f.result.pages==bytes/4096
                && f.result.started && f.result.completed && f.result.held
                && !f.result.host_exact && !f.result.recovery_required
                && !f.result.before[1] && !f.result.after[1]);
        }
        f.Finish();
    }
    for (unsigned slot=0;slot<32;slot++) {
        SpanControlFixture f(argv[1]);assert(f.PrepareSpan());f.io.summaryFault=slot;
        assert(!UploadWork(f,0,4096));f.Unsafe();
    }
    {
        SpanControlFixture f(argv[1]);assert(f.PrepareSpan());f.io.packetFault=1;
        assert(!UploadWork(f,0,4096) && f.result.marker==TT_Q38_SPAN_FAULT);f.Unsafe();
    }
    puts("PASS Q35 preload host controller and RV32 image: work uploads, all banks and retained faults; SIMULATED ONLY");
}
