/* SPDX-License-Identifier: MIT */
// Real device-manager service and driver-lifetime methods on simulated nodes.
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <new>
#include <thread>
using status_t=int; using uint64=uint64_t;
enum { B_OK=0,B_ERROR=-1,B_BAD_VALUE=-2,B_NOT_ALLOWED=-3,B_BUSY=-4,
    B_DEV_NOT_READY=-5,B_NO_MEMORY=-6 };
constexpr unsigned NODE_FLAG_DEVICE_REMOVED=1;
using pioneer_dm_retained_pin_operation=status_t (*)(void*);
static std::recursive_mutex sLock;
struct RecursiveLocker {
    std::unique_lock<std::recursive_mutex> lock;
    explicit RecursiveLocker(std::recursive_mutex& m):lock(m) {}
};
#define TRACE(...) ((void)0)
#define dprintf(...) ((void)0)
struct module_info {};
struct device_node;
struct driver_module_info : module_info {
    status_t (*init_driver)(device_node*,void**); void (*uninit_driver)(void*);
};
static int modules=0;
static status_t Init(device_node* n,void** p) {*p=n;return B_OK;}
static void Uninit(void* p) {assert(p);}
static driver_module_info fakeDriver{{},Init,Uninit};
static status_t get_module(const char*,module_info** out) {*out=&fakeDriver;++modules;return B_OK;}
static void put_module(const char*) {assert(modules>0);--modules;}
struct device_node {
    device_node* parent=nullptr;int fInitialized=0,refs=1;bool removed=false,registered=true;
    driver_module_info* fDriver=nullptr;void* fDriverData=nullptr;
    device_node* Parent() const {return parent;}
    const char* ModuleName() const {return "test";}
    void Acquire() {++refs;}
    void Release() {assert(refs>0);--refs;}
    bool IsRegistered() const {return registered;}
    bool IsInitialized() const {return fInitialized>0;}
    unsigned Flags() const {return removed?NODE_FLAG_DEVICE_REMOVED:0;}
    status_t InitDriver();bool UninitDriver();
};
#include "pioneer_dm_init_lifetime.inc"
#include "pioneer_dm_retained_pin_service.inc"

static status_t Result(void*) {return -77;}
static status_t Remove(void* p) {
    std::lock_guard<std::recursive_mutex> lock(sLock);
    static_cast<device_node*>(p)->removed=true;
    return -77;
}
struct Call {uint64 id;const void* owner;std::atomic<bool> entered{false},finish{false};};
static status_t Blocking(void* p) {
    auto& c=*static_cast<Call*>(p);status_t result=0;
    assert(pioneer_run_retained_pin(c.id,c.owner,1,Result,nullptr,&result)==B_BUSY);
    assert(pioneer_release_retained_pin(c.id,c.owner,1,true)==B_BUSY);
    c.entered=true;
    while (!c.finish.load()) std::this_thread::yield();
    return -77;
}
int main() {
    device_node root,card;card.parent=&root;int owner=0,other=0;uint64 id=99,id2=0;
    assert(pioneer_acquire_retained_pin(nullptr,&owner,1,&id)==B_BAD_VALUE && id==0);
    assert(pioneer_acquire_retained_pin(&card,&owner,1,nullptr)==B_BAD_VALUE);
    assert(pioneer_acquire_retained_pin(&card,nullptr,1,&id)==B_BAD_VALUE);
    assert(pioneer_acquire_retained_pin(&card,&owner,0,&id)==B_BAD_VALUE);
    assert(pioneer_acquire_retained_pin(&card,&owner,1,&id)==B_NOT_ALLOWED);
    assert(card.InitDriver()==B_OK);
    assert(pioneer_acquire_retained_pin(&card,&owner,1,&id)==B_OK && id);
    assert(pioneer_acquire_retained_pin(&root,&owner,1,&id2)==B_OK && id2>id);
    assert(card.fInitialized==2 && root.fInitialized==3 && card.refs==3 && root.refs==4);
    status_t result=0;
    assert(pioneer_run_retained_pin(id,&other,1,Result,nullptr,&result)==B_BAD_VALUE);
    assert(pioneer_run_retained_pin(id,&owner,2,Result,nullptr,&result)==B_BAD_VALUE);
    assert(pioneer_run_retained_pin(id+100,&owner,1,Result,nullptr,&result)==B_BAD_VALUE);
    assert(pioneer_run_retained_pin(id,&owner,1,nullptr,nullptr,&result)==B_BAD_VALUE);
    assert(pioneer_run_retained_pin(id,&owner,1,Result,nullptr,nullptr)==B_BAD_VALUE);
    assert(pioneer_release_retained_pin(id,&owner,1,false)==B_BAD_VALUE);
    Call call{id,&owner};status_t run=B_ERROR,operation=B_ERROR;
    std::thread worker([&]{run=pioneer_run_retained_pin(id,&owner,1,Blocking,&call,&operation);});
    while (!call.entered.load()) std::this_thread::yield();
    assert(pioneer_release_retained_pin(id,&owner,1,true)==B_BUSY);
    assert(pioneer_run_retained_pin(id,&owner,1,Result,nullptr,&result)==B_BUSY);
    call.finish=true;worker.join();assert(run==B_OK && operation==-77);
    assert(pioneer_run_retained_pin(id,&owner,1,Result,nullptr,&result)==B_OK && result==-77);
    assert(pioneer_run_retained_pin(id,&owner,1,Remove,&card,&result)==B_DEV_NOT_READY);
    assert(result==-77 && card.fInitialized==2 && root.fInitialized==3);
    {std::lock_guard<std::recursive_mutex> lock(sLock);card.removed=false;}
    assert(pioneer_run_retained_pin(id,&owner,1,Result,nullptr,&result)==B_DEV_NOT_READY);
    assert(pioneer_release_retained_pin(id2,&owner,1,true)==B_OK);
    assert(pioneer_release_retained_pin(id,&owner,1,true)==B_OK);
    assert(pioneer_release_retained_pin(id,&owner,1,true)==B_BAD_VALUE);
    assert(card.fInitialized==1 && root.fInitialized==1 && card.refs==2 && root.refs==2);
    assert(card.UninitDriver() && card.refs==1 && root.refs==1 && modules==0);
    sPioneerRetainedPinId=~uint64(0);
    assert(root.InitDriver()==B_OK);
    assert(pioneer_acquire_retained_pin(&root,&owner,1,&id)==B_BUSY && id==0);
    assert(root.UninitDriver() && modules==0 && !sPioneerRetainedPins);
    std::puts("PASS retained device-manager service; SIMULATED NODES ONLY");
}
