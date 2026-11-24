/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <new>
using uint32=uint32_t;
constexpr unsigned TT_Q38_PANEL_VIEW_WORDS=0x42000/4,TT_Q38_PANEL_VIEW_BYTES=0x42000;
constexpr int B_OK=0,B_ANY_KERNEL_ADDRESS=1,B_UNCACHED_MEMORY=2,B_KERNEL_READ_AREA=4,B_KERNEL_WRITE_AREA=8;
static unsigned allocations=0,maps=0;
static bool failAllocation=false;
static void* TestMalloc(size_t size) { ++allocations;return failAllocation?nullptr:std::malloc(size); }
static int map_physical_memory(const char*,uint64_t,unsigned,int,int,void** out) {
    ++maps;*out=reinterpret_cast<void*>(uintptr_t(0x1000));return 1;
}
#define malloc TestMalloc
#define TT_PIONEER_EXPERIMENTAL_Q38_COHORT 1
#define PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_Q38_SPAN_PRELOAD 1
struct SessionIO {
    unsigned runtimeCore=0;bool fleetEnabled=false,q38SpanAllowed=true,runtimeAvailable=true;
    uint32* q38PanelSaved=nullptr;int q38PanelArea=-1;volatile uint32* q38PanelView=nullptr;
    bool q38PanelMapAttempted=false;struct { uint64_t bar0_host_start=0; } profile;
    bool SpanBeforeCohort() const { return q38SpanAllowed && !fleetEnabled; }
    bool MapRuntime() const { return runtimeAvailable; }
    bool Q38PanelMappingStable() const { return runtimeCore<120 && (!runtimeCore || fleetEnabled) && q38PanelArea>=B_OK && q38PanelView; }
#include "pioneer_q35_panel_mapping.inc"
    ~SessionIO() { std::free(q38PanelSaved); }
};
#undef malloc
using uint64=uint64_t;
constexpr int B_BUSY=-2,B_NO_MEMORY=-3;
struct PioneerProbeCookie { void* context=nullptr; };
static PioneerProbeCookie* sQwen35ExclusiveCookie=nullptr;
struct PioneerStoreContext {
    SessionIO io;int* initialized=reinterpret_cast<int*>(1);int* hashes=reinterpret_cast<int*>(1);
    explicit PioneerStoreContext(uint64) {}
};
#define PIONEER_QWEN35_DATAFLOW_REWRITE 1
static unsigned reservations=0;
static int BeginUntilReserve(PioneerProbeCookie* cookie,uint64 capacity,bool bankOnly) {
#include "pioneer_q35_bank_prealloc.inc"
    ++reservations;delete context;return B_OK;
}
int main() {
    allocations=maps=0;SessionIO preload;
    assert(preload.MapQ38Panel() && preload.q38PanelSaved && allocations==1 && maps==1);
    assert(preload.MapQ38Panel() && allocations==1 && maps==1);
    allocations=maps=0;failAllocation=true;SessionIO failure;
    assert(!failure.MapQ38Panel() && !failure.q38PanelSaved && allocations==1 && maps==0);
    failAllocation=false;allocations=maps=0;SessionIO fleet;fleet.fleetEnabled=true;fleet.runtimeCore=1;
    assert(fleet.MapQ38Panel() && !fleet.q38PanelSaved && allocations==0 && maps==1);
    allocations=maps=0;SessionIO handedOff;handedOff.q38SpanAllowed=false;
    assert(handedOff.MapQ38Panel() && !handedOff.q38PanelSaved && allocations==0 && maps==1);
    allocations=maps=0;SessionIO invalid;invalid.runtimeCore=1;
    assert(!invalid.MapQ38Panel() && allocations==0 && maps==0);
    PioneerProbeCookie cookie;sQwen35ExclusiveCookie=&cookie;
    allocations=reservations=0;failAllocation=true;
    assert(BeginUntilReserve(&cookie,0,true)==B_NO_MEMORY && allocations==1 && reservations==0);
    failAllocation=false;allocations=reservations=0;
    assert(BeginUntilReserve(&cookie,0,true)==B_OK && allocations==1 && reservations==1);
    std::puts("PASS actual panel map and pre-reservation allocation failure: one preload snapshot, no fleet snapshots; SIMULATED ONLY");
}
