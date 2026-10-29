/* SPDX-License-Identifier: MIT */
// Members of the native TEST session. These callbacks expose only the SAME
// physical byte/register endpoints,not an owner/call/ready/completion grant.
Q38NativePeerTestBytes peerBytes;
void PreparePeerBytes(){
 peerBytes.context=this;
 peerBytes.word=[](void* p,unsigned worker,uint32_t address)->uint32_t&{return static_cast<PioneerSessionIO*>(p)->Raw(worker,address);};
 peerBytes.noc=[](void* p,unsigned worker,unsigned offset){return static_cast<PioneerSessionIO*>(p)->Core(worker).ResidentNocRead(offset);};
 peerBytes.events=[](void* p,unsigned worker,const uint32_t* delta,const uint32_t* commands,unsigned mask,bool pendingChanged,bool pending){
  auto& physical=static_cast<PioneerSessionIO*>(p)->Core(worker);assert(worker && worker<120 && delta[2]==delta[6] && !(mask&~1023U));
  physical.counters[0]+=delta[0];physical.counters[1]+=delta[1];physical.counters[3]+=delta[3];physical.accepted+=delta[2];
  physical.writeCounters[0]+=delta[4];physical.writeCounters[1]+=delta[5];physical.writeCounters[3]+=delta[7];
  for(unsigned i=0;i<10;i++)if(mask&(1U<<i))physical.command[i]=commands[i];
  if(pendingChanged){physical.pending=pending;physical.done=!pending;}
 }; // actual event DELTAS/explicit CPU writes;unrelated physical faults persist,NO journal recapture
 peerBytes.read=[](void* p,unsigned route,uint32_t address,uint8_t* out,unsigned count){
#ifdef PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE
  auto& self=*static_cast<PioneerSessionIO*>(p);
  if(!out || !count || count>16384 || (address|count)&3)return false;
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
  if(self.ReadMutableBytes(route,address,out,count))return true;
#else
  if(route==PioneerQ38ProjectionRoute(0) && address>=self.runtimeWorkspace && count<=67108864
   && address-self.runtimeWorkspace<=67108864-count){
   for(unsigned i=0;i<count/4;i++){const uint32 word=self.runtimeDram.at((address-self.runtimeWorkspace)/4+i);
    for(unsigned b=0;b<4;b++)out[i*4+b]=uint8(word>>(b*8));}return true;
  }
#endif
  return self.executionCpu && self.executionCpu->original && self.executionCpu->original->Read(route,address,out,count);
#else
  (void)p;(void)route;(void)address;(void)out;(void)count;return false;
#endif
 };
 peerBytes.put=[](void* p,unsigned route,uint32_t address,uint32_t word){
#ifdef PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE
  auto& self=*static_cast<PioneerSessionIO*>(p);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
  return self.PutMutableWord(route,address,word);
#else
  if(route!=PioneerQ38ProjectionRoute(0) || (address&3) || address<self.runtimeWorkspace || address-self.runtimeWorkspace>67108860)return false;
  self.runtimeDram.at((address-self.runtimeWorkspace)/4)=word;return true;
#endif
#else
  (void)p;(void)route;(void)address;(void)word;return false;
#endif
 };
 peerBytes.clock=[](void* p){return (UINT64_C(1)<<40)+static_cast<PioneerSessionIO*>(p)->clock*1000;};
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
 executionDramRead=[](TokenHeldEndpoints& p,unsigned route,uint32 address,uint8* out,unsigned bytes){
  auto& native=static_cast<PioneerSessionIO&>(p);return native.peerBytes.read(&native,route,address,out,bytes);
 };
 executionDramPut=[](TokenHeldEndpoints& p,unsigned route,uint32 address,uint32 word){
  auto& native=static_cast<PioneerSessionIO&>(p);return native.peerBytes.put(&native,route,address,word);
 };
#endif
} // BOOT-only profile refuses ALL learned/mutable transfers;matrix profile uses actual staged/original bytes ONLY
