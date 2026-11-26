/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using uint32=uint32_t;using int32=int32_t;using status_t=int;using sem_id=int;using thread_id=int;
enum {B_OK=0,B_BAD_VALUE=-1,B_ERROR=-2,B_NOT_ALLOWED=-3,B_CANCELED=-4,B_NO_INIT=-5,
 B_ENTRY_NOT_FOUND=-6,B_UNSUPPORTED=-7};
struct module_info {const char* name;};
struct driver_module_info {module_info info;};
struct device_node {int id;int refs=0;};
struct device_manager_info {device_node*(*get_parent_node)(device_node*);void(*put_node)(device_node*);
 status_t(*get_driver)(device_node*,driver_module_info**,void**);};
namespace PioneerPinnedNodes {
struct Ref {device_manager_info* dm;device_node* node;Ref(device_manager_info* d,device_node* n):dm(d),node(n){}
 ~Ref(){if(node)dm->put_node(node);}};
}
using operation=status_t (*)(void*);
struct pioneer_sg2042_lease_probe_module_info {module_info info;status_t (*probe)(void*,uint32*,status_t*);};
struct pioneer_sg2042_recovery_module_info {module_info info;status_t (*run)(void*,operation,void*,status_t*);};
struct pioneer_sg2042_lease_contention_module_info {module_info info;status_t (*hold)(void*,sem_id,sem_id,status_t*);
 status_t (*state)(void*,bool*,thread_id*,int32*);};
#define PIONEER_SG2042_LEASE_PROBE_MODULE "probe"
#define PIONEER_SG2042_RECOVERY_MODULE "recovery"
#define PIONEER_SG2042_LEASE_CONTENTION_MODULE "contention"
struct pioneer_ecam_lease_probe_result {uint32 version;status_t managerStatus,pciStatus;uint32 cards;status_t profileStatus;
 uint32 profileStage,profileReady;status_t leaseModuleStatus,leaseStatus,leaseCallbackStatus;uint32 leaseCallbacks;
 status_t recoveryModuleStatus,recoveryStatus;uint32 recoveryCallbacks;};
struct pioneer_ecam_lease_hold {uint32 version;sem_id ready,release;status_t profileStatus;uint32 profileStage,profileReady;status_t holdStatus,callbackStatus;};
struct pioneer_ecam_lease_state {uint32 version;status_t stateStatus;uint32 active;thread_id waiter;int32 waiters;};
static int mode=0,refs=0,controllerCookie;
static status_t hold(void* c,sem_id a,sem_id b,status_t* result) {assert(c==&controllerCookie && a==10 && b==11);*result=mode==3?B_ERROR:B_OK;return B_OK;}
static status_t state(void* c,bool* active,thread_id* waiter,int32* waiters) {assert(c==&controllerCookie);*active=true;*waiter=44;*waiters=1;return B_OK;}
static pioneer_sg2042_lease_contention_module_info contention{{"contention"},hold,state};
status_t get_module(const char* name,module_info** out) {
 assert(!strcmp(name,"contention"));if(mode==1)return B_ENTRY_NOT_FOUND;++refs;*out=(module_info*)&contention;return B_OK;
}
void put_module(const char* name){assert(!strcmp(name,"contention") && refs==1);--refs;}
static device_node card{0},root{1},controller{2};
static device_node* parent(device_node* n){if(mode==4)return nullptr;device_node* p=n==&card?&root:&controller;++p->refs;return p;}
static void put(device_node* n){assert(n->refs>0);--n->refs;}
static driver_module_info driver{{"busses/pci/ecam/driver_v1"}},wrong{{"wrong"}};
static status_t get_driver(device_node* n,driver_module_info** d,void** c){assert(n==&controller);*d=mode==2?&wrong:&driver;*c=&controllerCookie;return B_OK;}
static device_manager_info manager{parent,put,get_driver};
#define PIONEER_ECAM_CONTENTION_QUALIFICATION
#include "../../../src/add-ons/kernel/drivers/misc/PioneerEcamLeaseProbeLogic.h"
int main(){
 pioneer_ecam_lease_hold h{1,10,11,B_NO_INIT,0,0,B_NO_INIT,B_NO_INIT};PioneerEcamLeaseHoldCall hc{&h};
 assert(PioneerEcamLeaseHoldCall::Run(&controllerCookie,&hc)==B_OK && h.holdStatus==B_OK && h.callbackStatus==B_OK && !refs);
 mode=1;h.holdStatus=B_NO_INIT;assert(PioneerEcamLeaseHoldCall::Run(&controllerCookie,&hc)==B_ENTRY_NOT_FOUND && !refs);
 mode=3;assert(PioneerEcamLeaseHoldCall::Run(&controllerCookie,&hc)==B_ERROR && !refs);
 mode=0;pioneer_ecam_lease_state s{1,B_NO_INIT,0,-1,0};PioneerEcamLeaseStateCall sc{&manager,&s};
 assert(PioneerEcamLeaseStateCall::Run(&card,nullptr,&sc)==B_OK && s.active==1 && s.waiter==44 && s.waiters==1);
 assert(!refs && !root.refs && !controller.refs);
 mode=1;s.stateStatus=B_NO_INIT;assert(PioneerEcamLeaseStateCall::Run(&card,nullptr,&sc)==B_ENTRY_NOT_FOUND);
 assert(!refs && !root.refs && !controller.refs);
 mode=2;assert(PioneerEcamLeaseStateCall::Run(&card,nullptr,&sc)==B_UNSUPPORTED);
 mode=4;assert(PioneerEcamLeaseStateCall::Run(&card,nullptr,&sc)==B_ENTRY_NOT_FOUND);
 assert(!refs && !root.refs && !controller.refs);
 std::puts("PASS contention hold and metadata-only state binding, refusals and reference cleanup; SIMULATED ONLY");
}
