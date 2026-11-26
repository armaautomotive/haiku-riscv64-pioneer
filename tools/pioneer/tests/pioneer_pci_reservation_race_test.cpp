/* SPDX-License-Identifier: MIT */
// Actual PCI reservation and device-manager registration on simulated nodes.
#include <atomic>
#include <barrier>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>
using status_t=int;using uchar=unsigned char;using uint8=uint8_t;using uint64=uint64_t;
enum {B_OK=0,B_BAD_VALUE=-1,B_NAME_IN_USE=-2,B_NO_MEMORY=-3,B_ERROR=-4,B_DEVICE_NOT_FOUND=-5};
enum {B_STRING_TYPE=1,B_UINT8_TYPE=2,B_UINT64_TYPE=3};
#define B_DEVICE_BUS "device/bus"
#define B_DEVICE_PRETTY_NAME "device/pretty_name"
#define B_PCI_DEVICE_DOMAIN "pci/domain"
#define B_PCI_DEVICE_BUS "pci/bus"
#define B_PCI_DEVICE_DEVICE "pci/device"
#define B_PCI_DEVICE_FUNCTION "pci/function"
#define PCI_LEGACY_DRIVER_MODULE_NAME "bus_managers/pci/legacy_v1"
#define TRACE(...) ((void)0)
struct device_attr {const char* name;int type;union {const char* string;uint8 ui8;uint64 ui64;} value;};
struct io_resource {};
static std::recursive_mutex sLock;static thread_local unsigned lockDepth=0;
struct RecursiveLocker {
 std::unique_lock<std::recursive_mutex> lock;
 explicit RecursiveLocker(std::recursive_mutex& m):lock(m) {++lockDepth;}
 ~RecursiveLocker(){--lockDepth;}
};
static std::barrier<>* unlockedCheckGate=nullptr;
struct device_node;static device_node* sRootNode=nullptr;
struct device_node {
 std::string key;std::vector<device_node*> children;
 device_node()=default;
 device_node(const char*,const device_attr* attrs):key(attrs && attrs[0].name?attrs[0].value.string:"") {}
 device_node* FindChild(const device_attr* attrs) {
  device_node* found=nullptr;
  if(attrs && attrs[0].name) for(auto* child:children) if(child->key==attrs[0].value.string){found=child;break;}
  if(!lockDepth) {assert(unlockedCheckGate);unlockedCheckGate->arrive_and_wait();}
  return found;
 }
 status_t InitCheck(){return B_OK;}
 status_t AcquireResources(const io_resource*){return B_OK;}
 status_t Register(device_node* parent){assert(lockDepth>0);if(parent)parent->children.push_back(this);else sRootNode=this;return B_OK;}
 void Release(){delete this;}
};
#include "pioneer_dm_register_node.inc"
static device_node card,root;
static status_t GetNext(device_node* parent,const device_attr*,device_node** out) {
 assert(parent==&root && out && !*out);*out=&card;return B_OK;
}
static void Put(device_node*) {}
static status_t Unregister(device_node*) {assert(false);return B_ERROR;}
struct FakeManager {
 status_t (*get_next_child_node)(device_node*,const device_attr*,device_node**);
 status_t (*register_node)(device_node*,const char*,const device_attr*,const io_resource*,device_node**);
 status_t (*unregister_node)(device_node*);void (*put_node)(device_node*);
} manager{GetNext,register_node,Unregister,Put};
static FakeManager* gDeviceManager=&manager;
struct FakeDomain {device_node* root_node=&root;};
struct FakePCI {
 FakeDomain domain;
 status_t ResolveVirtualBus(uchar,uint8* domainOut,uint8* busOut) {*domainOut=0;*busOut=1;return B_OK;}
 FakeDomain* _GetDomainData(uint8){return &domain;}
} pci;
static FakePCI* gPCI=&pci;
#include "pioneer_pci_reserve_device.inc"
int main() {
 constexpr int count=16;std::barrier ready(count),unlockedChecks(count);
 unlockedCheckGate=&unlockedChecks;std::vector<std::thread> threads;
 status_t results[count]={};int cookies[count]={};
 for(int i=0;i<count;i++)threads.emplace_back([&,i]{
  ready.arrive_and_wait();results[i]=pci_reserve_device(1,0,0,"pioneer",&cookies[i]);
 });
 for(auto& thread:threads)thread.join();unlockedCheckGate=nullptr;
 unsigned okay=0,rejected=0;
 for(auto result:results) {if(result==B_OK)++okay;else {assert(result==B_NAME_IN_USE);++rejected;}}
 if(okay!=1 || rejected!=count-1)std::fprintf(stderr,"PCI reservation race: %u accepted, %u rejected\n",okay,rejected);
 assert(okay==1 && rejected==count-1 && card.children.size()==1 && card.children[0]->children.size()==1);
 for(auto* reservation:card.children) {for(auto* owner:reservation->children)delete owner;delete reservation;}
 card.children.clear();
 std::puts("PASS actual PCI reserve admits only one concurrent claimant; SIMULATED NODES ONLY");
}
