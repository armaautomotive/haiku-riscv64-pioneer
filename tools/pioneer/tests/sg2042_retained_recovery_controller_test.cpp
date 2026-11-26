/* SPDX-License-Identifier: MIT */
// Exercise the actual controller lease methods on a fake mutex/condition.
// This is a non-loadable witness: it grants no hardware recovery authority.
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include "../../../src/add-ons/kernel/busses/pci/ecam/Sg2042RecoveryLease.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;
using vuint8=volatile uint8;using vuint16=volatile uint16;using vuint32=volatile uint32;
using addr_t=uintptr_t;using thread_id=int;using status_t=int;
enum { B_OK=0,B_BAD_VALUE=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,B_ERROR=-4,B_DEV_NOT_READY=-5,B_ENTRY_NOT_FOUND=-6 };
enum {PCI_command=0x04,PCI_header_type=0x0e,PCI_header_type_mask=0x7f,PCI_header_type_PCI_to_PCI_bridge=1};
static constexpr uint32 kSg2042AtPciAddress0=0,kSg2042AtDescriptor0=8;
#define ROUNDDOWN(value,alignment) ((value)&~((alignment)-1))
#define dprintf(...) ((void)0)
template<class T> static T Sg2042MmioRead(volatile T* p) {return *p;}
template<class T> static void Sg2042MmioWrite(volatile T* p,T v) {*p=v;}
static uint32 TranslateSg2042BridgeBusNumbers(uint32 v,uint16,uint8,uint8,uint8,bool) {return v;}
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
    status_t ReadSg2042Config(uint8,uint8,uint8,uint16,uint8,uint32&);
    status_t WriteSg2042Config(uint8,uint8,uint8,uint16,uint8,uint32);
};
#include "sg2042_recovery_controller.inc"
#include "sg2042_config_access.inc"

static status_t Access(ECAMPCIController& controller) {
    mutex_lock(&controller.fLock);
    const status_t result=controller.WaitPioneerConfigAccess();
    mutex_unlock(&controller.fLock);return result;
}
int main() {
    ECAMPCIController controller;int owner=0,wrong=0;uint64 ticket=9,other=9;
    *reinterpret_cast<uint32*>(controller.root)=0x20421e30;
    current=11;
    assert(controller.BeginPioneerRetainedRecovery(nullptr,7,ticket)==B_BAD_VALUE && !ticket);
    assert(controller.BeginPioneerRetainedRecovery(&owner,0,ticket)==B_BAD_VALUE && !ticket);
    controller.fPrivatePioneerRoot=false;
    assert(controller.BeginPioneerRetainedRecovery(&owner,7,ticket)==B_NOT_ALLOWED && !ticket);
    controller.fPrivatePioneerRoot=true;
    assert(controller.BeginPioneerRetainedRecovery(&owner,7,ticket)==B_OK && ticket==1);
    assert(Access(controller)==B_OK);
    assert(controller.BeginPioneerRecovery(other)==B_BUSY && !other);
    assert(controller.EndPioneerRecovery(ticket)==B_BAD_VALUE);
    std::atomic<int> firstResult{123};
    std::thread first([&]{current=22;firstResult=Access(controller);});
    while (!controller.fPioneerRecoveryCondition.waiting.load()) std::this_thread::yield();
    assert(firstResult==123);
    assert(controller.ParkPioneerRetainedRecovery(&wrong,7,ticket)==B_BAD_VALUE);
    assert(controller.ParkPioneerRetainedRecovery(&owner,8,ticket)==B_BAD_VALUE);
    assert(controller.ParkPioneerRetainedRecovery(&owner,7,ticket)==B_OK);
    first.join();assert(firstResult==B_DEV_NOT_READY);
    assert(controller.fPioneerRecoveryLease.Active() && controller.fPioneerRecoveryLease.Parked());
    assert(Access(controller)==B_DEV_NOT_READY);
    uint32 unavailable=0;
    assert(controller.ReadSg2042Config(0,0,0,0,4,unavailable)==B_DEV_NOT_READY
        && unavailable==UINT32_MAX);
    assert(controller.WriteSg2042Config(0,0,0,4,4,0x1234)==B_DEV_NOT_READY
        && *reinterpret_cast<uint32*>(controller.root+4)==0);
    assert(controller.FinishPioneerRetainedRecovery(&owner,7,ticket,true)==B_BAD_VALUE);
    assert(controller.ResumePioneerRetainedRecovery(&wrong,7,ticket)==B_BAD_VALUE);
    assert(controller.ResumePioneerRetainedRecovery(&owner,8,ticket)==B_BAD_VALUE);
    assert(controller.ResumePioneerRetainedRecovery(&owner,7,ticket+1)==B_BAD_VALUE);
    controller.fPrivatePioneerRoot=false;
    assert(controller.ResumePioneerRetainedRecovery(&owner,7,ticket)==B_NOT_ALLOWED);
    controller.fPrivatePioneerRoot=true;
    // Two same-session resumptions must not both own config simultaneously.
    std::atomic<unsigned> started{0},attempted{0},winners{0};
    auto contend=[&](int id) {
        current=id;++started;
        while (started.load()!=2) std::this_thread::yield();
        const status_t result=controller.ResumePioneerRetainedRecovery(&owner,7,ticket);
        ++attempted;
        if (result==B_OK) {
            ++winners;
            while (attempted.load()!=2) std::this_thread::yield();
            assert(controller.ParkPioneerRetainedRecovery(&owner,7,ticket)==B_OK);
        } else assert(result==B_BAD_VALUE);
    };
    std::thread contenderA(contend,55),contenderB(contend,66);
    contenderA.join();contenderB.join();
    assert(winners==1 && controller.fPioneerRecoveryLease.Parked());
    current=33;
    assert(controller.ResumePioneerRetainedRecovery(&owner,7,ticket)==B_OK);
    assert(Access(controller)==B_OK && !controller.fPioneerRecoveryLease.Parked());
    assert(controller.FinishPioneerRetainedRecovery(&owner,7,ticket,false)==B_BAD_VALUE);
    std::atomic<int> secondResult{123};
    std::thread second([&]{current=44;secondResult=Access(controller);});
    while (!controller.fPioneerRecoveryCondition.waiting.load()) std::this_thread::yield();
    assert(secondResult==123);
    assert(controller.ParkPioneerRetainedRecovery(&owner,7,ticket)==B_OK);
    second.join();assert(secondResult==B_DEV_NOT_READY);
    current=44;assert(controller.ResumePioneerRetainedRecovery(&owner,7,ticket)==B_OK);
    controller.fPrivatePioneerRoot=false;
    assert(controller.FinishPioneerRetainedRecovery(&owner,7,ticket,true)==B_NOT_ALLOWED);
    controller.fPrivatePioneerRoot=true;
    assert(controller.FinishPioneerRetainedRecovery(&owner,7,ticket,true)==B_OK);
    assert(!controller.fPioneerRecoveryLease.Active() && Access(controller)==B_OK);
    assert(controller.ReadSg2042Config(0,0,0,0,4,unavailable)==B_OK && unavailable==0x20421e30);
    assert(controller.ResumePioneerRetainedRecovery(&owner,7,ticket)==B_BAD_VALUE);
    assert(controller.BeginPioneerRetainedRecovery(&owner,8,other)==B_OK && other==2);
    assert(controller.ParkPioneerRetainedRecovery(&owner,8,other)==B_OK);
    assert(controller.ResumePioneerRetainedRecovery(&owner,7,ticket)==B_BAD_VALUE);
    assert(controller.ResumePioneerRetainedRecovery(&owner,8,other)==B_OK);
    assert(controller.FinishPioneerRetainedRecovery(&owner,8,other,true)==B_OK);
    std::puts("PASS retained controller lease: park fails waiters closed, same session resumes, caller-controlled release gate; SIMULATED ONLY");
}
