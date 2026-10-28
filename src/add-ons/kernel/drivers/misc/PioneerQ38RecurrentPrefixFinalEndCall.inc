/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentPrefixFinalReleaseRole.h"
// Actual bank-END branch,global semaphore already held. A compute RAII
// seal cannot outlive END because successful END destroys its context.
auto* prefixEndContext=cookie?cookie->context:NULL;
const thread_id prefixEndThread=find_thread(NULL);thread_info prefixEndInfo={};
if(!prefixEndContext || cookie->magic!=0x50313030 || sRuntimeOwner!=cookie
 || !sBankStorageOnly || !prefixEndContext->bankStorageOnly || prefixEndContext->io.q38MutableContext!=prefixEndContext
 || prefixEndContext->q38GdnCallThread!=-1 || prefixEndThread<0
 || get_thread_info(prefixEndThread,&prefixEndInfo)!=B_OK || prefixEndInfo.team!=cookie->team){
 status=B_NOT_ALLOWED;
}else{
 prefixEndContext->q38GdnCallThread=prefixEndThread;
 EndResidentSession(cookie,request.result,true);
 if(cookie->context==prefixEndContext){
  if(sRuntimeOwner!=cookie || cookie->magic!=0x50313030
   || prefixEndContext->q38GdnCallThread!=prefixEndThread || find_thread(NULL)!=prefixEndThread){
   tt_resident_session_result retained={};RetainQ38DispatchOwner(*prefixEndContext,retained);status=B_DEV_NOT_READY;
  }else prefixEndContext->q38GdnCallThread=-1;
 }else if(cookie->context){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*cookie->context,retained);status=B_DEV_NOT_READY;
 }
} // never dereference the destroyed context after successful END
