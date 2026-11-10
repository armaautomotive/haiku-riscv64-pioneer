/* SPDX-License-Identifier: MIT */
// The real cookie-close body with synthetic OS and owner only.
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_QGWD_CONTROL_INSPECTION 1
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH 1
#include <cassert>
#include <cstdint>
#include <cstdio>
using status_t=int;constexpr int B_OK=0,B_BAD_VALUE=-1,B_BUSY=-2;
struct tt_resident_session_result {};
struct PioneerStoreContext {void* q38HybridReceipt=nullptr;void* q38Qgw8Receipt=nullptr;void* q38QgwdReceipt=nullptr;
 bool dispatchOwnerRetained=false;};
struct PioneerProbeCookie {uint32_t magic=0x50313030;PioneerStoreContext* context=nullptr;};
static int sLock=1,held=0,retains=0,ends=0;static bool sColdRecoveryRequired=false,failLock=false;
static status_t acquire_sem(int lock){assert(lock==sLock);if(failLock)return B_BUSY;assert(!held);held=1;return B_OK;}
static void release_sem(int lock){assert(lock==sLock && held);held=0;}
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result&){retains++;c.dispatchOwnerRetained=true;}
static void EndResidentSession(PioneerProbeCookie* cookie,tt_resident_session_result&,bool){ends++;cookie->context=nullptr;}
#include "pioneer_cookie_release.inc"
int main(){
 int marker=0;PioneerStoreContext context;context.q38QgwdReceipt=&marker;
 auto* cookie=new PioneerProbeCookie;cookie->context=&context;
 assert(ReleaseResidentCookie(cookie)==B_OK && !held && retains==1 && !ends && context.dispatchOwnerRetained && cookie->context==&context);
 delete cookie;
 auto* unavailable=new PioneerProbeCookie;unavailable->context=&context;failLock=true;
 assert(ReleaseResidentCookie(unavailable)==B_BUSY && !held && retains==1 && !ends && unavailable->context==&context);
 delete unavailable;puts("PASS QGWD cookie close retains its owner without END or IO; simulated OS only");
}
