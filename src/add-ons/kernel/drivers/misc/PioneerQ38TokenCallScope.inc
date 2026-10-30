/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
// One real nonreentrant FD/team/thread/global-lock scope for this distinct
// model family. Unlike the old scope,it can re-enter its OWN retained model;
// it does not suppress unrelated zero/graph/native unsafe state.
class PioneerQ38TokenCallScope {
 PioneerProbeCookie* cookie=NULL;PioneerStoreContext* context=NULL;
 thread_id thread=-1;status_t status=B_BAD_VALUE;bool held=false,destroyed=false;
public:
 explicit PioneerQ38TokenCallScope(PioneerProbeCookie* owner){
  if(!owner || owner->magic!=0x50313030)return;
  thread_info info={};thread=find_thread(NULL);
  if(thread<0 || get_thread_info(thread,&info)!=B_OK || info.team!=owner->team){status=B_NOT_ALLOWED;return;}
  status=AcquireQ38GdnSessionLock(owner);if(status!=B_OK)return;
  held=true;context=owner->context;
  if(owner->magic!=0x50313030 || sRuntimeOwner!=owner || !context || !sBankStorageOnly || !context->bankStorageOnly
   || context->bytes!=PioneerResidentBanks::Total || context->io.q38MutableContext!=context || context->q38GdnCallThread!=-1){
   context=NULL;status=B_NOT_ALLOWED;held=false;release_sem(sLock);return;
  }
  auto* state=context->q38TokenNative;
  if((state && !state->MatchesBinding(*context,owner,context->hashes,context->initialized,owner->team,sReloadGeneration))
   || context->Unsafe(false,false,true)){
   tt_resident_session_result retained={};RetainQ38DispatchOwner(*context,retained);
   context=NULL;status=B_DEV_NOT_READY;held=false;release_sem(sLock);return;
  }
  cookie=owner;context->q38GdnCallThread=thread;
 }
 PioneerQ38TokenCallScope(const PioneerQ38TokenCallScope&)=delete;
 PioneerQ38TokenCallScope& operator=(const PioneerQ38TokenCallScope&)=delete;
 ~PioneerQ38TokenCallScope(){
  if(!held)return;
  if(destroyed){release_sem(sLock);return;} // successful private Finish detached/deleted context UNDER this lock
  if(!context || !cookie || cookie->context!=context || cookie->magic!=0x50313030 || sRuntimeOwner!=cookie
   || context->q38GdnCallThread!=thread || find_thread(NULL)!=thread){
   if(context){tt_resident_session_result retained={};RetainQ38DispatchOwner(*context,retained);}
  }else context->q38GdnCallThread=-1;
  release_sem(sLock); // CPU seal only;NEVER END/STOP/restore/reset/ARC/unmap/free
 }
 PioneerStoreContext* Context()const{return status==B_OK && held?context:NULL;}
 status_t Status()const{return status;}
 bool Finish(PioneerQ38TokenEndResult&);
};
#endif
