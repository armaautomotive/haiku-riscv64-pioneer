/* Distributed under the MIT License. */
#include <cassert>
#include <cstdio>
#include <mutex>
#include <thread>
using status_t=int;
constexpr status_t B_OK=0,B_ERROR=-1,B_BAD_VALUE=-2,B_NOT_ALLOWED=-3;
constexpr unsigned NODE_FLAG_DEVICE_REMOVED=1;
using pioneer_dm_pinned_operation=status_t (*)(void*);
static std::recursive_mutex sLock;
struct RecursiveLocker {std::unique_lock<std::recursive_mutex> lock;explicit RecursiveLocker(std::recursive_mutex& m):lock(m) {}};
struct device_node {
    bool registered=true,initialized=true,removed=false;
    int refs=1,parentRefs=1,uninitCalls=0;
    bool IsRegistered() const {return registered;}
    bool IsInitialized() const {return initialized;}
    unsigned Flags() const {return removed?NODE_FLAG_DEVICE_REMOVED:0;}
    status_t InitDriver() {assert(initialized && refs>0);++refs;++parentRefs;return B_OK;}
    bool UninitDriver() {assert(refs>0 && parentRefs>0);++uninitCalls;--refs;--parentRefs;
        if(!refs)initialized=false;return !initialized;}
    void Remove() {assert(!removed);removed=true;UninitDriver();}
};
#include "pioneer_dm_driver_pin.inc"
struct Call {device_node* node;int count=0;bool remove=false;};
static status_t Operation(void* opaque) {
    auto& call=*static_cast<Call*>(opaque);++call.count;
    assert(call.node->initialized && call.node->refs==2 && call.node->parentRefs==2);
    if(call.remove) {
        // A different thread must be able to enter the device manager while
        // this callback is live; removal consumes the original references.
        std::thread other([&] {std::lock_guard<std::recursive_mutex> lock(sLock);call.node->Remove();});
        other.join();
        assert(call.node->removed && call.node->initialized && call.node->refs==1 && call.node->parentRefs==1);
    }
    return B_ERROR;
}
int main() {
    device_node n;Call call{&n};status_t result=B_OK;
    assert(pioneer_run_with_initialized_driver(nullptr,Operation,&call,&result)==B_BAD_VALUE);
    assert(pioneer_run_with_initialized_driver(&n,nullptr,&call,&result)==B_BAD_VALUE);
    assert(pioneer_run_with_initialized_driver(&n,Operation,&call,nullptr)==B_BAD_VALUE);
    n.registered=false;assert(pioneer_run_with_initialized_driver(&n,Operation,&call,&result)==B_NOT_ALLOWED && result==B_ERROR);
    n.registered=true;n.initialized=false;assert(pioneer_run_with_initialized_driver(&n,Operation,&call,&result)==B_NOT_ALLOWED);
    n.initialized=true;n.removed=true;assert(pioneer_run_with_initialized_driver(&n,Operation,&call,&result)==B_NOT_ALLOWED);
    n.removed=false;assert(pioneer_run_with_initialized_driver(&n,Operation,&call,&result)==B_OK);
    assert(result==B_ERROR && call.count==1 && n.refs==1 && n.parentRefs==1 && n.initialized);
    call.remove=true;assert(pioneer_run_with_initialized_driver(&n,Operation,&call,&result)==B_OK);
    assert(result==B_ERROR && call.count==2 && n.removed && !n.initialized && n.refs==0 && n.parentRefs==0);
    assert(pioneer_run_with_initialized_driver(&n,Operation,&call,&result)==B_NOT_ALLOWED);
    std::puts("PASS actual device-manager pin: ancestor references, unlocked callback and removal retention; simulated nodes");
}
