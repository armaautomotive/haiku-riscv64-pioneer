/* SPDX-License-Identifier: MIT */
// Actual recursive lookup and iteration cleanup on simulated nodes.
#include <cassert>
#include <cstdio>
#include <mutex>
#include <vector>
using status_t=int;
enum {B_OK=0,B_ENTRY_NOT_FOUND=-1};
struct device_attr {int id;};
static std::recursive_mutex sLock;static thread_local unsigned lockDepth=0;
struct RecursiveLocker {
 std::unique_lock<std::recursive_mutex> lock;
 explicit RecursiveLocker(std::recursive_mutex& m):lock(m){++lockDepth;}
 ~RecursiveLocker(){--lockDepth;}
};
struct device_node;
struct NodeList {
 std::vector<device_node*> nodes;
 struct ConstIterator {
  const std::vector<device_node*>& nodes;size_t index=0;
  bool HasNext() const {return index<nodes.size();}
  device_node* Next(){return nodes[index++];}
 };
 ConstIterator GetIterator() const {return {nodes};}
};
static unsigned released=0,acquired=0;
struct device_node {
 int id=0;bool registered=true;NodeList children;
 const NodeList& Children() const {return children;}
 bool IsRegistered() const {return registered;}
 int CompareTo(const device_attr* attr) const {return id==attr->id?0:1;}
 void Acquire(){assert(lockDepth>0);++acquired;}
 void Release(){assert(lockDepth>0);++released;}
};
#include "pioneer_dm_find_child.inc"
int main() {
 device_node root,first,last,other;first.id=7;last.id=7;other.id=8;
 root.children.nodes={&first,&last,&other};device_attr target{7};
 device_node* cursor=nullptr;
 assert(find_child_node(&root,&target,&cursor)==B_OK && cursor==&first && acquired==1 && released==0);
 assert(find_child_node(&root,&target,&cursor)==B_OK && cursor==&last && acquired==2 && released==1);
 // The last cursor is released after recursive traversal reports no match.
 assert(find_child_node(&root,&target,&cursor)==B_ENTRY_NOT_FOUND && released==2);
 cursor=nullptr;device_attr absent{99};
 assert(find_child_node(&root,&absent,&cursor)==B_ENTRY_NOT_FOUND && !cursor && released==2);
 {RecursiveLocker held(sLock);cursor=nullptr;
  assert(find_child_node(&root,&target,&cursor)==B_OK && cursor==&first);}
 std::puts("PASS device-manager find-child cleanup holds the recursive lock; SIMULATED NODES ONLY");
}
