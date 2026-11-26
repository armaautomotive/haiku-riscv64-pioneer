/* SPDX-License-Identifier: MIT */
// Actual retained pin and controller methods, simulated nodes and controller.
#include <condition_variable>
#include "../../../src/add-ons/kernel/busses/pci/ecam/Sg2042RecoveryLease.h"
/* Distributed under the MIT License. Actual pin/lifetime methods on fake nodes. */
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
using uint8=uint8_t;using thread_id=int;
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
struct mutex { std::mutex native; };
static void mutex_lock(mutex* m) { m->native.lock(); }
static void mutex_unlock(mutex* m) { m->native.unlock(); }
static thread_local thread_id current=-1;
static thread_id find_thread(const char*) { return current; }
struct FakeCondition {
    std::condition_variable cv;std::atomic<unsigned> waiting{0};
    status_t Wait(mutex* m) {
        std::unique_lock<std::mutex> lock(m->native,std::adopt_lock);
        ++waiting;cv.wait(lock);--waiting;lock.release();return B_OK;
    }
    void NotifyAll() { cv.notify_all(); }
};
class ECAMPCIController {
public:
    mutex fLock;bool fIsSg2042=true,fPrivatePioneerRoot=true;
    uint8 fStartBus=0x80,fEndBus=0xbf;uint64 fRegsPhysical=0x4800000000ULL;
    Sg2042RecoveryLease fPioneerRecoveryLease;FakeCondition fPioneerRecoveryCondition;
    alignas(4) uint8 root[4096]={},config[4096]={},translation[16]={};
    volatile uint8* fControllerRegs=root;volatile uint8* fRegs=config;volatile uint8* fAtRegs=translation;
    bool fRootConfigReadLogged=true,fDownstreamConfigReadLogged=true;uint8 fSg2042FunctionsLogged=0;
    status_t RunPioneerRecovery(status_t (*)(void*),void*,status_t*);
    status_t BeginPioneerRecovery(uint64&);
    status_t EndPioneerRecovery(uint64);
    status_t WaitPioneerConfigAccess();
    status_t BeginPioneerRetainedRecovery(const void*,uint64,uint64&);
    status_t ParkPioneerRetainedRecovery(const void*,uint64,uint64);
    status_t ResumePioneerRetainedRecovery(const void*,uint64,uint64);
    status_t FinishPioneerRetainedRecovery(const void*,uint64,uint64,bool);
};
#include "sg2042_recovery_controller.inc"

#include "../../../src/add-ons/kernel/drivers/misc/PioneerRetainedRecoveryScope.h"
using Scope=PioneerRetainedRecoveryScope<PioneerRetainedDriverPin,ECAMPCIController>;
struct Context { explicit Context(ECAMPCIController* c):controller(c) {} ECAMPCIController* controller;device_node* remove=nullptr;int calls=0;bool pending=true;status_t result=B_DEV_NOT_READY;std::thread waiter;std::atomic<int> waiterResult{123}; };
static status_t Access(ECAMPCIController& controller) {
    mutex_lock(&controller.fLock);status_t s=controller.WaitPioneerConfigAccess();mutex_unlock(&controller.fLock);return s;
}
static status_t Run(void* opaque,bool* pending) {
    auto& c=*static_cast<Context*>(opaque);++c.calls;assert(Access(*c.controller)==B_OK);
    if (c.calls==1 && !c.remove) {
        c.waiter=std::thread([&c]{current=22;c.waiterResult=Access(*c.controller);});
        while(!c.controller->fPioneerRecoveryCondition.waiting.load()) std::this_thread::yield();
        assert(c.waiterResult==123);
    }
    if(c.remove) {std::lock_guard<std::recursive_mutex> lock(sLock);c.remove->removed=true;}
    *pending=c.pending;return c.result;
}
int main() {
    current=11;device_node root,card;card.parent=&root;int owner=0;
    assert(card.InitDriver()==B_OK);
    PioneerRetainedDriverPin cardPin,rootPin;uint64 cg=0,rg=0;
    assert(cardPin.Acquire(&card,&owner,7,cg)==B_OK);
    assert(rootPin.Acquire(&root,&owner,7,rg)==B_OK);
    ECAMPCIController controller;Scope scope(cardPin,rootPin,controller,&owner,7,cg,rg);
    Context c{&controller};status_t operation=B_ERROR;
    Scope samePin(cardPin,cardPin,controller,&owner,7,cg,cg);
    assert(samePin.Step(Run,&c,operation)==Scope::Rejected && c.calls==0);
    Scope stale(cardPin,rootPin,controller,&owner,7,cg+1,rg);
    assert(stale.Step(Run,&c,operation)==Scope::Retained && c.calls==0);
    assert(!controller.fPioneerRecoveryLease.Active());
    assert(scope.Step(Run,&c,operation)==Scope::Pending && operation==B_DEV_NOT_READY);
    c.waiter.join();assert(c.waiterResult==B_DEV_NOT_READY);
    assert(controller.fPioneerRecoveryLease.Active() && controller.fPioneerRecoveryLease.Parked());
    assert(card.fInitialized==2 && root.fInitialized==3);
    uint64 ticket=scope.Ticket();assert(ticket);
    current=33;c.pending=false;c.result=B_OK;
    assert(scope.Step(Run,&c,operation)==Scope::AwaitingProof && operation==B_OK && c.calls==2);
    assert(scope.Ticket()==ticket && controller.fPioneerRecoveryLease.Parked());
    assert(scope.Step(Run,&c,operation)==Scope::Rejected && c.calls==2);
    // Simulation-only cleanup: this flag is not a physical release proof.
    assert(controller.ResumePioneerRetainedRecovery(&owner,7,ticket)==B_OK);
    assert(controller.FinishPioneerRetainedRecovery(&owner,7,ticket,true)==B_OK);
    assert(rootPin.Release(&owner,7,rg,true)==B_OK);
    assert(cardPin.Release(&owner,7,cg,true)==B_OK);
    assert(card.UninitDriver());
    device_node r2,c2;c2.parent=&r2;assert(c2.InitDriver()==B_OK);
    PioneerRetainedDriverPin cp2,rp2;assert(cp2.Acquire(&c2,&owner,8,cg)==B_OK);
    assert(rp2.Acquire(&r2,&owner,8,rg)==B_OK);
    ECAMPCIController ctl2;Scope failed(cp2,rp2,ctl2,&owner,8,cg,rg);
    Context d{&ctl2};d.remove=&r2;d.pending=false;d.result=B_OK;
    assert(failed.Step(Run,&d,operation)==Scope::Retained && d.calls==1);
    assert(ctl2.fPioneerRecoveryLease.Parked());
    assert(failed.Step(Run,&d,operation)==Scope::Rejected && d.calls==1);
    ticket=failed.Ticket();assert(ctl2.ResumePioneerRetainedRecovery(&owner,8,ticket)==B_OK);
    assert(ctl2.FinishPioneerRetainedRecovery(&owner,8,ticket,true)==B_OK);
    assert(rp2.Release(&owner,8,rg,true)==B_OK && cp2.Release(&owner,8,cg,true)==B_OK);
    assert(c2.UninitDriver());
    assert(modules==0);std::puts("PASS retained pins and controller lease integration; SIMULATED ONLY");
}
