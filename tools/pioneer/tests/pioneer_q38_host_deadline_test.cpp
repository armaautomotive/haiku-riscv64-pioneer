/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HostDeadline.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <type_traits>
int main(){
 static_assert(!std::is_copy_constructible<PioneerQ38HostDeadline>::value
  && !std::is_move_constructible<PioneerQ38HostDeadline>::value,"no copied/restarted clock interval");
 unsigned checks=0;
 for(uint64_t start:{UINT64_C(0),UINT64_C(1),UINT64_C(1000000000),UINT64_MAX-PioneerQ38HostDeadline::Microseconds}){
  PioneerQ38HostDeadline clock;assert(clock.Begin(start));checks++;
  for(uint64_t delta:{UINT64_C(0),UINT64_C(1),UINT64_C(1),PioneerQ38HostDeadline::Microseconds-1}){assert(clock.Observe(start+delta));checks++;}
  assert(!clock.Observe(start+PioneerQ38HostDeadline::Microseconds));checks++;
  for(uint64_t later:{start,start+1,UINT64_MAX}){assert(!clock.Observe(later) && !clock.Begin(later));checks+=2;}
 }
 for(unsigned delta=1;delta<=4096;delta++){
  PioneerQ38HostDeadline clock;assert(clock.Begin(100) && clock.Observe(100+delta));checks+=2;
  assert(!clock.Observe(99+delta) && !clock.Observe(100+delta) && !clock.Begin(100+delta));checks+=3;
 }
 for(uint64_t bad:{UINT64_MAX-PioneerQ38HostDeadline::Microseconds+1,UINT64_MAX-1,UINT64_MAX}){
  PioneerQ38HostDeadline clock;assert(!clock.Begin(bad) && !clock.Begin(0) && !clock.Observe(0));checks+=3;
 }
 {PioneerQ38HostDeadline clock;assert(!clock.Observe(0) && !clock.Begin(0));checks+=2;}
 {PioneerQ38HostDeadline clock;assert(clock.Begin(5) && !clock.Begin(5) && !clock.Observe(5));checks+=3;}
 printf("PASS CPU-only fixed30s monotonic deadline %u checks/expiry-equality/overflow/rollback/rebind/sticky failure;NO native or cleanup authority/device IO\n",checks);
}
