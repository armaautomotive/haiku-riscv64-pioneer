/* SPDX-License-Identifier: MIT */
// Inside the REAL bank-END branch,with the actual global semaphore held.
// The ordinary compute call scope cannot surround EndResidentSession:
// successful END destroys its context. Establish the SAME real FD/team/
// thread seal,and release it ONLY if the retained context is still alive.
// Never dereference a successfully destroyed context or invent clean state.
auto* layerEndContext=cookie->context;
const thread_id layerEndThread=find_thread(NULL);thread_info layerEndInfo={};
if(!layerEndContext || cookie->magic!=0x50313030 || sRuntimeOwner!=cookie
 || !sBankStorageOnly || !layerEndContext->bankStorageOnly || layerEndContext->io.q38MutableContext!=layerEndContext
 || layerEndContext->q38GdnCallThread!=-1 || layerEndThread<0
 || get_thread_info(layerEndThread,&layerEndInfo)!=B_OK || layerEndInfo.team!=cookie->team){
 status=B_NOT_ALLOWED;
}else{
 layerEndContext->q38GdnCallThread=layerEndThread;
 EndResidentSession(cookie,request.result,true); // actual private END gates STILL owe every physical proof
 if(cookie->context==layerEndContext){
  if(sRuntimeOwner!=cookie || layerEndContext->q38GdnCallThread!=layerEndThread || find_thread(NULL)!=layerEndThread){
   tt_resident_session_result retained={};RetainQ38DispatchOwner(*layerEndContext,retained);status=B_DEV_NOT_READY;
  }else layerEndContext->q38GdnCallThread=-1; // call seal ONLY;no owner/unsafe/clock/cleanup state cleared
 }else if(cookie->context){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*cookie->context,retained);status=B_DEV_NOT_READY;
 }
} // a NULL context after genuine successful destruction is NEVER dereferenced
