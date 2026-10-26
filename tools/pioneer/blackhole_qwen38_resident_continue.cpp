/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ResidentContinueImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
typedef __UINT64_TYPE__ uint64_t;
#define UINT32_MAX 0xffffffffU
#define UINT64_C(value) value##ULL
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentContinueShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerProof.h"
extern "C" void cohort_fault_park() __attribute__((noreturn));
extern "C" unsigned q38_resident_advance_main();
extern "C" unsigned q38_resident_cohort_main();
namespace Continue {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static const uint32_t* Data(unsigned a){return reinterpret_cast<const uint32_t*>(a);}
static const uint32_t* Header(){return Data(TT_Q38_CONTINUE_SHADOW);}
static volatile uint32_t* Receipt(){return Words(TT_Q38_CONTINUE_RECEIPT);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Audit(){
 for(unsigned i=0;i<TT_Q38_CONTINUE_WORDS;i++)if(Words(TT_Q38_CONTINUE_MANIFEST)[i]!=Header()[i])cohort_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=Header()[16+i])cohort_fault_park();
 if(!Quiet())cohort_fault_park();
}
static void Copy(unsigned destination,unsigned source,unsigned words){for(unsigned i=0;i<words;i++)Words(destination)[i]=Words(source)[i];}
static void Save(unsigned word,unsigned source,unsigned words){Copy(TT_Q38_CONTINUE_RECEIPT+word*4,source,words);}
static void Read(unsigned route,unsigned remote,unsigned local,unsigned bytes,uint32_t* tracked){
 Audit();uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])cohort_fault_park();
  unsigned delta=i==1?bytes/64+unsigned((bytes&63)!=0):i==0 || i==2 || i==3 || i==6?1U:0U;
  expected[i]=tracked[i]+delta;if(i==6 && expected[6]!=expected[2])cohort_fault_park();
 }
 const uint32_t command[]={remote,0,route,local,0,129,0,0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){
   Audit();for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   Receipt()[1666]++;Receipt()[1667]+=(bytes+63)/64;return;
  }
 }
 cohort_fault_park(); // UNKNOWN: no next packet/reset/rollback/STOP/retry
}
static void Peer(unsigned w,unsigned remote,unsigned bytes,uint32_t* tracked){
 bool csr=remote==0xffb13140 || remote==0xffb121b0;
 for(unsigned i=0;i<16;i++)csr |= remote==0xffb20000+Source(i);
 for(unsigned i=0;i<8;i++)csr |= remote==0xffb20000+Counter(i);
 if(!w || w>119 || !((csr && bytes==4) || (remote==0x74140 && bytes==64)))cohort_fault_park();
 Read(PioneerQ38ConvPeer(w),remote,0x75000+(remote&31),bytes,tracked);
}
static unsigned PeerWord(unsigned remote){return *Words(0x75000+(remote&31));}
static void AuditPeers(uint32_t* tracked){
 const auto* old=Data(TT_Q38_COHORT_SHADOW);
 for(unsigned w=1;w<=119;w++){
  unsigned seq=Receipt()[992+w-1];const auto* proof=Data(0x76000+(w-1)*256);
  if(proof[17]!=seq || proof[18]!=w || proof[19]!=TT_Q38_FFN_PERSISTENT_MARKER || proof[31]!=PioneerQ38ConvPeer(w)
   || Words(TT_Q38_COHORT_SEQUENCE)[w-1]!=seq)cohort_fault_park();
  Peer(w,0x74140,64,tracked);
  for(unsigned i=0;i<16;i++)if(Words(0x75000)[i]!=(i<2?seq:i==3?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):0))cohort_fault_park();
  for(unsigned i=0;i<2;i++){
   Peer(w,0xffb13140,4,tracked);unsigned pc=PeerWord(0xffb13140);if(pc!=0x7204c && pc!=0x72050)cohort_fault_park();
  }
  Peer(w,0xffb121b0,4,tracked);if(PeerWord(0xffb121b0)!=0x46800)cohort_fault_park();
  for(unsigned i=0;i<16;i++){
   if(proof[32+i]!=PioneerQ38CohortSource(old,w)[i])cohort_fault_park();
   Peer(w,0xffb20000+Source(i),4,tracked);if(PeerWord(0xffb20000+Source(i))!=proof[32+i])cohort_fault_park();
  }
  for(unsigned i=0;i<8;i++){
   Peer(w,0xffb20000+Counter(i),4,tracked);if(PeerWord(0xffb20000+Counter(i))!=proof[8+i])cohort_fault_park();
  }
  Receipt()[1664]++;
 }
}
static void ValidateRecipe(bool bound){
 if(!PioneerQ38ResidentContinueRecipe(Header(),Data(TT_Q38_COHORT_MANIFEST),Data(TT_Q38_OUTPUT_COHORT_MANIFEST),
  Data(TT_Q38_RESIDENT_LAYER_MANIFEST),Data(TT_Q38_COHORT_SHADOW),bound?Data(TT_Q38_CONTINUE_RECEIPT+992*4):nullptr))cohort_fault_park();
}
}
extern "C" void q38_resident_continue_capture(){
 using namespace Continue;
 Copy(TT_Q38_CONTINUE_SHADOW,TT_Q38_CONTINUE_MANIFEST,TT_Q38_CONTINUE_WORDS);Audit();
 if(!PioneerQ38ResidentContinueShape(Header(),Data(TT_Q38_ADVANCE_MANIFEST),Data(TT_Q38_COHORT_MANIFEST)))cohort_fault_park();
 for(unsigned i=0;i<TT_Q38_CONTINUE_RECEIPT_WORDS;i++)Receipt()[i]=0;
}
extern "C" unsigned q38_resident_continue_load_recipe(){
 // Device-internal subroutine. Its caller MUST first prove the actual
 // predecessor. Direct instruction tests provide DATA only,not that proof.
 using namespace Continue;Audit();uint32_t tracked[8];
 for(unsigned i=0;i<8;i++){tracked[i]=*Words(0xffb20000+Counter(i));Receipt()[1112+i]=tracked[i];if(i==6 && tracked[6]!=tracked[2])cohort_fault_park();}
 AuditPeers(tracked);
 unsigned sourceOffset=0;
 for(unsigned section=0;section<3;section++){
  unsigned bytes=PioneerQ38ResidentContinueWords(section)*4,target=PioneerQ38ResidentContinueTarget(section);
  for(unsigned offset=0;offset<bytes;){
   unsigned length=bytes-offset;if(length>16384)length=16384;
   Read(Header()[8],Header()[7]+TT_Q38_CONTINUE_RECIPE_OFFSET+sourceOffset+offset,target+offset,length,tracked);
   offset+=length;Receipt()[1665]++;
  }
  sourceOffset+=bytes;
 }
 Audit();ValidateRecipe(false);
 for(unsigned w=0;w<119;w++){
  unsigned actual=Receipt()[992+w];
  if(actual!=Words(TT_Q38_COHORT_SEQUENCE)[w])cohort_fault_park();
  Words(TT_Q38_COHORT_MANIFEST)[64+w]=actual;
 }
 ValidateRecipe(true);Audit();
 for(unsigned i=0;i<8;i++){
  // Each of28 peer reads uses ONE flit (status64B, other CSR reads4B).
  // The fixed285184B recipe adds4456 flits in19 packets:7788 total.
  unsigned delta=i==1?7788:i==0 || i==2 || i==3 || i==6?3351U:0U;
  if(uint32_t(tracked[i]-Receipt()[1112+i])!=delta || tracked[i]!=*Words(0xffb20000+Counter(i)))cohort_fault_park();
  Receipt()[1120+i]=tracked[i];
 }
 if(Receipt()[1664]!=119 || Receipt()[1665]!=19 || Receipt()[1666]!=3351 || Receipt()[1667]!=7788)cohort_fault_park();
 return 119; // actual sequences admitted,NOT a completed second layer
}
extern "C" unsigned q38_resident_continue_once(){
 using namespace Continue;Audit();
 const auto* a=Data(TT_Q38_ADVANCE_SHADOW);const auto* old=Data(TT_Q38_COHORT_SHADOW);const auto* done=Data(TT_Q38_ADVANCE_RECEIPT);
 uint32_t after[8],command[10];for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<10;i++)command[i]=Words(0xffb20000)[i];
 if(!PioneerQ38ResidentContinueShape(Header(),a,old)
  || !PioneerQ38ResidentAdvanceReceipt(done,a,Data(TT_Q38_LAYER_LINK_SHADOW),old,done+352,after,command))cohort_fault_park();
 Save(128,TT_Q38_ADVANCE_RECEIPT,512);Save(640,TT_Q38_LAYER_LINK_RECEIPT,96);Save(736,TT_Q38_RESIDENT_LAYER_RECEIPT,256);
 for(unsigned w=0;w<119;w++){
  unsigned actual=Words(TT_Q38_COHORT_SEQUENCE)[w];if(actual!=done[378+w])cohort_fault_park();Receipt()[992+w]=actual;
 }
 if(q38_resident_continue_load_recipe()!=119)cohort_fault_park();
 // The old entry now reloads the SAME actual private sequences admitted
 // above,NOT host zero placeholders or restarted workers. All recipe
 // transfer and binding happened ON CARD after its own actual advance.
 if(q38_resident_cohort_main()!=5120)cohort_fault_park();
 Audit();for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<10;i++)command[i]=Words(0xffb20000)[i];
 if(!PioneerQ38ResidentLayerRootReceipt(Data(TT_Q38_RESIDENT_LAYER_SHADOW),Data(TT_Q38_COHORT_SHADOW),Data(TT_Q38_OUTPUT_COHORT_SHADOW),
  Data(TT_Q38_RESIDENT_LAYER_RECEIPT),Data(TT_Q38_COHORT_STATS),Data(TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST),Data(TT_Q38_FFN_RESIDUAL_GRAPH_STATS),
  Data(TT_Q38_CONTINUE_RECEIPT+1120*4),after,command))cohort_fault_park();
 for(unsigned w=1;w<=119;w++){
  unsigned actual=Words(TT_Q38_COHORT_SEQUENCE)[w-1];
  if(actual!=PioneerQ38ResidentLayerSequence(Data(TT_Q38_COHORT_SHADOW),w))cohort_fault_park();
  Receipt()[1152+w-1]=actual;
 }
 for(unsigned i=0;i<8;i++)Receipt()[1128+i]=after[i];
 for(unsigned i=0;i<10;i++)Receipt()[1136+i]=command[i];
 Save(1280,TT_Q38_RESIDENT_LAYER_RECEIPT,256);Save(1536,TT_Q38_COHORT_STATS,64);Save(1600,TT_Q38_FFN_RESIDUAL_GRAPH_STATS,64);
 for(unsigned i=0;i<TT_Q38_CONTINUE_WORDS;i++)Receipt()[i]=i==0?0:Header()[i];
 Fence();Receipt()[0]=TT_Q38_CONTINUE_RECEIPT_MARKER;Fence();return 5120;
}
extern "C" unsigned q38_resident_continue_main(){
 q38_resident_continue_capture();
 if(q38_resident_advance_main()!=5120)cohort_fault_park();
 return q38_resident_continue_once();
}
