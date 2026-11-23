/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_RECOVERY_QUALIFY 1
#include <cassert>
#include <cstdint>
#include <cstdio>
using status_t=int;using uint64=uint64_t;
constexpr int B_OK=0,B_BAD_VALUE=-1,B_DEV_NOT_READY=-2;
struct module_info {};
struct pioneer_driver_recovery_guard_v2_info {
    module_info info;unsigned version=2;bool begin=true,complete=true,abort=true,validate=true;
};
struct ReloadGuard { unsigned detaches=0;void detach(uint64 generation,bool clean) {
    assert(generation==7 && clean);++detaches;
} } reload;
static ReloadGuard* sReloadGuard=&reload;
static uint64 sReloadGeneration=7;
static unsigned mode=0,putsV1=0,putsV2=0,pciCalls=0;
static pioneer_driver_recovery_guard_v2_info guard;
#define PIONEER_DRIVER_RECOVERY_GUARD_V2_MODULE "recovery/v2"
#define PIONEER_DRIVER_RELOAD_GUARD_MODULE "reload/v1"
static status_t get_module(const char*,module_info** out) {
    if (mode==1) return B_DEV_NOT_READY;
    *out=mode==2?nullptr:&guard.info;return B_OK;
}
static void put_module(const char* name) {
    if (name[0]=='r' && name[1]=='e' && name[2]=='c') ++putsV2;else ++putsV1;
}
static status_t Run() {
    status_t guarded=B_OK;
#include "pioneer_qualify_guard_check.inc"
    ++pciCalls;return B_OK;
}
int main() {
    for (mode=0;mode<5;mode++) {
        reload.detaches=putsV1=putsV2=pciCalls=0;sReloadGuard=&reload;guard={};
        if (mode==3) guard.version=1;
        if (mode==4) guard.validate=false;
        const int result=Run();
        assert((result==B_OK)==(mode==0));
        assert(pciCalls==(mode==0?1U:0U));
        assert(reload.detaches==(mode==0?0U:1U) && putsV1==(mode==0?0U:1U));
        assert(putsV2==(mode==1?0U:1U));
        assert((sReloadGuard!=nullptr)==(mode==0));
    }
    std::puts("PASS qualification rejects a missing or invalid recovery guard before PCI; SIMULATED ONLY");
}
