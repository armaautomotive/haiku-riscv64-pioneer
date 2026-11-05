/* SPDX-License-Identifier: MIT */
// Pure fixed-register IO contract over MOCK endpoints;NEVER hardware.
#include "../PioneerBlackholeInstructionPrefetch.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
using P=PioneerBlackholeInstructionPrefetch;
struct IO {
 struct Event {char kind;unsigned index,value;};
 std::array<unsigned,224> config{};std::vector<Event> events;
 unsigned failAt=0,corruptAt=0;
 IO(unsigned control=0,unsigned limit=0){
  for(unsigned i=0;i<config.size();i++)config[i]=0x76543210U+i;
  config[P::Control]=control;config[P::Limit]=limit;
 }
 bool ReadConfig(unsigned at,unsigned& out){
  assert(at==P::Control || at==P::Limit);events.push_back({'R',at,config[at]});
  if(events.size()==failAt)return false;out=config[at]^(events.size()==corruptAt?1U:0U);return true;
 }
 bool WriteConfig(unsigned at,unsigned value){
  assert(at==P::Control || at==P::Limit);events.push_back({'W',at,value});
  config[at]=value;return events.size()!=failAt; // failed write MAY commit
 }
};
static void Positive(unsigned ctrl,unsigned limit){
 IO io(ctrl,limit);const auto initial=io.config;P scope;
 assert(scope.Begin(io) && !scope.IsUnsafe() && io.events.size()==10);
 const unsigned selected=(ctrl&~unsigned(P::RequestMask|P::EnableT0))|(P::Requests<<5)|P::EnableT0;
 assert(io.config[P::Control]==selected && io.config[P::Limit]==P::ProgramLastByte);
 for(unsigned i=0;i<initial.size();i++)if(i!=P::Control && i!=P::Limit)assert(io.config[i]==initial[i]);
 unsigned at=0;for(const auto& e:io.events){
  if(at<4)assert(e.kind=='R');
  if(at==4)assert(e.kind=='W' && e.index==P::Control && e.value==(ctrl&~1U));
  if(at==6)assert(e.kind=='W' && e.index==P::Limit && e.value==P::ProgramLastByte);
  if(at==8)assert(e.kind=='W' && e.index==P::Control && e.value==selected);at++;
 }
 assert(!scope.Begin(io) && io.events.size()==10);
 assert(scope.Restore(io) && !scope.IsUnsafe() && io.config==initial && io.events.size()==18);
 const auto count=io.events.size();assert(!scope.Restore(io) && !scope.Begin(io) && io.events.size()==count);
}
int main(){
 unsigned good=0,faults=0,withdrawals=0;
 for(unsigned ctrl=0;ctrl<65536;ctrl++){Positive(ctrl,ctrl^0xffffffffU);good++;}
 for(unsigned ctrl:{0xffffffffU,0xabcdef01U,0x800001e0U,0x12345678U})for(unsigned limit:{0U,1U,0xc0c67U,0x17ffffU,0xffffffffU}){Positive(ctrl,limit);good++;}
 for(bool corrupt:{false,true})for(unsigned event=1;event<=18;event++){
  // Corruption is meaningful only at actual read events.
  if(corrupt && (event==5 || event==7 || event==9 || event==13 || event==15 || event==17))continue;
  IO io(0xabcdef13,0x34567);P scope;if(corrupt)io.corruptAt=event;else io.failAt=event;
  const bool begun=scope.Begin(io);bool restored=false;if(begun)restored=scope.Restore(io);
  const unsigned detected=corrupt && event<=2?event+2:event;
  assert(!restored && scope.IsUnsafe() && io.events.size()==detected);
  const auto config=io.config;assert(!scope.Begin(io) && !scope.Restore(io) && io.events.size()==detected && io.config==config);faults++;
 }
 for(unsigned which:{unsigned(P::Limit),unsigned(P::Control)}){
  IO io;P scope;assert(scope.Begin(io));io.config[which]^=1;
  const auto config=io.config;assert(!scope.Restore(io) && scope.IsUnsafe() && io.config==config);withdrawals++;
 }
 IO fresh;P pristine;assert(!pristine.Restore(fresh) && fresh.events.empty());
 printf("PASS_FIXED_PREFETCH_POLICY positives%u all_low16_controls_reserved_upper_bits_exact18_operation_restore faults%u owner_changes%u fail_once_NO_following_IO_no_rollback;MOCK_ONLY_NO_DEVICE_OR_SPEED\n",good,faults,withdrawals);
}
