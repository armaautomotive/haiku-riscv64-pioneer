/* SPDX-License-Identifier: MIT */
// Prove that the new default byte endpoint is exactly the old Get loop.
// No ROOT/worker instruction is skipped and no native hardware is used.
#define PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY
#include "pioneer_q38_global_chain_instruction_test.cpp"
struct RawBatchProbe:CohortBytes {
 unsigned route=0,base=0;mutable unsigned gets=0;
 uint32_t Get(unsigned r,unsigned address)const override{
  assert(r==route && address==base+gets*4);gets++;
  return (address*UINT32_C(1664525)+r)^UINT32_C(0x91e3a70b);
 }
 void Put(unsigned,unsigned,uint32_t)override{assert(false);}
 uint64_t Clock()const override{assert(false);return 0;}
 uint32_t RootWord(unsigned)const override{assert(false);return 0;}
};
int main(){
 (void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 RawBatchProbe probe;unsigned cases=0,bytes=0;
 for(unsigned bank=0;bank<7;bank++)for(unsigned start:{0x04000000U,0x7ffff000U,0xbfff8000U})for(unsigned size:{64,576,2304,3392,16384}){
  probe.route=PioneerQ38ProjectionRoute(bank);probe.base=start;probe.gets=0;std::vector<uint8_t> raw(size,0x5a);
  probe.ReadRaw(probe.route,start,raw.data(),size);assert(probe.gets==size/4);
  for(unsigned i=0;i<size;i++){
   unsigned address=start+(i&~3U);uint32_t word=(address*UINT32_C(1664525)+probe.route)^UINT32_C(0x91e3a70b);
   assert(raw[i]==uint8_t(word>>((i&3)*8)));
  }
  cases++;bytes+=size;
 }
 printf("PASS default CohortBytes ReadRaw/%u batches/%u exact bytes/all7banks+3addresses+Q8Q4Q6tile+16KiB sizes/exact Get addresses+counts/no other endpoint calls;NOT model or hardware qualification\n",cases,bytes);
}
