/* Distributed under the MIT License. Actual pin/lifetime methods on fake nodes. */
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
using status_t=int;using uint64=uint64_t;
enum { B_OK=0,B_ERROR=-1,B_BAD_VALUE=-2,B_NOT_ALLOWED=-3,B_BUSY=-4,B_DEV_NOT_READY=-5 };
constexpr unsigned NODE_FLAG_DEVICE_REMOVED=1;
using pioneer_dm_pinned_operation=status_t (*)(void*);
static std::recursive_mutex sLock;
struct RecursiveLocker {std::unique_lock<std::recursive_mutex> lock;explicit RecursiveLocker(std::recursive_mutex& m):lock(m) {}};
#define TRACE(...) ((void)0)
#define dprintf(...) ((void)0)
struct module_info {};
struct device_node;
struct driver_module_info : module_info {status_t (*init_driver)(device_node*,void**);void (*uninit_driver)(void*);};
static int initialized=0,uninitialized=0,modules=0;
static status_t Init(device_node* n,void** p){++initialized;*p=n;return B_OK;}
static void Uninit(void* p){assert(p);++uninitialized;}
static driver_module_info fakeDriver{{},Init,Uninit};
static status_t get_module(const char*,module_info** out){*out=&fakeDriver;++modules;return B_OK;}
static void put_module(const char*){assert(modules>0);--modules;}
struct device_node {
    device_node* parent=nullptr;int fInitialized=0,refs=1;bool removed=false,registered=true;
    driver_module_info* fDriver=nullptr;void* fDriverData=nullptr;
    device_node* Parent() const {return parent;}
    const char* ModuleName() const {return "test";}
    void Acquire(){++refs;}
    void Release(){assert(refs>0);--refs;}
    bool IsRegistered() const {return registered;}
    bool IsInitialized() const {return fInitialized>0;}
    unsigned Flags() const {return removed?NODE_FLAG_DEVICE_REMOVED:0;}
    status_t InitDriver();bool UninitDriver();
};
#include "pioneer_dm_init_lifetime.inc"
#include "pioneer_dm_retained_driver_pin.inc"
struct Call {std::atomic<bool> entered{false},release{false};device_node* leaf=nullptr;};
static status_t WaitOperation(void* opaque) {
    auto& call=*static_cast<Call*>(opaque);
    call.entered=true;while (!call.release.load()) std::this_thread::yield();
    return B_OK;
}
static status_t RemoveOperation(void* opaque) {
    auto& call=*static_cast<Call*>(opaque);
    std::thread remover([&]{std::lock_guard<std::recursive_mutex> lock(sLock);
        call.leaf->removed=true;call.leaf->UninitDriver();});
    remover.join();return B_OK;
}
int main() {
    device_node parent,leaf;leaf.parent=&parent;int owner=0,wrong=0;
    assert(leaf.InitDriver()==B_OK && leaf.fInitialized==1 && parent.fInitialized==1);
    PioneerRetainedDriverPin pin;uint64 generation=99;status_t operation=B_OK;Call call;call.leaf=&leaf;
    assert(pin.Acquire(nullptr,&owner,7,generation)==B_BAD_VALUE && !generation);
    assert(pin.Acquire(&leaf,nullptr,7,generation)==B_BAD_VALUE && !generation);
    assert(pin.Acquire(&leaf,&owner,0,generation)==B_BAD_VALUE && !generation);
    leaf.registered=false;assert(pin.Acquire(&leaf,&owner,7,generation)==B_NOT_ALLOWED);
    leaf.registered=true;leaf.removed=true;assert(pin.Acquire(&leaf,&owner,7,generation)==B_NOT_ALLOWED);
    leaf.removed=false;assert(pin.Acquire(&leaf,&owner,7,generation)==B_OK && generation==1);
    assert(leaf.fInitialized==2 && parent.fInitialized==2 && modules==2);
    uint64 duplicate=7;assert(pin.Acquire(&leaf,&owner,7,duplicate)==B_BUSY && !duplicate);
    assert(pin.Run(&wrong,7,generation,WaitOperation,&call,&operation)==B_BAD_VALUE);
    assert(pin.Run(&owner,8,generation,WaitOperation,&call,&operation)==B_BAD_VALUE);
    assert(pin.Run(&owner,7,generation+1,WaitOperation,&call,&operation)==B_BAD_VALUE);
    assert(pin.Run(&owner,7,generation,nullptr,&call,&operation)==B_BAD_VALUE);
    status_t callStatus=B_ERROR;
    std::thread runner([&]{callStatus=pin.Run(&owner,7,generation,WaitOperation,&call,&operation);});
    while (!call.entered.load()) std::this_thread::yield();
    assert(pin.Release(&owner,7,generation,true)==B_BUSY);
    status_t nested=B_OK;
    assert(pin.Run(&owner,7,generation,WaitOperation,&call,&nested)==B_BUSY);
    // The manager lock is not held by the callback.
    assert(sLock.try_lock());sLock.unlock();
    call.release=true;runner.join();assert(callStatus==B_OK && operation==B_OK);
    assert(pin.Release(&owner,7,generation,false)==B_BAD_VALUE);
    assert(pin.Run(&owner,7,generation,RemoveOperation,&call,&operation)==B_DEV_NOT_READY);
    assert(operation==B_OK && leaf.removed && leaf.fInitialized==1 && parent.fInitialized==1);
    assert(pin.Run(&owner,7,generation,WaitOperation,&call,&operation)==B_DEV_NOT_READY);
    // A transiently usable-looking node must not revive this retained pin.
    leaf.removed=false;
    assert(pin.Run(&owner,7,generation,WaitOperation,&call,&operation)==B_DEV_NOT_READY);
    assert(pin.Release(&owner,7,generation,true)==B_OK);
    assert(leaf.fInitialized==0 && parent.fInitialized==0 && modules==0 && leaf.refs==1 && parent.refs==1);
    assert(pin.Release(&owner,7,generation,true)==B_BAD_VALUE);
    device_node another;assert(another.InitDriver()==B_OK);
    uint64 newer=0;assert(pin.Acquire(&another,&owner,8,newer)==B_OK && newer==2);
    assert(pin.Run(&owner,7,generation,WaitOperation,&call,&operation)==B_BAD_VALUE);
    assert(pin.Release(&owner,8,newer,true)==B_OK);
    PioneerRetainedDriverPin peer;uint64 first=0,second=0;
    assert(pin.Acquire(&another,&owner,8,first)==B_OK && first==3);
    assert(peer.Acquire(&another,&owner,8,second)==B_OK && second==4);
    assert(another.fInitialized==3);
    assert(peer.Release(&owner,8,first,true)==B_BAD_VALUE);
    assert(peer.Release(&owner,8,second,true)==B_OK);
    assert(pin.Release(&owner,8,first,true)==B_OK && another.fInitialized==1);
    assert(another.UninitDriver() && modules==0);
    device_node last;assert(last.InitDriver()==B_OK);
    sPioneerRetainedPinGeneration=~uint64(0)-1;
    uint64 finalGeneration=0;
    assert(pin.Acquire(&last,&owner,9,finalGeneration)==B_OK && finalGeneration==~uint64(0));
    assert(pin.Release(&owner,9,finalGeneration,true)==B_OK);
    finalGeneration=123;
    assert(pin.Acquire(&last,&owner,9,finalGeneration)==B_BUSY && !finalGeneration);
    assert(last.fInitialized==1 && last.UninitDriver() && modules==0);
    std::puts("PASS retained actual driver pin: lifecycle, unlocked callback, removal, active-call exclusion and stale tickets; SIMULATED ONLY");
}
