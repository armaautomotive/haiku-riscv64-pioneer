/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentBlockFinalReleaseRole.h"
// Actual bank-END branch,global semaphore already held. A compute RAII
// seal cannot outlive END because successful END destroys its context.
auto* recurrentEndContext=cookie?cookie->context:NULL;
const thread_id recurrentEndThread=find_thread(NULL);thread_info recurrentEndInfo={};
if(!recurrentEndContext || cookie->magic!=0x50313030 || sRuntimeOwner!=cookie
 || !sBankStorageOnly || !recurrentEndContext->bankStorageOnly || recurrentEndContext->io.q38MutableContext!=recurrentEndContext
 || recurrentEndContext->q38GdnCallThread!=-1 || recurrentEndThread<0
 || get_thread_info(recurrentEndThread,&recurrentEndInfo)!=B_OK || recurrentEndInfo.team!=cookie->team){
 status=B_NOT_ALLOWED;
}else{
 recurrentEndContext->q38GdnCallThread=recurrentEndThread;
 EndResidentSession(cookie,request.result,true);
 if(cookie->context==recurrentEndContext){
  if(sRuntimeOwner!=cookie || cookie->magic!=0x50313030
   || recurrentEndContext->q38GdnCallThread!=recurrentEndThread || find_thread(NULL)!=recurrentEndThread){
   tt_resident_session_result retained={};RetainQ38DispatchOwner(*recurrentEndContext,retained);status=B_DEV_NOT_READY;
  }else recurrentEndContext->q38GdnCallThread=-1;
 }else if(cookie->context){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*cookie->context,retained);status=B_DEV_NOT_READY;
 }
} // never dereference the destroyed context after successful END
