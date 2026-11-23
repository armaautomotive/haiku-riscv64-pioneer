/* SPDX-License-Identifier: MIT */
// Execute the actual rewrite close/unload paths against fake ioctl replies.
#include <cassert>
#include <cstdint>
#include <cstdio>
using uint32=uint32_t;
constexpr unsigned TT_QWEN35_DATAFLOW_ABI=5,TT_Q38_SPAN_PUBLIC_VERSION=1,TT_Q38_BANK_PROTOCOL=1;
constexpr unsigned TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL=1,TT_PCI_PROBE_Q38_SPAN_END=2,TT_PCI_PROBE_Q38_BANK_END=3;
constexpr unsigned TT_QWEN35_DATAFLOW_ACTION_CLOSE=1,TT_QWEN35_DATAFLOW_ACTION_QUERY=2;
constexpr unsigned TT_QWEN35_DATAFLOW_RESULT_CLOSED=17,TT_QWEN35_DATAFLOW_RESULT_RESET_REQUIRED=32;
constexpr unsigned TT_QWEN35_DATAFLOW_RESULT_STOP_AUDIT_FAILED=15,TT_QWEN35_DATAFLOW_RESULT_RESUME_FAILED=18;
constexpr unsigned TT_QWEN35_DATAFLOW_RESULT_RESTORE_FAILED=19,TT_QWEN35_DATAFLOW_RESULT_RECOVERY_PENDING=13;
constexpr unsigned TT_QWEN35_DATAFLOW_STATE_CLOSED=2,TT_QWEN35_DATAFLOW_STATE_EMPTY=0,TT_QWEN35_DATAFLOW_STATE_FAULTED=3;
constexpr unsigned TT_RESIDENT_SESSION_CLOSED=7;
struct tt_qwen35_dataflow_request { unsigned version=0,action=0,result=0,state=0,recovery_required=0,reserved[3]{}; };
struct tt_q38_span_end { unsigned version=0,closed=0,recovery_required=0; };
struct tt_q38_bank_session { unsigned version=0;struct { unsigned state=0,active=0,pci_restored=0,host_released=0,cold=0; } result; };
static unsigned mode=0,closeCalls=0,resetCalls=0,forceCalls=0,bankCalls=0,spanCalls=0;
static bool ResetStoppedRewrite(int fd,bool force) { assert(fd==10);++resetCalls;forceCalls+=force;return mode!=1 && mode!=5 && mode!=7; }
static bool RecoverRewrite(int fd,bool) { assert(fd==10);return false; }
static int snooze(int) { return 0; }
static bool Call(int fd,uint32 operation,void* data,size_t) {
    assert(fd==10);
    if (operation==TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL) {
        auto& r=*static_cast<tt_qwen35_dataflow_request*>(data);
        assert(r.action==TT_QWEN35_DATAFLOW_ACTION_CLOSE);++closeCalls;
        if (mode>=6) return false; // cold latch or lost close response
        if (mode==2) { r.result=TT_QWEN35_DATAFLOW_RESULT_CLOSED;r.state=TT_QWEN35_DATAFLOW_STATE_CLOSED; }
        else { r.result=mode>=4?TT_QWEN35_DATAFLOW_RESULT_STOP_AUDIT_FAILED:TT_QWEN35_DATAFLOW_RESULT_RESET_REQUIRED;
            r.state=TT_QWEN35_DATAFLOW_STATE_FAULTED;r.recovery_required=1; }
        return true;
    }
    if (operation==TT_PCI_PROBE_Q38_SPAN_END) {
        auto& r=*static_cast<tt_q38_span_end*>(data);++spanCalls;
        r.closed=mode!=3;r.recovery_required=mode==3;return true;
    }
    assert(operation==TT_PCI_PROBE_Q38_BANK_END);++bankCalls;
    auto& r=*static_cast<tt_q38_bank_session*>(data);
    r.result.state=TT_RESIDENT_SESSION_CLOSED;r.result.pci_restored=r.result.host_released=1;return true;
}
#include "pioneer_close_client.inc"
#include "pioneer_end_client.inc"
int main() {
    for (mode=0;mode<8;mode++) {
        closeCalls=resetCalls=forceCalls=bankCalls=spanCalls=0;
        const bool okay=End(10,true,mode==3,true,true);
        assert(okay==(mode==0 || mode==2 || mode==4 || mode==6));
        assert(closeCalls==1 && resetCalls==(mode==2?0U:1U));
        assert(forceCalls==(mode>=4?1U:0U));
        assert(bankCalls==(mode==2?1U:0U) && spanCalls==(mode==3?1U:0U));
    }
    std::puts("PASS rewrite unload invokes guarded reset and never claims failed cleanup; SIMULATED ONLY");
}
