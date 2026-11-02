/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_DRAM_SWEEP_ROUTES_H
#define BLACKHOLE_DRAM_SWEEP_ROUTES_H
#include "BlackholeDramSamplesProbe.h"
// No sample payload arrays, GDDR writes, retries, engine release or arbitrary
// coordinates. Reuse the exact seven-bank firmware route/identity policy.
template<class IO> bool BlackholeDramSweepRoutes(IO& io,const tt_arc_telemetry_result& t,
 const tt_tensix_reset_result& c,tt_dram_samples_result& r){
 r={};r.version=1;r.data_restored=r.tlb_restored=1;
 auto fail=[&](){r.recovery_required=1;io.SweepQuarantineMemoryOnly();return false;};
 if(t.version!=1 || t.state!=TT_ARC_TELEMETRY_OK || !t.tlb_restored
  || (t.present_mask&0x3c1)!=0x3c1 || t.values[0]!=0x12050001 || t.values[6]!=0xf7
  || t.values[7]!=0x5515 || t.values[8] || t.values[9]!=0xfff
  || c.version!=1 || c.state!=TT_TENSIX_RESET_HELD || !c.tlb_restored
  || c.reset[0]!=0x47800 || c.reset[1]!=0x47800 || c.reset[2]!=0x47800)return fail();
 for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)r.source[i]=io.SourceRead(i);
 if(r.source[0]!=c.niu_config || !BlackholeDramBuildRoutes(r))return fail();
 for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)r.tlb_before[s][i]=io.DramTlbRead(s,i);
 for(unsigned s=0;s<2;s++){
  if(r.tlb_before[s][2]&0xffffc000)return fail();
  for(unsigned i=0;i<3;i++)if(io.DramTlbRead(s,i)!=r.tlb_before[s][i])return fail();
 }
 r.tlb_restored=0;r.state=TT_DRAM_SAMPLE_CONFIG_FAILED;
 for(unsigned b=0;b<7;b++){
  for(unsigned i=0;i<3;i++){
   const uint32_t v=BlackholeDramFixedTlbWord(0xffa00000,r.translated[b][0],r.translated[b][1],true,i);
   io.DramTlbWrite(1,i,v);if(io.DramTlbRead(1,i)!=v)return fail();
  }
  for(unsigned sample=0;sample<3;sample++){
   for(unsigned i=0;i<TT_DRAM_ID_WORDS;i++)r.id[b][sample][i]=io.DramIdRead(i);
   const auto* id=r.id[b][sample];
   const uint32_t xy=BlackholeDramPhysicalX(b)|(BlackholeDramPhysicalY(b)<<6);
   const uint32_t logical=r.translated[b][0]|(r.translated[b][1]<<6);
   if((id[0]&0xfff)!=xy || ((id[0]>>12)&127)!=17 || ((id[0]>>19)&127)!=12
    || (id[0]&0xf0000000)!=0x10000000 || id[1]!=(0x80000|BlackholeDramBank(b))
    || (id[2]&0x9000)!=0x8000 || ((id[3]&0xfff)!=xy && (id[3]&0xfff)!=logical)){
    r.state=TT_DRAM_SAMPLE_BAD_ROUTE;return fail();
   }
   for(unsigned i=0;i<TT_DRAM_ID_WORDS;i++)if(id[i]!=r.id[b][0][i])return fail();
  }
  r.routes_verified++;
 }
 for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++){
  r.source_after[i]=io.SourceRead(i);if(r.source_after[i]!=r.source[i])return fail();
 }
 for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){
  io.DramTlbWrite(s,i,r.tlb_before[s][i]);r.tlb_after[s][i]=io.DramTlbRead(s,i);
  if(r.tlb_after[s][i]!=r.tlb_before[s][i])return fail();
 }
 for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)if(io.DramTlbRead(s,i)!=r.tlb_before[s][i])return fail();
 r.tlb_restored=1;r.state=TT_DRAM_SAMPLE_ROUTES_OK;return true;
}
#endif
