/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_SPAN_QUALIFICATION_CLIENT_H
#define PIONEER_QWEN35_SPAN_QUALIFICATION_CLIENT_H
#include <cstddef>
#include "PioneerQwen35VerifiedSpan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35SpanQualification.h"
#include <memory>

// A failed call or bad receipt is close-only: never retry, reset, or perform
// another device call after an uncertain operation.
template<class IO> static bool PioneerQwen35SpanQualificationClient(IO& io)
{
    const int fd=io.Open(); if (fd<0) return false;
    auto fail=[&]() { io.Close(fd); return false; };
    tt_q38_span_support span={}; tt_q38_bank_support banks={};
    if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_SUPPORT,&span,sizeof(span))
        || !PioneerQwen35::VerifiedSpanSupport(span)
        || !io.Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&banks,sizeof(banks))
        || banks.driver!=45 || banks.version!=1 || banks.banks!=7 || banks.page_bytes!=4096
        || banks.max_pages!=4 || banks.storage_only!=1 || banks.bytes_per_bank!=PioneerResidentBanks::Capacity)
        return fail();
    tt_q38_bank_session session={}; session.version=1;
    if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))
        || session.version!=1 || session.reserved || session.result.version!=1
        || session.result.state!=TT_RESIDENT_SESSION_READY || session.result.active!=1
        || session.result.host_ready!=1 || session.result.cold || session.result.capabilities
        || session.result.bytes!=PioneerResidentBanks::Total) return fail();
    auto request=std::make_unique<tt_q38_span_upload>();
    for (unsigned bank=0;bank<7;bank++) {
        *request={}; request->version=1; request->bank=bank; request->bytes=4096;
        for (unsigned i=0;i<1024;i++) request->data[i]=0x7fc00000U+(i+bank*271U)*0x9e3779b9U;
        if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_UPLOAD,request.get(),sizeof(*request))
            || !PioneerQwen35SpanQualificationRequest(*request,bank)
            || !PioneerQwen35::VerifiedSpanReply(*request,bank,0,4096,bank+1)) return fail();
    }
    tt_q38_span_end spanEnd={1,0,0,0};
    if (!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&spanEnd,sizeof(spanEnd))
        || spanEnd.version!=1 || spanEnd.reserved || spanEnd.closed!=1 || spanEnd.recovery_required)
        return fail();
    session={}; session.version=1;
    if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))
        || session.version!=1 || session.reserved || session.result.version!=1
        || session.result.state!=TT_RESIDENT_SESSION_CLOSED || session.result.active
        || session.result.pci_restored!=1 || session.result.host_released!=1 || session.result.cold)
        return fail();
    return io.Close(fd)==0;
}
#endif
