/* SPDX-License-Identifier: MIT */
// Extracted actual ProbeControl dispatch; endpoints are inert test stubs.
#define PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION 1
#include <cassert>
#include <cstddef>
#include <cstdint>
using uint32=uint32_t;using status_t=int;
enum {B_NOT_ALLOWED=-1,TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT=0x8800,TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL=0x8801};
struct PioneerProbeCookie{};static unsigned support=0,control=0;static PioneerProbeCookie* seen=nullptr;
static status_t Q38FullOffloadSupport(void* data,size_t length){assert(data && length==32);support++;return 11;}
static status_t Q38TokenNativeControl(PioneerProbeCookie* cookie,void* data,size_t length){
 assert(cookie && data && length==64);seen=cookie;control++;return 22;
}
static status_t Route(PioneerProbeCookie* instance,uint32 operation,void* data,size_t length){
#include "pioneer_qgw8_full_dispatch.inc"
 return B_NOT_ALLOWED;
}
int main(){
 PioneerProbeCookie cookie;uint8_t buffer[64]={};
 assert(Route(&cookie,TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT,buffer,32)==11 && support==1 && !control);
 assert(Route(&cookie,TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL,buffer,64)==22 && support==1 && control==1 && seen==&cookie);
 assert(Route(&cookie,0xffffffffU,buffer,64)==B_NOT_ALLOWED && support==1 && control==1);
}
