/* SPDX-License-Identifier: MIT */
// Real span ioctl/service/controller and Q35 RV32 loader over simulated IO.
#define PIONEER_QWEN35_SPAN_SERVICE_MOCK_ONLY
#include "pioneer_q38_span_service_test.cpp"
#include "../PioneerQwen35VerifiedSpan.h"

int main(int argc,char** argv)
{
    assert(argc==2);
    (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;
    (void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
    PioneerProbeCookie cookie;
    {
        tt_q38_span_support support={55,1,7,4096,TT_Q38_SPAN_PUBLIC_BYTES,1,1,0};
        assert(PioneerQwen35::VerifiedSpanSupport(support));
        support.driver=60;assert(!PioneerQwen35::VerifiedSpanSupport(support));
    }
    {
        NativeSpanContext c(argv[1]);Bind(cookie,c);
        for (unsigned bank=0;bank<7;bank++) {
            const unsigned pages=bank+1,bytes=pages*4096;
            auto request=Request(bytes,bank,PioneerResidentBanks::Capacity-bytes);
            assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,
                request.get(),sizeof(*request))==B_OK);
            const auto& result=request->result;
            assert(result.state==TT_RUNTIME_PASSED && result.pages==pages && result.held
                && !result.host_exact && !result.matched_words && !result.recovery_required);
            assert(c.io.lastDeviceWords.size()==bytes/4);
            for (unsigned w=0;w<bytes/4;w++) assert(c.io.lastDeviceWords[w]==request->data[w]);
            uint32_t address=0,route=0,index=0;
            assert(c.banks.Page(c.fixture.owner.dram,bank,request->offset,address,route,index));
            for (unsigned p=0;p<pages;p++) assert(c.initialized[index+p]
                && c.hashes[index+p]==PioneerResidentPageHash(request->data+p*1024));
            const unsigned touches=c.io.touches;
            assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,
                request.get(),sizeof(*request))==B_ENTRY_NOT_FOUND && c.io.touches==touches);
        }
        assert(c.uploadedPages==28);
        tt_q38_span_end end={1,0,0,0};
        assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK
            && end.closed && !end.recovery_required && !c.Unsafe());
        Unbind();
    }
    {
        NativeSpanContext c(argv[1]);Bind(cookie,c);auto request=Request(8192,0,0);
        assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,
            request.get(),sizeof(*request))==B_OK);
        assert(PioneerQwen35::VerifiedSpanReply(*request,0,0,8192,1));
        request->result.host_exact=0;
        assert(!PioneerQwen35::VerifiedSpanReply(*request,0,0,8192,1));
        request->result.host_exact=1;request->result.matched_words--;
        assert(!PioneerQwen35::VerifiedSpanReply(*request,0,0,8192,1));
        request->result.matched_words++;
        assert(!PioneerQwen35::VerifiedSpanReply(*request,0,0,8192,2));
        tt_q38_span_end end={1,0,0,0};
        assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK
            && end.closed && !end.recovery_required);
        Unbind();
    }
    {
        NativeSpanContext c(argv[1]);Bind(cookie,c);auto request=Request();
        c.io.deviceCorruptPacket=1; // a corrupted device write is detected by the strict readback
        assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_UPLOAD,
            request.get(),sizeof(*request))==B_OK);
        assert(!PioneerQwen35::VerifiedSpanReply(*request,0,0,4096,1)
            && request->result.recovery_required && !c.uploadedPages && c.Unsafe());
        Unbind();
    }
    for (unsigned fault=0;fault<2;fault++) {
        NativeSpanContext c(argv[1]);Bind(cookie,c);auto request=Request();
        if (!fault) c.io.packetFault=1; else c.io.summaryFault=7;
        assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,
            request.get(),sizeof(*request))==B_OK);
        assert(request->result.recovery_required && !request->result.pages && !c.uploadedPages
            && !c.initialized[0] && c.Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired);
        const unsigned touches=c.io.touches;
        tt_q38_span_end end={1,0,0,0};
        assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_DEV_NOT_READY
            && c.io.touches==touches);
        Unbind();
    }
    assert(acquired==released);
    puts("PASS Q35 preload ioctl/service/RV32: seven banks, page hashes and retained faults; SIMULATED ONLY");
}
