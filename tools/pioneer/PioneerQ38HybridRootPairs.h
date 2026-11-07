/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_ROOT_PAIRS_H
#define PIONEER_Q38_HYBRID_ROOT_PAIRS_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid ROOT translation is experimental and cannot select native or Work roles
#endif
#if defined(PIONEER_Q38_HEAD_MATRIX_SIMULATION) && (PIONEER_Q38_HEAD_MATRIX_SIMULATION != 1 \
 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE))
#error QGW8 synthetic relocation is DATA-only, not a ROOT build
#endif

// Translate the actual ROOT's existing canonical paired plan to the proposed
// converted layout. The caller MUST authenticate the seven destination entries
// against the actual uploaded model and seal the source/outputs while in use.
// This function establishes geometry only; it does not grant device ownership.
struct PioneerQ38HybridRootPairs {
 // Per-bank entry: original stripe address, relocated stripe address,
 // first output row, row count. The bank is the entry index.
#ifdef PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP
 template<class IO> static __attribute__((noinline,section(".bf16.root"))) bool Relocate(IO& io,unsigned layer,unsigned k,unsigned rows){
#else
 template<class IO> static bool Relocate(IO& io,unsigned layer,unsigned k,unsigned rows){
#endif
  if(layer>=64 || (k!=5120 && k!=17408) || rows!=(k==5120?17408U:5120U))return false;
#if defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || defined(PIONEER_Q38_HEAD_MATRIX_SIMULATION)
  const bool eight=layer>=50 && layer<=60;
#else
  const bool eight=layer>=48 && layer<=60;
#endif
  const unsigned oldBand=k==5120?92160U:313344U;
  const unsigned newBand=eight?(k==5120?174080U:591872U):oldBand;
  unsigned original[7],destination[7],first[7],bands[7],done[7];
  volatile unsigned* initial=done;for(unsigned bank=0;bank<7;bank++)initial[bank]=0;
  unsigned relocated[119];
  for(unsigned bank=0;bank<7;bank++){
   original[bank]=io.Table(bank,0);destination[bank]=io.Table(bank,1);
   first[bank]=io.Table(bank,2);const unsigned n=io.Table(bank,3);
   if((original[bank]&4095) || (destination[bank]&4095) || original[bank]<0x04000000
    || destination[bank]<0x04000000 || !n || ((first[bank]|n)&31)
    || first[bank]>=rows || n>rows-first[bank])return false;
   bands[bank]=n/32;
   if(bands[bank]>544 || original[bank]>0xc4000000U-bands[bank]*oldBand
    || destination[bank]>0xc4000000U-bands[bank]*newBand)return false;
   for(unsigned prior=0;prior<bank;prior++)
    if(first[bank]<first[prior]+bands[prior]*32 && first[prior]<first[bank]+n)return false;
  }
  for(unsigned w=0;w<119;w++){
   const unsigned bank=io.Result(w,1),groups=io.Result(w,2),row=io.ResultRow(w);
   if(bank>=7 || !groups || (groups&1) || groups>16 || io.Result(w,3) || io.Result(w,6) || io.Result(w,7)
    || io.Result(w,4)!=io.Route(bank) || !io.Result(w,5))return false;
   const unsigned count=groups/2;
   if(done[bank]>bands[bank] || count>bands[bank]-done[bank]
    || row!=first[bank]+done[bank]*32 || io.Result(w,0)!=original[bank]+done[bank]*oldBand)return false;
   relocated[w]=destination[bank]+done[bank]*newBand;done[bank]+=count;
  }
  unsigned total=0;for(unsigned bank=0;bank<7;bank++){if(done[bank]!=bands[bank])return false;total+=bands[bank];}
  if(total*32!=rows)return false;
  // Only publish after all seven stripes and all 119 jobs validate.
  for(unsigned w=0;w<119;w++)io.Output(w,0,relocated[w]);
  return true;
 }
};
#endif
