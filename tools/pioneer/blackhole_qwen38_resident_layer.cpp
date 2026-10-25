/* SPDX-License-Identifier: MIT */
// New complete recurrent-layer coordinator. All intermediate arithmetic
// stays on the card. Native ownership/initialization/physical qualification
// are still REQUIRED;this emitted program never grants those capabilities.
#include "PioneerQ38ResidentLayerRootImageRole.h"
#define q38_resident_cohort_main q38_resident_layer_prefix_once
#include "blackhole_qwen38_resident_cohort.cpp"
#undef q38_resident_cohort_main
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerShape.h"
extern "C" unsigned q38_resident_layer_norm_once();
extern "C" unsigned q38_resident_layer_ffn_once();
namespace Layer {
using namespace Cohort;
static const uint32_t* Snapshot(){return reinterpret_cast<const uint32_t*>(TT_Q38_RESIDENT_LAYER_SHADOW);}
static volatile uint32_t* Receipt(){return Words(TT_Q38_RESIDENT_LAYER_RECEIPT);}
static bool Sealed(){
 for(unsigned i=0;i<TT_Q38_RESIDENT_LAYER_WORDS;i++)if(Words(TT_Q38_RESIDENT_LAYER_MANIFEST)[i]!=Words(TT_Q38_RESIDENT_LAYER_SHADOW)[i])return false;
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+SourceOffset(i))!=Snapshot()[16+i])return false;
 return Quiet();
}
static void Capture(){
 for(unsigned i=0;i<TT_Q38_RESIDENT_LAYER_WORDS;i++)Words(TT_Q38_RESIDENT_LAYER_SHADOW)[i]=Words(TT_Q38_RESIDENT_LAYER_MANIFEST)[i];
 if(!PioneerQ38ResidentLayerShape(Snapshot(),reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_MANIFEST),
  reinterpret_cast<const uint32_t*>(TT_Q38_OUTPUT_COHORT_MANIFEST)) || !Sealed())cohort_fault_park();
 for(unsigned i=0;i<TT_Q38_RESIDENT_LAYER_RECEIPT_WORDS;i++)Receipt()[i]=0;
 for(unsigned i=0;i<TT_Q38_RESIDENT_LAYER_GDN_SAVED_WORDS;i++)Words(TT_Q38_RESIDENT_LAYER_GDN_SAVED)[i]=0;
 for(unsigned i=0;i<8;i++){
  Receipt()[232+i]=*Words(0xffb20000+Counter(i));
  if(i==6 && Receipt()[238]!=Receipt()[234])cohort_fault_park();
 }
}
static void Audit(){if(!Sealed() || !Cohort::Audit() || !OutputAudit())cohort_fault_park();}
static void SavePrefix(){
 Audit();if(Words(TT_Q38_COHORT_STATS)[9]!=5120)cohort_fault_park();
 // Preserve ACTUAL independent-prefix receipts before FFN reuses7f800.
 // No zero reconstruction,synthetic completion or host callback occurs.
 for(unsigned i=0;i<64;i++){
  Receipt()[32+i]=Words(TT_Q38_COHORT_STATS)[i];Receipt()[96+i]=Words(0x7f800)[i];
 }
 for(unsigned i=0;i<8;i++){
  Receipt()[160+i]=*Words(0xffb20000+Counter(i));
  if(i==6 && Receipt()[166]!=Receipt()[162])cohort_fault_park();
 }
 for(unsigned i=0;i<10;i++)Receipt()[168+i]=Words(0xffb20000)[i];
 Receipt()[178]=68;Receipt()[179]=32;Receipt()[180]=TT_Q38_RESIDENT_LAYER_GDN_SAVED;
}
static void Normalize(){
 Audit();if(Words(TT_Q38_COHORT_STATS)[9]!=5120)cohort_fault_park();
 uint32_t d[64],before[8];PioneerQ38ResidentLayerNormDescriptor(Snapshot(),d);
 if(!PioneerQ38ResidentLayerNormShape(d))cohort_fault_park();
 for(unsigned i=0;i<64;i++)Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]=d[i];
 for(unsigned i=0;i<8;i++){before[i]=*Words(0xffb20000+Counter(i));if(i==6 && before[6]!=before[2])cohort_fault_park();}
 if(q38_resident_layer_norm_once()!=5120)cohort_fault_park();
 Audit();const auto* s=reinterpret_cast<const uint32_t*>(TT_Q38_FFN_RESIDENT_SUMMARY);
 const uint32_t header[]={6,960,4,640,5120,20480,d[6],TT_Q38_RESIDENT_LAYER_NORM_MARKER};
 const unsigned delta[]={6,960,10,6,4,640,10,4};
 for(unsigned i=0;i<8;i++)if(s[i]!=header[i] || s[8+i]!=before[i] || s[16+i]!=*Words(0xffb20000+Counter(i))
  || uint32_t(s[16+i]-before[i])!=delta[i])cohort_fault_park();
 const uint32_t receipt[]={d[40],d[4],d[41],d[50],d[10],d[11],d[53],98304};
 for(unsigned i=0;i<8;i++)if(s[24+i]!=receipt[i])cohort_fault_park();
 for(unsigned i=0;i<32;i++)Receipt()[192+i]=s[i];
 Words(TT_Q38_COHORT_STATS)[10]=5120;Words(TT_Q38_COHORT_STATS)[11]=10;
}
static void Ffn(){
 Audit();const auto* l=Snapshot();
 for(unsigned i=0;i<TT_Q38_FFN_RESIDUAL_GRAPH_WORDS;i++)Words(TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST)[i]=l[64+i];
 for(unsigned w=0;w<119;w++){
  Words(TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST)[64+w]=Words(TT_Q38_COHORT_SEQUENCE)[w];
  for(unsigned i=0;i<16;i++)if(Words(0x7d800)[w*16+i]!=Source(w+1)[i])cohort_fault_park();
 }
 const auto* f=reinterpret_cast<const uint32_t*>(TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST);
 if(!PioneerQ38FfnResidualGraphManifest(f,Source(1)))cohort_fault_park();
 if(q38_resident_layer_ffn_once()!=5120)cohort_fault_park();
 Audit();const auto* s=reinterpret_cast<const uint32_t*>(TT_Q38_FFN_RESIDUAL_GRAPH_STATS);
 if(s[0]!=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER || s[1]!=3 || s[2]!=1 || s[49]!=9 || s[50]!=3 || s[51]!=20 || s[52]!=68
  || s[53]!=20480 || s[54]!=f[13] || s[55]!=l[3] || s[56]!=1 || s[57]!=0x33524651
  || s[58]!=20480 || s[59]!=4 || s[60]!=2 || s[61]!=f[13] || s[62]!=l[4] || s[63]!=l[11])cohort_fault_park();
 for(unsigned w=0;w<119;w++){
  unsigned actual=Words(TT_Q38_FFN_RESIDUAL_GRAPH_SEQUENCE)[w];
  if(actual!=PioneerQ38FfnResidualGraphNextPeer(f,w))cohort_fault_park();
  Words(TT_Q38_COHORT_SEQUENCE)[w]=actual;
 }
 Words(TT_Q38_COHORT_STATS)[12]=5120;Words(TT_Q38_COHORT_STATS)[13]=3;
 Words(TT_Q38_COHORT_STATS)[14]=68;Words(TT_Q38_COHORT_STATS)[15]=20;Words(TT_Q38_COHORT_STATS)[16]=9;
 Words(TT_Q38_COHORT_STATS)[17]=TT_Q38_RESIDENT_LAYER_MARKER;
}
static void PublishReceipt(){
 Audit();const auto* l=Snapshot();
 const uint32_t header[]={0,2,TT_Q38_RESIDENT_LAYER_RECEIPT_WORDS,l[3],l[4],l[11],l[5],l[6],l[7],l[8],l[9],l[10],TT_Q38_RESIDENT_LAYER_MARKER,5120,3,1};
 for(unsigned i=0;i<16;i++)Receipt()[i]=header[i];
 for(unsigned i=0;i<16;i++)Receipt()[16+i]=l[16+i];
 for(unsigned i=0;i<8;i++)Receipt()[224+i]=l[32+i];
 Fence();Receipt()[0]=TT_Q38_RESIDENT_LAYER_RECEIPT_MARKER;Fence();
 // This is completion DATA ONLY,not model-token/current-slot/native authority.
}
}
extern "C" unsigned q38_resident_cohort_main(){
 Layer::Capture();
 if(q38_resident_layer_prefix_once()!=5120)cohort_fault_park();
 Layer::SavePrefix();Layer::Normalize();Layer::Ffn();Layer::PublishReceipt();
 return 5120; // final hidden ONLY;current-slot/model-token commit remains the native owner's responsibility
}
