/* SPDX-License-Identifier: MIT */
// Compile the actual driver open/free hooks with a fake lock and no PCI IO.
#define TT_PIONEER_DRIVER_RELOAD 1
#define PIONEER_QWEN35_DATAFLOW_REWRITE 1
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
#include <thread>

using uint32=uint32_t;
using status_t=int;
constexpr status_t B_OK=0,B_BUSY=-1,B_NOT_ALLOWED=-2,B_ENTRY_NOT_FOUND=-3,B_NO_MEMORY=-4;
constexpr uint32 kReadWrite=2,kExclusive=0x100;
#undef O_RWMASK
#undef O_RDWR
#undef O_EXCL
#define O_RWMASK 3
#define O_RDWR 2
#define O_EXCL 0x100
#define TT_PCI_PROBE_DEVICE "test/pioneer"
#define geteuid fake_geteuid
#define dprintf(...) ((void)0)
struct PioneerProbeCookie { int team=-1; };
struct thread_info { int team; };
static int sLock=1;
static uint32 sOpenCookies=0;
static PioneerProbeCookie* sQwen35ExclusiveCookie=nullptr;
static bool sColdRecoveryRequired=false,retainOnFree=false;
static PioneerProbeCookie* retained=nullptr;
static std::mutex lock;
static int fake_geteuid() { return 0; }
static int find_thread(void*) { return 1; }
static int get_thread_info(int,thread_info* info) { info->team=17; return B_OK; }
static int acquire_sem(int) { lock.lock(); return B_OK; }
static void release_sem(int) { lock.unlock(); }
static int ReleaseResidentCookie(PioneerProbeCookie* cookie) {
    if (retainOnFree) retained=cookie;
    else delete cookie;
    return B_OK;
}
#include "pioneer_probe_open.inc"
#include "pioneer_probe_free.inc"

int main()
{
    void *first=nullptr,*second=nullptr;
    assert(ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite,&first)==B_OK);
    assert(ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite|kExclusive,&second)==B_BUSY);
    assert(ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite,&second)==B_OK);
    assert(ProbeFree(first)==B_OK && ProbeFree(second)==B_OK && sOpenCookies==0);

    first=second=nullptr;
    assert(ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite|kExclusive,&first)==B_OK);
    assert(ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite,&second)==B_BUSY);
    assert(ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite|kExclusive,&second)==B_BUSY);
    assert(sQwen35ExclusiveCookie==first && sOpenCookies==1);
    // The failed owner's context may be retained after final free; the
    // exclusive descriptor must still release so a recovery inspector opens.
    retainOnFree=true;
    assert(ProbeFree(first)==B_OK && retained==first && !sQwen35ExclusiveCookie && !sOpenCookies);
    retainOnFree=false;
    assert(ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite,&second)==B_OK);
    assert(ProbeFree(second)==B_OK);
    delete retained; retained=nullptr;

    for (unsigned iteration=0;iteration<100;iteration++) {
        std::atomic<bool> start{false};
        void *exclusive=nullptr,*ordinary=nullptr;
        status_t a=99,b=99;
        std::thread t1([&] { while (!start.load(std::memory_order_acquire)) {};
            a=ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite|kExclusive,&exclusive); });
        std::thread t2([&] { while (!start.load(std::memory_order_acquire)) {};
            b=ProbeOpen(TT_PCI_PROBE_DEVICE,kReadWrite,&ordinary); });
        start.store(true,std::memory_order_release); t1.join(); t2.join();
        assert((a==B_OK && b==B_BUSY) || (a==B_BUSY && b==B_OK));
        if (exclusive) assert(ProbeFree(exclusive)==B_OK);
        if (ordinary) assert(ProbeFree(ordinary)==B_OK);
        assert(!sQwen35ExclusiveCookie && sOpenCookies==0 && !sColdRecoveryRequired);
    }
    std::puts("PASS exclusive rewrite open, competing opens, concurrent claim and retained-fault inspector; NO HARDWARE");
}
