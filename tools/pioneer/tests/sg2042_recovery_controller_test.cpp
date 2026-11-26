/* Distributed under the MIT License. */
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
enum {B_OK=0,B_BAD_VALUE=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,B_ENTRY_NOT_FOUND=-4,B_ERROR=-5,B_CANCELED=-6};
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
struct FakeCondition {
	std::condition_variable cv;std::atomic<unsigned> waiting{0};
	status_t Wait(mutex* m) {
		std::unique_lock<std::mutex> lock(m->native,std::adopt_lock);
		++waiting;cv.wait(lock);--waiting;lock.release();return B_OK;
	}
	void NotifyAll() {cv.notify_all();}
};
class ECAMPCIController {
public:
	mutex fLock;bool fIsSg2042=true,fPrivatePioneerRoot=true;
	uint8 fStartBus=0x80,fEndBus=0xbf;uint64 fRegsPhysical=0x4800000000ULL;
	Sg2042RecoveryLease fPioneerRecoveryLease;FakeCondition fPioneerRecoveryCondition;
	alignas(4) uint8 root[4096]={},config[4096]={},translation[16]={};
	volatile uint8* fControllerRegs=root;volatile uint8* fRegs=config;volatile uint8* fAtRegs=translation;
	bool fRootConfigReadLogged=true,fDownstreamConfigReadLogged=true;uint8 fSg2042FunctionsLogged=0;
	status_t RunPioneerRecovery(status_t (*operation)(void*),void* argument,status_t* operationStatus);
	status_t BeginPioneerRecovery(uint64& ticket);
	status_t EndPioneerRecovery(uint64 ticket);
	status_t WaitPioneerConfigAccess();
	status_t ReadSg2042Config(uint8,uint8,uint8,uint16,uint8,uint32&);
	status_t WriteSg2042Config(uint8,uint8,uint8,uint16,uint8,uint32);
};
#include "sg2042_recovery_controller.inc"
#include "sg2042_config_access.inc"
#include "pioneer_sg2042_lease_probe.inc"

struct CallbackContext {ECAMPCIController* controller; int calls;};
static status_t FailingOperation(void* arg) {
	CallbackContext& context=*static_cast<CallbackContext*>(arg);
	++context.calls;uint64 nested=0;
	assert(context.controller->BeginPioneerRecovery(nested)==B_BUSY && !nested);
	uint32 value=0;
	assert(context.controller->ReadSg2042Config(0,0,0,0,4,value)==B_OK);
	return B_ERROR;
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
	std::puts("PASS actual SG2042 controller lease and config methods: private-root admission, owner access, blocked read/write and wake; SIMULATED ONLY");
}
