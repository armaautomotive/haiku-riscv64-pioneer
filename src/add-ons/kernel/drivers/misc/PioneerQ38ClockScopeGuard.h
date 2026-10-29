/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CLOCK_SCOPE_GUARD_H
#define PIONEER_Q38_CLOCK_SCOPE_GUARD_H
// Transport gate ONLY,never held/STOP/cleanup authority. The native factory
// must derive that authority before attaching its SAME private lifetime.
class PioneerQ38ClockScopeGuard {
 const void* owner=nullptr;bool (*scope)(const void*)=nullptr;
 bool stopped=false,sealed=false;
public:
 bool Pristine()const{return !owner && !scope && !stopped && !sealed;}
 bool Matches(const void* context,bool (*callback)(const void*))const{
  return owner==context && context && scope==callback && callback;
 }
 bool Stopped()const{return stopped;}
 bool Sealed()const{return sealed;}
 bool Bind(const void* context,bool (*callback)(const void*)){
  if(!Pristine() || !context || !callback)return false;
  owner=context;scope=callback;return true;
 }
 bool Check(){
  if(stopped || sealed)return false;
  if(!owner && !scope)return true; // ordinary pre-token normal-clock BEGIN
  if(!owner || !scope || !scope(owner)){stopped=true;return false;}
  return true;
 }
 bool Seal(const void* context,bool (*callback)(const void*)){
  if(!Matches(context,callback) || stopped || sealed)return false;
  sealed=true;return true;
 }
};
#endif
