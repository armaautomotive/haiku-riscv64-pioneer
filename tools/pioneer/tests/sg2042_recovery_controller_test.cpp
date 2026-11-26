/* Distributed under the MIT License. */
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include "../../../src/add-ons/kernel/busses/pci/ecam/Sg2042RecoveryLease.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;
using vuint8=volatile uint8;using vuint16=volatile uint16;using vuint32=volatile uint32;
using addr_t=uintptr_t;using thread_id=int;using sem_id=int;using int32=int32_t;using status_t=int;
enum {B_OK=0,B_BAD_VALUE=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,B_ENTRY_NOT_FOUND=-4,B_ERROR=-5,B_CANCELED=-6,B_TIMED_OUT=-7,B_DEV_NOT_READY=-8};
enum {B_RELATIVE_TIMEOUT=1};
enum {PCI_command=0x04,PCI_header_type=0x0e,PCI_header_type_mask=0x7f,PCI_header_type_PCI_to_PCI_bridge=1};
static constexpr uint32 kSg2042AtPciAddress0=0,kSg2042AtDescriptor0=8;
#define ROUNDDOWN(value,alignment) ((value)&~((alignment)-1))
#define dprintf(...) ((void)0)
template<class T> static T Sg2042MmioRead(volatile T* p) {return *p;}
template<class T> static void Sg2042MmioWrite(volatile T* p,T v) {*p=v;}
static uint32 TranslateSg2042BridgeBusNumbers(uint32 v,uint16,uint8,uint8,uint8,bool) {return v;}
struct mutex {std::mutex native;};
static void mutex_lock(mutex* m) {m->native.lock();}
static void mutex_unlock(mutex* m) {m->native.unlock();}
static thread_local thread_id current=-1;
static thread_id find_thread(const char*) {return current;}
struct FakeSemaphore {std::mutex lock;std::condition_variable cv;int count=0;};
static FakeSemaphore sems[2];static bool forceTimeout=false;
static status_t release_sem(sem_id s) {assert(s>=0 && s<2);std::lock_guard<std::mutex> guard(sems[s].lock);++sems[s].count;sems[s].cv.notify_one();return B_OK;}
static status_t acquire_sem_etc(sem_id s,int32,uint32,int64_t) {
 assert(s>=0 && s<2);if(forceTimeout)return B_TIMED_OUT;
 std::unique_lock<std::mutex> lock(sems[s].lock);
 if(!sems[s].cv.wait_for(lock,std::chrono::seconds(3),[&]{return sems[s].count>0;}))return B_TIMED_OUT;
 --sems[s].count;return B_OK;
}
struct FakeCondition {
	std::condition_variable cv;std::atomic<unsigned> waiting{0};
	status_t Wait(mutex* m) {
		std::unique_lock<std::mutex> lock(m->native,std::adopt_lock);
		++waiting;cv.wait(lock);--waiting;lock.release();return B_OK;
	}
	void NotifyAll() {cv.notify_all();}
	int32 EntriesCount() {return waiting.load();}
};
class ECAMPCIController {
public:
	mutex fLock;bool fIsSg2042=true,fPrivatePioneerRoot=true;
	uint8 fStartBus=0x80,fEndBus=0xbf;uint64 fRegsPhysical=0x4800000000ULL;
	Sg2042RecoveryLease fPioneerRecoveryLease;FakeCondition fPioneerRecoveryCondition;
	thread_id fPioneerQualificationWaiter=-1;
	alignas(4) uint8 root[4096]={},config[4096]={},translation[16]={};
	volatile uint8* fControllerRegs=root;volatile uint8* fRegs=config;volatile uint8* fAtRegs=translation;
	bool fRootConfigReadLogged=true,fDownstreamConfigReadLogged=true;uint8 fSg2042FunctionsLogged=0;
	status_t RunPioneerRecovery(status_t (*operation)(void*),void* argument,status_t* operationStatus);
	status_t BeginPioneerRecovery(uint64& ticket);
	status_t EndPioneerRecovery(uint64 ticket);
	status_t WaitPioneerConfigAccess();
	status_t HoldPioneerQualification(sem_id,sem_id,status_t*);
	status_t PioneerQualificationState(bool*,thread_id*,int32*);
	status_t ReadSg2042Config(uint8,uint8,uint8,uint16,uint8,uint32&);
	status_t WriteSg2042Config(uint8,uint8,uint8,uint16,uint8,uint32);
};
#include "sg2042_recovery_controller.inc"
#include "sg2042_config_access.inc"
#include "pioneer_sg2042_lease_probe.inc"
#include "pioneer_sg2042_lease_contention.inc"

struct CallbackContext {ECAMPCIController* controller; int calls;};
static status_t FailingOperation(void* arg) {
	CallbackContext& context=*static_cast<CallbackContext*>(arg);
	++context.calls;uint64 nested=0;
	assert(context.controller->BeginPioneerRecovery(nested)==B_BUSY && !nested);
	uint32 value=0;
	assert(context.controller->ReadSg2042Config(0,0,0,0,4,value)==B_OK);
	return B_ERROR;
}
static status_t PendingOperation(void* arg) {
	CallbackContext& context=*static_cast<CallbackContext*>(arg);
	++context.calls;
	return B_DEV_NOT_READY;
}
int main()
{
	ECAMPCIController controller,other;current=11;uint64 ticket=0,wrong=0;
	current=-1;mutex_lock(&controller.fLock);
	assert(controller.WaitPioneerConfigAccess()==B_OK);mutex_unlock(&controller.fLock);current=11;
	CallbackContext context{&controller,0};status_t result=B_OK;
	assert(controller.RunPioneerRecovery(nullptr,&context,&result)==B_BAD_VALUE);
	assert(controller.RunPioneerRecovery(FailingOperation,&context,nullptr)==B_BAD_VALUE);
	controller.fPrivatePioneerRoot=false;
	assert(controller.RunPioneerRecovery(FailingOperation,&context,&result)==B_NOT_ALLOWED);
	assert(context.calls==0 && result==B_ERROR);
	controller.fPrivatePioneerRoot=true;
	assert(controller.RunPioneerRecovery(FailingOperation,&context,&result)==B_OK);
	assert(context.calls==1 && result==B_ERROR && !controller.fPioneerRecoveryLease.Active());
	controller.fPrivatePioneerRoot=false;
	assert(controller.BeginPioneerRecovery(ticket)==B_NOT_ALLOWED && !ticket);
	controller.fPrivatePioneerRoot=true;
	assert(controller.BeginPioneerRecovery(ticket)==B_OK && ticket==2);
	assert(controller.BeginPioneerRecovery(wrong)==B_BUSY && !wrong);
	mutex_lock(&controller.fLock);assert(controller.WaitPioneerConfigAccess()==B_OK);mutex_unlock(&controller.fLock);
	std::atomic<bool> done{false};
	std::thread reader([&] {current=22;mutex_lock(&controller.fLock);
		assert(controller.WaitPioneerConfigAccess()==B_OK);done=true;mutex_unlock(&controller.fLock);});
	while (!controller.fPioneerRecoveryCondition.waiting.load()) std::this_thread::yield();
	assert(!done.load() && controller.EndPioneerRecovery(ticket+1)==B_BAD_VALUE);
	assert(other.BeginPioneerRecovery(wrong)==B_OK && other.EndPioneerRecovery(wrong)==B_OK);
	assert(controller.EndPioneerRecovery(ticket)==B_OK);reader.join();assert(done.load());
	assert(controller.EndPioneerRecovery(ticket)==B_BAD_VALUE);
	assert(controller.BeginPioneerRecovery(ticket)==B_OK);
	*reinterpret_cast<uint32*>(controller.root)=0x20421e30;
	uint32 ownerRead=0;
	assert(controller.ReadSg2042Config(0,0,0,0,4,ownerRead)==B_OK && ownerRead==0x20421e30);
	std::atomic<bool> readDone{false},writeDone{false};uint32 observed=0;
	std::thread configReader([&] {current=22;
		assert(controller.ReadSg2042Config(0,0,0,0,4,observed)==B_OK);readDone=true;});
	std::thread configWriter([&] {current=33;
		assert(controller.WriteSg2042Config(0,0,0,4,4,0x402)==B_OK);writeDone=true;});
	while (controller.fPioneerRecoveryCondition.waiting.load()!=2) std::this_thread::yield();
	assert(!readDone.load() && !writeDone.load() && *reinterpret_cast<uint32*>(controller.root+4)==0);
	assert(controller.EndPioneerRecovery(ticket)==B_OK);configReader.join();configWriter.join();
	assert(readDone && writeDone && observed==0x20421e30 && *reinterpret_cast<uint32*>(controller.root+4)==0x402);
	// These assert the exact dword sent to the root, rather than modelling
	// W1C as ordinary memory. Untouched status bits must be sent as zero.
	auto* commandStatus=reinterpret_cast<uint32*>(controller.root+4);
	*commandStatus=0xf9100402;
	assert(controller.WriteSg2042Config(0,0,0,PCI_command,2,0x403)==B_OK && *commandStatus==0x00000403);
	*commandStatus=0xf9100402;
	assert(controller.WriteSg2042Config(0,0,0,PCI_command+1,1,0x05)==B_OK && *commandStatus==0x00000502);
	*commandStatus=0xf9100402;
	assert(controller.WriteSg2042Config(0,0,0,PCI_command+2,2,0x2000)==B_OK && *commandStatus==0x20000402);
	*commandStatus=0xf9100402;
	assert(controller.WriteSg2042Config(0,0,0,PCI_command+3,1,0x20)==B_OK && *commandStatus==0x20000402);
	*commandStatus=0xf9100402;
	assert(controller.WriteSg2042Config(0,0,0,PCI_command,4,0x20000403)==B_OK && *commandStatus==0x20000403);
	// A bridge-control halfword write becomes an aligned dword. The reset
	// caller sends zero for discard-timer W1C status even if that status
	// appeared after its earlier read; the interrupt line/pin remain intact.
	auto* interruptBridge=reinterpret_cast<uint32*>(controller.root+0x3c);
	*interruptBridge=0x04030122;
	assert(controller.WriteSg2042Config(0,0,0,0x3e,2,0x43)==B_OK
		&& *interruptBridge==0x00430122);
	*interruptBridge=0x04430122;
	assert(controller.WriteSg2042Config(0,0,0,0x3e,2,0x03)==B_OK
		&& *interruptBridge==0x00030122);
	other.fStartBus=0xc0;
	*reinterpret_cast<uint32*>(other.root+4)=0xf9100402;
	assert(other.WriteSg2042Config(0,0,0,PCI_command,2,0x403)==B_OK
		&& *reinterpret_cast<uint32*>(other.root+4)==0xf9100403);
	other.fStartBus=0x80;other.fEndBus=0xbe;
	*reinterpret_cast<uint32*>(other.root+4)=0xf9100402;
	assert(other.WriteSg2042Config(0,0,0,PCI_command,2,0x403)==B_OK
		&& *reinterpret_cast<uint32*>(other.root+4)==0xf9100403);
	other.fEndBus=0xbf;other.fRegsPhysical=0x4800001000ULL;
	*reinterpret_cast<uint32*>(other.root+4)=0xf9100402;
	assert(other.WriteSg2042Config(0,0,0,PCI_command,2,0x403)==B_OK
		&& *reinterpret_cast<uint32*>(other.root+4)==0xf9100403);
	uint32 callbacks=99;status_t callbackStatus=B_OK;
	assert(pioneer_sg2042_lease_probe(nullptr,&callbacks,&callbackStatus)==B_BAD_VALUE);
	assert(pioneer_sg2042_lease_probe(&controller,nullptr,&callbackStatus)==B_BAD_VALUE);
	assert(pioneer_sg2042_lease_probe(&controller,&callbacks,nullptr)==B_BAD_VALUE);
	assert(pioneer_sg2042_lease_probe(&other,&callbacks,&callbackStatus)==B_NOT_ALLOWED);
	assert(callbacks==0 && callbackStatus==B_ERROR);
	assert(pioneer_sg2042_lease_probe(&controller,&callbacks,&callbackStatus)==B_OK);
	assert(callbacks==1 && callbackStatus==B_CANCELED && !controller.fPioneerRecoveryLease.Active());
	bool active=false;thread_id waiter=-1;int32 waiters=0;
	assert(controller.PioneerQualificationState(nullptr,&waiter,&waiters)==B_BAD_VALUE);
	assert(other.PioneerQualificationState(&active,&waiter,&waiters)==B_NOT_ALLOWED);
	assert(controller.HoldPioneerQualification(0,0,&callbackStatus)==B_BAD_VALUE);
	status_t holdStatus=B_ERROR,holdCallback=B_ERROR;
	std::thread holder([&] {current=44;holdStatus=controller.HoldPioneerQualification(0,1,&holdCallback);});
	assert(acquire_sem_etc(0,1,B_RELATIVE_TIMEOUT,2000000)==B_OK);
	std::atomic<bool> competingDone{false};uint32 competingValue=0;
	std::thread competing([&] {current=55;assert(controller.ReadSg2042Config(0,0,0,0,4,competingValue)==B_OK);competingDone=true;});
	while (!controller.fPioneerRecoveryCondition.waiting.load()) std::this_thread::yield();
	assert(controller.PioneerQualificationState(&active,&waiter,&waiters)==B_OK);
	assert(active && waiter==55 && waiters==1 && !competingDone.load());
	assert(release_sem(1)==B_OK);holder.join();competing.join();
	assert(holdStatus==B_OK && holdCallback==B_OK && competingDone);
	assert(controller.PioneerQualificationState(&active,&waiter,&waiters)==B_OK && !active && waiters==0);
	forceTimeout=true;assert(controller.HoldPioneerQualification(0,1,&holdCallback)==B_OK);
	assert(holdCallback==B_TIMED_OUT && !controller.fPioneerRecoveryLease.Active());forceTimeout=false;
	// This is a deliberate negative control, not a recovery success: the
	// synchronous gateway releases its lease even when its callback reports a
	// retained/pending reset. A different thread can access config between
	// calls. The non-loadable gateway must not be used as continuous exclusion.
	current=11;const int priorCalls=context.calls;status_t pending=B_OK;
	assert(controller.RunPioneerRecovery(PendingOperation,&context,&pending)==B_OK
		&& pending==B_DEV_NOT_READY && context.calls==priorCalls+1
		&& !controller.fPioneerRecoveryLease.Active());
	current=22;uint32 betweenCalls=0;
	assert(controller.ReadSg2042Config(0,0,0,0,4,betweenCalls)==B_OK
		&& betweenCalls==0x20421e30);
	std::puts("PASS actual SG2042 controller lease and config methods: private-root admission, owner access, blocked read/write and wake; SIMULATED ONLY");
	std::puts("PASS negative control: pending callback releases the lease and permits another config reader; SIMULATED ONLY");
}
