/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION 1
#include "../PioneerNativeModelBandPairInspection.h"
#include "PioneerNativeBfpModelBandMetadata.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>

// Fake MMIO, not an accelerator emulator. It deliberately withholds BOTH
// completions until both processors have started. Goldens are injected only
// to test result routing; this cannot validate numerical hardware results.
struct IO {
 std::vector<uint32_t> mem[2],original[2];
 uint32_t tlb[2][3]={{1,2,3},{4,5,6}},reset[2]={0x47800,0x47800};
 uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
  0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
 unsigned starts=0,resetWrites=0,writes=0,tlbWrites=0,reads=0,pauses=0;
 unsigned dropReset=0,dropWrite=0,dropTlb=0;bool timeout=false,badMarker=false,sourceBad=false;
 uint64_t now=1000000;
 IO(){for(unsigned i=0;i<2;i++){mem[i].resize(0x180000/4);for(unsigned j=0;j<mem[i].size();j++)mem[i][j]=0x9e370000U^(j*71+i);original[i]=mem[i];}}
 bool Match(unsigned s,unsigned x,unsigned y,bool ro){for(unsigned w=0;w<3;w++)if(tlb[s][w]!=BlackholeDramFixedTlbWord(s?0xffa00000:0,x,y,ro,w))return false;return true;}
 int Core(unsigned slot,bool ro){for(unsigned i=0;i<2;i++)if(Match(slot,1,2+i,ro))return int(i);return -1;}
 int Window(){int a=Core(0,false),b=Core(1,false);assert(a>=0 && a==b);return a;}
 uint32_t SourceRead(unsigned i){reads++;assert(i<16);return source[i]^((sourceBad && starts)?1U:0U);}
 uint32_t DramTlbRead(unsigned s,unsigned w){reads++;assert(s<2 && w<3);return tlb[s][w];}
 void DramTlbWrite(unsigned s,unsigned w,uint32_t v){assert(s<2 && w<3);if(++tlbWrites!=dropTlb)tlb[s][w]=v;}
 uint32_t ResetRead(){reads++;int core=Core(1,false);if(core>=0)return reset[core];core=Core(1,true);if(core>=0)return reset[core];
  const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};for(unsigned x:xs)for(unsigned y=2;y<=11;y++)if(Match(1,x,y,true))return 0x47800;assert(false);return 0;}
 uint32_t BfpRead(uint32_t a){int c=Window();assert(!(a&3) && a<0x180000);reads++;return mem[c][a/4];}
 void BfpWrite(uint32_t a,uint32_t v){int c=Window();assert(reset[c]==0x47800 && !(a&3) && a<0x180000);if(++writes!=dropWrite)mem[c][a/4]=v;}
 uint32_t BfpControlRead(unsigned i){assert(Window()>=0 && i<3);return 0;}
 uint32_t BfpPcRead(){assert(Window()>=0);return pauses&1?0x70038:0x7003c;}
 void BfpResetWrite(uint32_t v){int c=Window();if(++resetWrites==dropReset)return;
  if(v==0x46800){assert(reset[c]==0x87ffff);reset[c]=v;starts|=1U<<c;
   if(starts==3 && !timeout)for(unsigned i=0;i<2;i++){
    mem[i][0x17ffc0/4]=0x7000c;mem[i][0x17ffc4/4]=0x04000000|5120;
    mem[i][0x17ffc8/4]=(badMarker && i==1)?0x4542464e:0x3342464e;
    const auto* expected=PioneerNativeBfpModelBandMetadata::Golden(i);
    for(unsigned j=0;j<64;j++)mem[i][(PioneerNativeBfpBandLayout::Output+j*4)/4]=expected[j%32];
   }
  }else{assert(v==0x87ffff || v==0x47800);reset[c]=v;}}
 uint64_t BfpNow(){return now;}
 void Pause(){pauses++;now+=100000;}
 bool BfpHealthy()const{return true;}
 tt_dram_samples_result Baseline(){tt_dram_samples_result b={};b.version=1;b.state=TT_DRAM_SAMPLE_ROUTES_OK;
  b.tlb_restored=b.data_restored=1;b.routes_verified=7;for(unsigned i=0;i<16;i++)b.source[i]=source[i];assert(BlackholeDramBuildRoutes(b));return b;}
};
struct Image {static constexpr unsigned Width=5120,Phases=4;static constexpr uint32_t Boot=0x12345678;
 static uint32_t Word(unsigned n){return 0x10000+n;}};
using Owner=PioneerNativeModelBandPairInspection;
int main(){
 for(unsigned invalid: {3U,UINT32_MAX}){
  IO io;auto baseline=io.Baseline();auto owner=std::make_unique<Owner>();
  assert((!owner->Run<IO,Image>(io,baseline,0,invalid)));
  assert(!io.reads && !io.writes && !io.resetWrites && !io.tlbWrites);
 }
 {IO io;auto baseline=io.Baseline();auto owner=std::make_unique<Owner>();
  assert((owner->Run<IO,Image>(io,baseline,0,1)));assert(io.starts==3 && io.resetWrites==4);
  for(unsigned i=0;i<2;i++){auto& r=owner->Result(i);assert(r.completed && r.state==PIONEER_NATIVE_BFP_RESULT);
   const auto* expected=PioneerNativeBfpModelBandMetadata::Golden(i);for(unsigned j=0;j<64;j++)assert(r.output[j]==expected[j%32]);}
  assert(owner->End(io,baseline));assert(io.resetWrites==8);
  for(unsigned i=0;i<2;i++)assert(io.mem[i]==io.original[i]);
  for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)assert(io.tlb[s][w]==s*3+w+1);
  auto reads=io.reads,writes=io.writes,resets=io.resetWrites,tlbs=io.tlbWrites;
  assert(!owner->End(io,baseline));assert((!owner->Run<IO,Image>(io,baseline,0,1)));
  assert(reads==io.reads && writes==io.writes && resets==io.resetWrites && tlbs==io.tlbWrites);
 }
 for(unsigned fault=0;fault<10;fault++){
  IO io;auto baseline=io.Baseline();auto owner=std::make_unique<Owner>();
  if(fault==0)io.timeout=true;if(fault==1)io.badMarker=true;if(fault==2)io.dropWrite=100;
  if(fault==3)io.dropReset=1;if(fault==4)io.dropReset=3;if(fault==5)io.sourceBad=true;
  bool ran=owner->Run<IO,Image>(io,baseline,0,1);
  if(fault>=6){assert(ran);if(fault==6)io.dropReset=5;if(fault==7)io.dropReset=8;
   if(fault==8)io.dropWrite=io.writes+20;if(fault==9)io.dropTlb=io.tlbWrites+2;assert(!owner->End(io,baseline));}
  else assert(!ran);
  for(unsigned i=0;i<2;i++)assert(owner->Result(i).state==PIONEER_NATIVE_BFP_UNKNOWN && owner->Result(i).cold);
  auto reads=io.reads,writes=io.writes,resets=io.resetWrites,tlbs=io.tlbWrites;
  assert(!owner->End(io,baseline));assert((!owner->Run<IO,Image>(io,baseline,0,1)));
  assert(reads==io.reads && writes==io.writes && resets==io.resetWrites && tlbs==io.tlbWrites);
 }
 puts("PASS_OFFLINE_PAIR_OWNERSHIP concurrent_start injected_outputs exact_restore ten_faults; NO_DEVICE_OR_INFERENCE_SPEED_PROOF");
}
