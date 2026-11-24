/* SPDX-License-Identifier: MIT */
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowProtocol.h"
#include "../PioneerQwen35VerifiedSpan.h"
constexpr unsigned TT_QWEN35_DATAFLOW_CAP_DECODER_SEQUENCE=16384U,TT_QWEN35_DATAFLOW_CAP_GENERATION=32768U;
namespace PioneerQwen35 { enum { DataflowCoreCount=120,Banks=7,LoadPageBytes=4096 }; }
namespace D=PioneerQwen35;
struct PioneerQwen35PackedPlan { static constexpr uint64_t Capacity=PioneerResidentBanks::Capacity; };
static bool noDataflow=false,badBank=false,badSpan=false,badBegin=false;
static unsigned calls=0;
static bool Call(int,uint32_t op,void* ptr,size_t) {
    ++calls;
    if (op==TT_PCI_PROBE_QWEN35_DATAFLOW_SUPPORT) {
        if (noDataflow) return false;
        auto& s=*static_cast<tt_qwen35_dataflow_support*>(ptr);
        s={TT_QWEN35_DATAFLOW_ABI,TT_QWEN35_DATAFLOW_DRIVER,120,
           TT_QWEN35_DATAFLOW_CAP_RESIDENT_SERVICE|TT_QWEN35_DATAFLOW_CAP_MANIFEST_BIND
           |TT_QWEN35_DATAFLOW_CAP_TOKEN_EMBEDDING|TT_QWEN35_DATAFLOW_CAP_DECODER_SEQUENCE
           |TT_QWEN35_DATAFLOW_CAP_GENERATION,TT_QWEN35_DATAFLOW_PROJECTION_MATRIX,
           TT_QWEN35_DATAFLOW_TOKEN_EMBEDDING};return true;
    }
    if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
        auto& b=*static_cast<tt_q38_bank_support*>(ptr);
        b={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};if (badBank) b.banks=6;return true;
    }
    if (op==TT_PCI_PROBE_Q38_SPAN_SUPPORT) {
        auto& s=*static_cast<tt_q38_span_support*>(ptr);s={55,1,7,4096,1048576,1,1,0};
        if (badSpan) s.before_cohort=0;return true;
    }
    assert(op==TT_PCI_PROBE_Q38_BANK_BEGIN);
    auto& r=static_cast<tt_q38_bank_session*>(ptr)->result;r.state=TT_RESIDENT_SESSION_READY;
    r.active=r.host_ready=1;if (badBegin) r.cold=1;return true;
}
#include "pioneer_q35_begin_client.inc"
int main() {
    bool open=false;noDataflow=true;calls=0;
    assert(Begin(7,open,true) && open && calls==3);
    open=false;calls=0;assert(!Begin(7,open) && !open && calls==1);
    noDataflow=false;calls=0;assert(Begin(7,open) && open && calls==4);
    badBank=true;open=true;calls=0;assert(!Begin(7,open,true) && !open && calls==1);badBank=false;
    badSpan=true;open=true;calls=0;assert(!Begin(7,open,true) && !open && calls==2);badSpan=false;
    badBegin=true;open=false;calls=0;assert(!Begin(7,open,true) && open && calls==3);
    std::puts("PASS actual Q35 preload begin requires bank and span support; normal inference retains dataflow checks; SIMULATED ONLY");
}
