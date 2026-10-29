/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
 PioneerQ38ClockScopeGuard tokenClockGuard;
 bool TokenClockViewsRetained()const{
  return queueWriteArea>=B_OK && msiArea>=B_OK && csmArea>=B_OK && scratchArea>=B_OK && tlbArea>=B_OK && area>=B_OK
   && queueWrite && msi && csm && scratch && tlb && nodeID;
 }
 bool TokenClockViewsReleased()const{
  return queueWriteArea<B_OK && msiArea<B_OK && csmArea<B_OK && scratchArea<B_OK && tlbArea<B_OK && area<B_OK
   && !queueWrite && !msi && !csm && !scratch && !tlb && !nodeID;
 }
 __attribute__((noinline)) bool Map(){
  return tokenClockGuard.Pristine() && PioneerArcQueryIO::Map();
 } // no fresh aliases after native END begins or the guard has stopped/sealed
 __attribute__((noinline)) uint32 TlbRead(unsigned word){
  return tokenClockGuard.Check()?PioneerArcQueryIO::TlbRead(word):UINT32_MAX;
 }
 __attribute__((noinline)) void TlbWrite(unsigned word,uint32 value){
  if(tokenClockGuard.Check())PioneerArcQueryIO::TlbWrite(word,value);
 }
 __attribute__((noinline)) uint32 CsmRead(uint32 address){
  return tokenClockGuard.Check()?PioneerArcQueryIO::CsmRead(address):UINT32_MAX;
 }
 __attribute__((noinline)) void SetQueueBounds(uint32 base,uint32 request,uint32 response){
  if(tokenClockGuard.Check())PioneerArcQueryIO::SetQueueBounds(base,request,response);
 }
 __attribute__((noinline)) void QueueWrite(uint32 address,uint32 value){
  if(tokenClockGuard.Check())PioneerArcQueryIO::QueueWrite(address,value);
 }
 __attribute__((noinline)) uint32 MsiStatusRead(){
  return tokenClockGuard.Check()?PioneerArcQueryIO::MsiStatusRead():UINT32_MAX;
 }
 __attribute__((noinline)) void NotifyArc(){if(tokenClockGuard.Check())PioneerArcQueryIO::NotifyArc();}
 __attribute__((noinline)) void Pause(){if(tokenClockGuard.Check())PioneerArcQueryIO::Pause();}
#endif
