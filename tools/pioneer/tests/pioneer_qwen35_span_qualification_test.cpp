/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQwen35SpanQualificationClient.h"
#include <cassert>
#include <cstdio>
struct FakeTransport {
    unsigned calls=0,uploads=0,closes=0,failCall=0,badReply=0;
    int Open() { return 7; }
    void Failure(const char*,unsigned) {}
    int Close(int fd) { assert(fd==7); ++closes; return 0; }
    bool Call(int fd,uint32_t op,void* data,size_t) {
        assert(fd==7); ++calls;
        if (calls==failCall) return false;
        if (op==TT_PCI_PROBE_Q38_SPAN_SUPPORT) *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};
        else if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};
        else if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
            auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={};r.version=1;
            r.state=TT_RESIDENT_SESSION_READY;r.active=r.host_ready=1;r.bytes=PioneerResidentBanks::Total;
        } else if (op==TT_PCI_PROBE_Q38_SPAN_UPLOAD) {
            auto& q=*static_cast<tt_q38_span_upload*>(data);
            assert(PioneerQwen35SpanQualificationRequest(q,uploads));
            ++uploads; q.result={1,TT_RUNTIME_PASSED,4096,1,uploads,1,1,1,1,1024,0,0};
        } else if (op==TT_PCI_PROBE_Q38_SPAN_END) *static_cast<tt_q38_span_end*>(data)={1,0,1,0};
        else {
            assert(op==TT_PCI_PROBE_Q38_BANK_END);
            auto& r=static_cast<tt_q38_bank_session*>(data)->result;r={};r.version=1;
            r.state=TT_RESIDENT_SESSION_CLOSED;r.pci_restored=r.host_released=1;
        }
        if (calls==badReply) {
            if (op==TT_PCI_PROBE_Q38_SPAN_UPLOAD) static_cast<tt_q38_span_upload*>(data)->result.nonce^=1;
            else *static_cast<uint32_t*>(data)^=1;
        }
        return true;
    }
};
int main()
{
    auto p=std::make_unique<tt_q38_span_upload>();p->version=1;p->bank=0;p->bytes=4096;
    for (unsigned i=0;i<1024;i++) p->data[i]=0x7fc00000U+i*0x9e3779b9U;
    assert(PioneerQwen35SpanQualificationRequest(*p,0));
    assert(!PioneerQwen35SpanQualificationRequest(*p,1));
    p->data[1023]^=1;assert(!PioneerQwen35SpanQualificationRequest(*p,0));p->data[1023]^=1;
    p->offset=4096;assert(!PioneerQwen35SpanQualificationRequest(*p,0));p->offset=0;
    assert(!PioneerQwen35SpanQualificationRequest(*p,7));
    FakeTransport good;assert(PioneerQwen35SpanQualificationClient(good));assert(good.uploads==7 && good.closes==1);
    for (unsigned i=1;i<=good.calls;i++) for (unsigned bad=0;bad<2;bad++) {
        FakeTransport f;if (bad) f.badReply=i;else f.failCall=i;
        assert(!PioneerQwen35SpanQualificationClient(f));assert(f.calls==i && f.closes==1);
    }
    puts("PASS Q35 fixed span qualification: seven banks, exact payload and close-only failures; SIMULATED ONLY");
}
