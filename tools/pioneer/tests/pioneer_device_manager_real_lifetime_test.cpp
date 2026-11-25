/* Distributed under the MIT License. Simulated nodes, actual lifetime methods. */
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
using status_t=int;
constexpr int B_OK=0,B_ERROR=-1,B_BAD_VALUE=-2,B_NOT_ALLOWED=-3;
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
    device_node* parent=nullptr;int fInitialized=0,refs=1;bool removed=false;
    driver_module_info* fDriver=nullptr;void* fDriverData=nullptr;
    device_node* Parent() const {return parent;}
    const char* ModuleName() const {return "test";}
    void Acquire(){++refs;}
    void Release(){assert(refs>0);--refs;}
    bool IsRegistered() const {return true;}
    bool IsInitialized() const {return fInitialized>0;}
    unsigned Flags() const {return removed?NODE_FLAG_DEVICE_REMOVED:0;}
    status_t InitDriver();bool UninitDriver();
};
#include "pioneer_dm_init_lifetime.inc"
#include "pioneer_dm_driver_pin.inc"
struct Call {device_node* leaf;device_node* parent;};
static status_t RemoveWhilePinned(void* opaque){
    auto& call=*static_cast<Call*>(opaque);
    assert(call.leaf->fInitialized==2 && call.parent->fInitialized==2);
    std::thread remover([&]{std::lock_guard<std::recursive_mutex> lock(sLock);
        call.leaf->removed=true;call.leaf->UninitDriver();});
    remover.join();
    assert(call.leaf->fInitialized==1 && call.parent->fInitialized==1);
    assert(call.leaf->fDriver && call.parent->fDriver && uninitialized==0 && modules==2);
    return B_ERROR;
}
int main(){
    device_node parent,leaf;leaf.parent=&parent;
    assert(leaf.InitDriver()==B_OK);
    assert(initialized==2 && modules==2 && leaf.fInitialized==1 && parent.fInitialized==1);
    Call call{&leaf,&parent};status_t result=B_OK;
    assert(pioneer_run_with_initialized_driver(&leaf,RemoveWhilePinned,&call,&result)==B_OK);
    assert(result==B_ERROR && leaf.removed && leaf.fInitialized==0 && parent.fInitialized==0);
    assert(initialized==2 && uninitialized==2 && modules==0 && leaf.refs==1 && parent.refs==1);
    assert(pioneer_run_with_initialized_driver(&leaf,RemoveWhilePinned,&call,&result)==B_NOT_ALLOWED);
    std::puts("PASS actual device-manager InitDriver/UninitDriver and pin across simulated removal");
}
