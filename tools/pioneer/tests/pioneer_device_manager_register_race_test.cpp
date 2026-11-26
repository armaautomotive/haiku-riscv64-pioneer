/* SPDX-License-Identifier: MIT */
// Actual device-manager registration method with simulated nodes and locks.
#include <atomic>
#include <barrier>
#include <cassert>
#include <cstdio>
#include <mutex>
#include <new>
#include <thread>
#include <vector>
using status_t=int;
enum {B_OK=0,B_BAD_VALUE=-1,B_NAME_IN_USE=-2,B_NO_MEMORY=-3};
struct device_attr {int id;};struct io_resource {};
static std::recursive_mutex sLock;
static thread_local unsigned lockDepth=0;
struct RecursiveLocker {
 std::unique_lock<std::recursive_mutex> lock;
 explicit RecursiveLocker(std::recursive_mutex& m):lock(m) {++lockDepth;}
 ~RecursiveLocker(){--lockDepth;}
};
static std::atomic<int> constructions{0},registrations{0};
static std::barrier<>* unlockedCheckGate=nullptr;
struct device_node;
static device_node* sRootNode=nullptr;
struct device_node {
 int id=0;std::vector<device_node*> children;
 device_node()=default;
 device_node(const char*,const device_attr* attrs):id(attrs?attrs->id:0) {++constructions;}
 device_node* FindChild(const device_attr* attrs) {
  device_node* found=nullptr;
  for (auto* child:children) if (child->id==attrs->id) {found=child;break;}
  // Reproduce the worst legal scheduling of an unlocked duplicate check:
  // all callers finish their read before any can proceed to registration.
  if (!lockDepth) {assert(unlockedCheckGate);unlockedCheckGate->arrive_and_wait();}
  return found;
 }
 status_t InitCheck() {return B_OK;}
 status_t AcquireResources(const io_resource*) {return B_OK;}
 status_t Register(device_node* parent) {
  assert(lockDepth>0);++registrations;
  if(parent)parent->children.push_back(this);else sRootNode=this;
  return B_OK;
 }
 void Release() {delete this;}
};
#define TRACE(...) ((void)0)
#include "pioneer_dm_register_node.inc"
int main() {
 device_node parent;device_attr attrs{7};constexpr int count=16;
 std::barrier ready(count);std::vector<std::thread> threads;
 std::barrier unlockedChecks(count);unlockedCheckGate=&unlockedChecks;
 status_t results[count]={};device_node* nodes[count]={};
 for(int i=0;i<count;i++) threads.emplace_back([&,i] {
  ready.arrive_and_wait();results[i]=register_node(&parent,"test",&attrs,nullptr,&nodes[i]);
 });
 for(auto& thread:threads)thread.join();
 unlockedCheckGate=nullptr;
 unsigned okay=0,duplicates=0;
 for(int i=0;i<count;i++) {
  if(results[i]==B_OK) {++okay;assert(nodes[i]);}
  else {assert(results[i]==B_NAME_IN_USE && !nodes[i]);++duplicates;}
 }
 if (okay!=1 || duplicates!=count-1)
  std::fprintf(stderr,"duplicate race: %u accepted, %u rejected\n",okay,duplicates);
 assert(okay==1 && duplicates==count-1 && constructions==1 && registrations==1 && parent.children.size()==1);
 assert(register_node(&parent,nullptr,&attrs,nullptr,nullptr)==B_BAD_VALUE);
 device_attr other{8};device_node* nested=nullptr;
 {RecursiveLocker held(sLock);assert(register_node(&parent,"test",&other,nullptr,&nested)==B_OK);}
 assert(nested && parent.children.size()==2);
 for(auto* child:parent.children)delete child;
 parent.children.clear();
 constructions=0;registrations=0;threads.clear();std::barrier rootReady(count);
 for(int i=0;i<count;i++) threads.emplace_back([&,i] {
  rootReady.arrive_and_wait();results[i]=register_node(nullptr,"root",&attrs,nullptr,nullptr);
 });
 for(auto& thread:threads)thread.join();okay=duplicates=0;
 for(auto result:results) {if(result==B_OK)++okay;else {assert(result==B_BAD_VALUE);++duplicates;}}
 assert(okay==1 && duplicates==count-1 && constructions==1 && registrations==1 && sRootNode);
 delete sRootNode;sRootNode=nullptr;
 std::puts("PASS device-manager duplicate and root registration are atomic; SIMULATED NODES ONLY");
}
