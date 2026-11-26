/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using uint32=uint32_t;using status_t=int;
enum {B_OK=0,B_BAD_VALUE=-1,B_ERROR=-2,B_NOT_ALLOWED=-3,B_CANCELED=-4,B_NO_INIT=-5};
struct module_info {};
using operation=status_t (*)(void*);
struct pioneer_sg2042_lease_probe_module_info {module_info info;status_t (*probe)(void*,uint32*,status_t*);};
struct pioneer_sg2042_recovery_module_info {module_info info;status_t (*run)(void*,operation,void*,status_t*);};
#define PIONEER_SG2042_LEASE_PROBE_MODULE "lease"
#define PIONEER_SG2042_RECOVERY_MODULE "recovery"
struct pioneer_ecam_lease_probe_result {
 uint32 version;status_t managerStatus,pciStatus;uint32 cards;status_t profileStatus;
 uint32 profileStage,profileReady;status_t leaseModuleStatus,leaseStatus,leaseCallbackStatus;
 uint32 leaseCallbacks;status_t recoveryModuleStatus,recoveryStatus;uint32 recoveryCallbacks;
};
static int mode=0,leaseRefs=0,recoveryRefs=0,device;
static status_t fixedProbe(void* cookie,uint32* count,status_t* result) {
 assert(cookie==&device);if(mode==3)return B_NOT_ALLOWED;
 *count=mode==4?2:1;*result=B_CANCELED;return B_OK;
}
static status_t recoveryRun(void*,operation op,void* arg,status_t* result) {
 if(mode==6) {*result=op(arg);return B_OK;}return B_NOT_ALLOWED;
}
static pioneer_sg2042_lease_probe_module_info lease{{},fixedProbe};
static pioneer_sg2042_recovery_module_info recovery{{},recoveryRun};
static status_t get_module(const char* name,module_info** out) {
 if(!strcmp(name,"lease")) {if(mode==1)return B_ERROR;++leaseRefs;*out=mode==2?nullptr:(module_info*)&lease;return B_OK;}
 assert(!strcmp(name,"recovery"));if(mode==5)return B_ERROR;
 ++recoveryRefs;*out=(module_info*)&recovery;return B_OK;
}
static void put_module(const char* name) {
 if(!strcmp(name,"lease"))--leaseRefs;else {assert(!strcmp(name,"recovery"));--recoveryRefs;}
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerEcamLeaseProbeLogic.h"
int main() {
 pioneer_ecam_lease_probe_result r{};PioneerEcamLeaseProbeCall call{&r};
 assert(PioneerEcamLeaseProbeCall::Run(nullptr,&call)==B_BAD_VALUE);
 for(mode=0;mode<=6;++mode) {
  r={};r.leaseStatus=r.leaseCallbackStatus=r.recoveryModuleStatus=r.recoveryStatus=B_NO_INIT;
  status_t status=PioneerEcamLeaseProbeCall::Run(&device,&call);
  assert(!leaseRefs && !recoveryRefs);
  if(mode==0)assert(status==B_OK && r.leaseCallbacks==1 && r.recoveryStatus==B_NOT_ALLOWED && !r.recoveryCallbacks);
  else assert(status!=B_OK);
  if(mode==6)assert(r.recoveryCallbacks==1 && r.recoveryStatus==B_OK);
 }
 std::puts("PASS fixed lease probe composition, failure paths and disabled general recovery; SIMULATED ONLY");
}
