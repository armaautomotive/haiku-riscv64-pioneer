/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ResidentLayerLinkImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38LearnedRms.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerLinkShape.h"
extern "C" void cohort_fault_park() __attribute__((noreturn));
extern "C" unsigned q38_resident_cohort_main();
namespace Link {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static const uint32_t* Snapshot(){return reinterpret_cast<const uint32_t*>(TT_Q38_LAYER_LINK_SHADOW);}
static const uint32_t* Parent(){return reinterpret_cast<const uint32_t*>(TT_Q38_RESIDENT_LAYER_SHADOW);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static bool Seal(bool parent){
 for(unsigned i=0;i<TT_Q38_LAYER_LINK_WORDS;i++)if(Words(TT_Q38_LAYER_LINK_MANIFEST)[i]!=Snapshot()[i])return false;
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=Snapshot()[16+i])return false;
 if(parent)for(unsigned i=0;i<TT_Q38_RESIDENT_LAYER_WORDS;i++)if(Words(TT_Q38_RESIDENT_LAYER_MANIFEST)[i]!=Parent()[i])return false;
 return Quiet();
}
static void Packet(unsigned index,uint32_t* tracked){
 if(index>=8 || !Seal(true))cohort_fault_park();
 const auto* n=Snapshot()+64;bool write=index>=4,learned=index>=2 && index<4,normalized=index>=6;
 unsigned offset=(index&1)*16384,bytes=index&1?4096:16384;
 unsigned local=(write?(normalized?0x90000:0x80000):learned?0x88000:0x80000)+offset;
 unsigned remote=(learned?n[11]:n[4])+offset+(write && !normalized?TT_Q38_RMS_BACKUP:0);
 unsigned route=learned?n[12]:n[8];
 uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])cohort_fault_park();
  unsigned delta=i==2 || i==6?1U:i==0 || i==3?unsigned(!write):i==1?(!write?bytes/64:0U):i==5?(write?bytes/64:0U):unsigned(write);
  expected[i]=tracked[i]+delta;if(i==6 && expected[6]!=expected[2])cohort_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){
   if(!Seal(true))cohort_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   return;
  }
 }
 cohort_fault_park(); // UNKNOWN: no next packet, peer work, publication, reset or retry
}
}
extern "C" void q38_resident_layer_link_capture(){
 using namespace Link;
 for(unsigned i=0;i<TT_Q38_LAYER_LINK_WORDS;i++)Words(TT_Q38_LAYER_LINK_SHADOW)[i]=Words(TT_Q38_LAYER_LINK_MANIFEST)[i];
 if(!PioneerQ38ResidentLayerLinkShape(Snapshot(),reinterpret_cast<const uint32_t*>(TT_Q38_RESIDENT_LAYER_MANIFEST)) || !Seal(false))cohort_fault_park();
 for(unsigned i=0;i<TT_Q38_LAYER_LINK_RECEIPT_WORDS;i++)Words(TT_Q38_LAYER_LINK_RECEIPT)[i]=0;
}
extern "C" unsigned q38_resident_layer_link_once(){
 using namespace Link;const auto* d=Snapshot();const auto* n=d+64;
 if(!Seal(true) || !PioneerQ38ResidentLayerLinkShape(d,Parent()))cohort_fault_park();
 // Completion is established by the preceding actual layer call, not
 // these DATA fields alone. Still reject changed predecessor evidence
 // before the FIRST handoff packet; preserve its original receipts.
 const auto* r=reinterpret_cast<const uint32_t*>(TT_Q38_RESIDENT_LAYER_RECEIPT);
 const uint32_t header[]={TT_Q38_RESIDENT_LAYER_RECEIPT_MARKER,2,256,d[3],d[5],d[6],Parent()[5],Parent()[6],Parent()[7],d[7],d[8],n[8],TT_Q38_RESIDENT_LAYER_MARKER,5120,3,1};
 for(unsigned i=0;i<16;i++)if(r[i]!=header[i] || r[16+i]!=d[16+i])cohort_fault_park();
 for(unsigned i=0;i<8;i++)if(r[224+i]!=d[32+i])cohort_fault_park();
 const auto* s=reinterpret_cast<const uint32_t*>(TT_Q38_FFN_RESIDUAL_GRAPH_STATS);
 if(s[0]!=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER || s[1]!=3 || s[2]!=1 || s[49]!=9 || s[50]!=3 || s[51]!=20 || s[52]!=68
  || s[53]!=20480 || s[54]!=d[9] || s[55]!=d[3] || s[56]!=1 || s[57]!=0x33524651
  || s[58]!=20480 || s[59]!=4 || s[60]!=2 || s[61]!=d[9] || s[62]!=d[5] || s[63]!=d[6])cohort_fault_park();
 uint32_t before[8],tracked[8];
 for(unsigned i=0;i<8;i++){
  before[i]=tracked[i]=*Words(0xffb20000+Counter(i));
  if(before[i]!=s[24+i] || (i==6 && before[6]!=before[2]))cohort_fault_park();
 }
 const uint32_t previous[]={0x9d000,0,129,d[8]+16384,0,n[8],0,0x212,4096,0};
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=previous[i])cohort_fault_park();
 for(unsigned i=0;i<4;i++)Packet(i,tracked);
 if(!PioneerQ38LearnedRms(reinterpret_cast<const float*>(0x80000),reinterpret_cast<const float*>(0x88000),reinterpret_cast<float*>(0x90000)))cohort_fault_park();
 // Both complete vectors remain in GDDR; no host callback or numeric
 // validation. ALL math/domain checks precede FIRST result publication.
 for(unsigned i=4;i<8;i++)Packet(i,tracked);
 if(!Seal(true))cohort_fault_park();
 volatile uint32_t* out=Words(TT_Q38_LAYER_LINK_RECEIPT);
 for(unsigned i=0;i<40;i++)out[i]=i==0?0:i==2?unsigned(TT_Q38_LAYER_LINK_RECEIPT_WORDS):d[i];
 const uint32_t summary[]={4,640,4,640,5120,20480,n[6],TT_Q38_RMS_MARKER};
 for(unsigned i=0;i<8;i++){out[40+i]=summary[i];out[48+i]=before[i];out[56+i]=tracked[i];}
 const uint32_t fields[]={n[3],n[4],n[5],n[7],n[10],n[11],n[40],TT_Q38_RMS_BACKUP};
 for(unsigned i=0;i<8;i++)out[64+i]=fields[i];
 for(unsigned i=0;i<10;i++)out[72+i]=Words(0xffb20000)[i];
 Fence();out[0]=TT_Q38_LAYER_LINK_RECEIPT_MARKER;Fence();return 5120;
}
extern "C" unsigned q38_resident_layer_link_main(){
 // One uninterrupted on-card call chain. The qualified layer object is
 // linked WITHOUT modifying/recompiling/re-tagging any of its code/data.
 // No native CLOSE/relaunch, old producer alias or CPU feedback occurs.
 q38_resident_layer_link_capture();
 if(q38_resident_cohort_main()!=5120)cohort_fault_park();
 return q38_resident_layer_link_once();
}
