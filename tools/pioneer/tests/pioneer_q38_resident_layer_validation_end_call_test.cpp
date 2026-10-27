/* SPDX-License-Identifier: MIT */
// Execute the ACTUAL END seal block against CPU-only framework metadata.
// NO native model/owner/ready/IO or cleanup proof is created by this test.
#include <cassert>
#include <cstdio>
#include <cstdint>
using thread_id=int;using status_t=int;
enum {B_OK=0,B_NOT_ALLOWED=-1,B_DEV_NOT_READY=-2};
struct thread_info{int team=7;};
struct Result{};struct Request{Result result;};
struct Context {struct IO{Context* q38MutableContext=nullptr;}io;
 bool bankStorageOnly=true,unsafe=true;int q38GdnCallThread=-1;
 Context(){io.q38MutableContext=this;}
};
struct Cookie{unsigned magic=0x50313030;Context* context=nullptr;int team=7;};
static Cookie* sRuntimeOwner=nullptr;static bool sBankStorageOnly=true,infoFail=false;
static int currentThread=42,currentTeam=7,behavior=0,endCalls=0,retains=0;
static Context* replacement=nullptr;
static int find_thread(void*){return currentThread;}
static int get_thread_info(int,thread_info* info){info->team=currentTeam;return infoFail?B_NOT_ALLOWED:B_OK;}
using tt_resident_session_result=Result;
static void RetainQ38DispatchOwner(Context& context,Result&){context.unsafe=true;retains++;}
static void EndResidentSession(Cookie* cookie,Result&,bool explicitEnd){
 assert(explicitEnd && cookie && cookie==sRuntimeOwner && cookie->context
  && cookie->context->q38GdnCallThread==currentThread);endCalls++;
 if(behavior==1){delete cookie->context;cookie->context=nullptr;sRuntimeOwner=nullptr;}
 if(behavior==2)cookie->context->q38GdnCallThread++;
 if(behavior==3)cookie->context=replacement;
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_END_CALL_TEST
 if(behavior==4)sRuntimeOwner=nullptr;
 if(behavior==5)currentThread++;
 if(behavior==6)cookie->magic=0;
#endif
}
static status_t Invoke(Cookie* cookie){Request request;status_t status=B_OK;
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_END_CALL_TEST
 #include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveFinalEndCall.inc"
#else
 #include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerValidationEndCall.inc"
#endif
 return status;
}
int main(){unsigned passed=0;
 for(unsigned fault=0;fault<10;fault++){
  auto* context=new Context;Cookie owner;owner.context=context;sRuntimeOwner=&owner;
  currentThread=42;currentTeam=7;sBankStorageOnly=true;infoFail=false;endCalls=retains=0;behavior=0;
  if(fault==0)owner.magic=0;if(fault==1)sRuntimeOwner=nullptr;if(fault==2)sBankStorageOnly=false;
  if(fault==3)context->bankStorageOnly=false;if(fault==4)context->io.q38MutableContext=nullptr;
  if(fault==5)context->q38GdnCallThread=42;if(fault==6)currentThread=-1;
  if(fault==7)currentTeam=8;if(fault==8)infoFail=true;if(fault==9)owner.context=nullptr;
  assert(Invoke(&owner)==B_NOT_ALLOWED && !endCalls && !retains && context->unsafe);
  delete context;passed++;
 }
 currentThread=42;currentTeam=7;sBankStorageOnly=true;infoFail=false;
 for(behavior=0;behavior<
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_END_CALL_TEST
  7
#else
  4
#endif
  ;behavior++){
  auto* context=new Context;Cookie owner;owner.context=context;sRuntimeOwner=&owner;
  replacement=new Context;endCalls=retains=0;
  const int result=Invoke(&owner);assert(endCalls==1);
  if(behavior==0)assert(result==B_OK && context->q38GdnCallThread==-1 && context->unsafe && !retains);
  if(behavior==1)assert(result==B_OK && !owner.context && !sRuntimeOwner && !retains); // ASAN proves no freed-context dereference
  if(behavior==2)assert(result==B_DEV_NOT_READY && retains==1 && context->unsafe);
  if(behavior==3)assert(result==B_DEV_NOT_READY && retains==1 && replacement->unsafe);
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_END_CALL_TEST
  if(behavior>=4)assert(result==B_DEV_NOT_READY && retains==1 && context->unsafe);
  currentThread=42;
#endif
  if(behavior!=1)delete context;delete replacement;passed++;
 }
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_END_CALL_TEST
 endCalls=retains=0;assert(Invoke(nullptr)==B_NOT_ALLOWED && !endCalls && !retains);passed++;
 printf("PASS ACTUAL private QBLNODRV bank-END call-seal block:%u CPU-only metadata/refusal/retained/destroyed/foreign-context/owner-thread-magic-change cases;ASAN no use-after-free,unsafe NEVER cleared;NO native/IO/recheck/cleanup qualification\n",passed);
#else
 printf("PASS ACTUAL DEBUG69 bank-END call-seal block:%u CPU-only metadata/refusal/retained/destroyed-context/thread-change/foreign-context cases;ASAN no use-after-free,unsafe NEVER cleared;NO native/IO/cleanup qualification\n",passed);
#endif
}
