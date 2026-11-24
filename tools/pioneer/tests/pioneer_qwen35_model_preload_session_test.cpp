/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
constexpr unsigned TT_QWEN35_DATAFLOW_DRIVER=60;
static int QuietPrint(const char*,...) { return 0; }
static int QuietError(FILE*,const char*,...) { return 0; }
static unsigned scenario=0,calls=0,begins=0,uploads=0,closes=0,ends=0;
struct Packed { bool Stable() const { return scenario!=5; } } packed;
struct Plan {} plan;
struct Manifest {} localManifest;
static Manifest* manifest=&localManifest;
static int64_t system_time() { return 1; }
static bool Call(int,uint32_t op,void* ptr,size_t) {
    ++calls;
    if (op==TT_PCI_PROBE_RELOAD_STATUS) {
        auto& s=*static_cast<tt_pioneer_driver_reload*>(ptr);
        s.driver=60;s.build_tag=UINT64_C(0x2026112300030018);s.generation=1;s.open_count=1;s.guard_version=1;
        if (scenario==1) s.cold=1;
        return true;
    }
    assert(op==TT_PCI_PROBE_Q38_BANK_END);++ends;
    auto& r=static_cast<tt_q38_bank_session*>(ptr)->result;
    r.state=TT_RESIDENT_SESSION_CLOSED;r.pci_restored=r.host_released=1;
    return scenario!=7;
}
static bool Begin(int,bool& open,bool preloadOnly) {
    ++begins;assert(preloadOnly);open=scenario!=2;return scenario!=2;
}
static bool Upload(int,Packed&,const Plan&,const Manifest&,bool& open,uint64_t& bytes) {
    ++uploads;open=scenario==3 || scenario==6;bytes=scenario==4?4096:UINT64_C(2740498432);
    return scenario!=3;
}
static int TestClose(int) { ++closes;return scenario==8?-1:0; }
#define close TestClose
#define printf QuietPrint
#define fprintf QuietError
static bool Run() {
    const int fd=7;
#include "pioneer_q35_model_preload_session.inc"
}
#undef fprintf
#undef printf
#undef close
int main() {
    for (scenario=0;scenario<=8;scenario++) {
        calls=begins=uploads=closes=ends=0;
        assert(Run()==(scenario==0));assert(closes==1);
        assert(begins==(scenario==1?0U:1U));
        assert(uploads==((scenario==1 || scenario==2)?0U:1U));
        assert(ends==((scenario==0 || scenario==4 || scenario==5 || scenario==7 || scenario==8)?1U:0U));
    }
    std::puts("PASS actual Q35 model preload session: closed upload, clean validation failures and uncertain close-only failures; SIMULATED ONLY");
}
