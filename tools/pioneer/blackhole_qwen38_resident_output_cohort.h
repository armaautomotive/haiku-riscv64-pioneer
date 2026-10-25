/* SPDX-License-Identifier: MIT */
// Included ONLY by the separate coordinator variant, after Cohort helpers.
#include "PioneerQ38ResidentOutputRootImageRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputCohortShape.h"
extern "C" unsigned q38_resident_output_matrix_once();
namespace Cohort {
static const uint32_t* OutputManifest(){return reinterpret_cast<const uint32_t*>(TT_Q38_OUTPUT_COHORT_SHADOW);}
static bool OutputAudit(){
 for(unsigned i=0;i<TT_Q38_OUTPUT_COHORT_WORDS;i++)if(Words(TT_Q38_OUTPUT_COHORT_MANIFEST)[i]!=Words(TT_Q38_OUTPUT_COHORT_SHADOW)[i])return false;
 return Current();
}
static void OutputCapture(){
 for(unsigned i=0;i<TT_Q38_OUTPUT_COHORT_WORDS;i++)Words(TT_Q38_OUTPUT_COHORT_SHADOW)[i]=Words(TT_Q38_OUTPUT_COHORT_MANIFEST)[i];
 if(!PioneerQ38ResidentOutputCohortShape(OutputManifest(),Manifest()) || !OutputAudit())cohort_fault_park();
}
static void OutputJob(unsigned w,uint32_t* j){
 for(unsigned i=0;i<16;i++)j[i]=0;
 j[0]=TT_Q38_RESIDENT_GDN_OUTPUT_MARKER;j[1]=256;j[2]=576;j[3]=OutputManifest()[5];
 j[10]=TT_Q38_FFN_PERSISTENT_MARKER;j[11]=w;j[12]=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;j[13]=6;
}
static void StageOutput(){
 if(!Audit() || !OutputAudit())cohort_fault_park();
 Before(24,6);
 for(unsigned w=1;w<=24;w++){
  uint32_t d[64],j[16];PioneerQ38ResidentOutputDescriptor(Manifest(),OutputManifest(),w,d);OutputJob(w,j);
  if(!PioneerQ38ResidentGdnOutputJob(j,d,Source(w),w,PioneerQ38ConvPeer(w)))cohort_fault_park();
  Write(w,TT_Q38_FFN_RESIDENT_DESCRIPTOR,d,64);Write(w,0x74000,j,16);
 }
 if(!Audit() || !OutputAudit())cohort_fault_park();
 for(unsigned w=1;w<=24;w++){uint32_t sequence=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;Write(w,0x74100,&sequence,1);}
 for(unsigned w=1;w<=24;w++){
  uint32_t d[64],j[16],p[64],actual[8];unsigned sequence=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;
  Wait(w,sequence,6,false);Read(w,0x83f00,p,64);
  PioneerQ38ResidentOutputDescriptor(Manifest(),OutputManifest(),w,d);OutputJob(w,j);
  const auto* before=reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_COUNTERS+(w-1)*32);
  if(!PioneerQ38ResidentGdnOutputProof(p,j,d,Source(w),w,PioneerQ38ConvPeer(w),before,p+8))cohort_fault_park();
  Sources(w);Counters(w,actual,p+8);Words(TT_Q38_COHORT_SEQUENCE)[w-1]=sequence;Words(TT_Q38_COHORT_STATS)[7]++;
 }
 if(!Audit() || !OutputAudit())cohort_fault_park();
}
static void OutputDramPacket(unsigned chunk,bool write,uint32_t* tracked){
 const auto* m=Manifest();
 if((write?chunk!=24:chunk>=24) || !OutputAudit() || !Quiet())cohort_fault_park();
 unsigned local=write?0xab000:0x90e00,remote=m[8]+(write?unsigned(TT_Q38_PROJECTION_COMPACT_OFFSET):TT_Q38_PROJECTION_LINES_OFFSET+chunk*128),bytes=write?768:128;
 const uint32_t command[]={write?local:remote,0,write?129U:m[9],write?remote:local,0,write?m[9]:129U,0,write?0x212U:0,bytes,0};
 if(chunk){
  const uint32_t previous[]={m[8]+TT_Q38_PROJECTION_LINES_OFFSET+(chunk-1)*128,0,m[9],0x90e00,0,129,0,0,128,0};
  for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=previous[i])cohort_fault_park();
 }
 uint32_t after[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])cohort_fault_park();
  unsigned delta=i==2 || i==6?1U:i==0 || i==3?unsigned(!write):i==1?(!write?bytes/64:0U):i==5?(write?bytes/64:0U):unsigned(write);
  after[i]=tracked[i]+delta;if(i==6 && after[6]!=after[2])cohort_fault_park();
 }
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==after[i];
  if(done){
   if(!OutputAudit() || !Quiet())cohort_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=after[i];
   return;
  }
 }
 cohort_fault_park();
}
static void GatherOutput(){
 uint32_t tracked[8],before[8];
 if(!OutputAudit() || !Quiet())cohort_fault_park();
 for(unsigned i=0;i<8;i++){before[i]=tracked[i]=*Words(0xffb20000+Counter(i));if(i==6 && before[6]!=before[2])cohort_fault_park();}
 for(unsigned chunk=0;chunk<24;chunk++){
  for(unsigned i=0;i<32;i++)Words(0x90e00)[i]=0x7fca0000+chunk*32+i;
  for(unsigned i=0;i<32;i++)if(Words(0x90e00)[i]!=0x7fca0000+chunk*32+i)cohort_fault_park();
  OutputDramPacket(chunk,false,tracked);uint32_t line[32],d[64];
  for(unsigned i=0;i<32;i++)line[i]=Words(0x90e00)[i];
  PioneerQ38ResidentOutputDescriptor(Manifest(),OutputManifest(),chunk+1,d);
  if(!PioneerQ38ResidentGdnOutputLine(line,d))cohort_fault_park();
  for(unsigned i=0;i<8;i++)Words(0xab000)[chunk*8+i]=line[i];
  Words(TT_Q38_COHORT_STATS)[8]++;
 }
 // Publish compact scales only after ALL24 fresh QOL1 lines passed. Native
 // operands are fetched ONCE by the subsequent actual matrix core, not
 // redundantly read here and fetched a second time.
 OutputDramPacket(24,true,tracked);
 const unsigned delta[]={24,48,25,24,1,12,25,1};
 for(unsigned i=0;i<8;i++)if(*Words(0xffb20000+Counter(i))!=tracked[i] || uint32_t(tracked[i]-before[i])!=delta[i])cohort_fault_park();
 if(!Audit() || !OutputAudit() || !Quiet())cohort_fault_park();
}
static void StageOutputMatrix(){
 Before(119,1);const auto* m=Manifest();const auto* q=OutputManifest();uint32_t d[64]={};
 d[0]=TT_Q38_PROJECTION_MATRIX_MARKER;d[1]=1;d[2]=67108864;d[3]=m[7];d[4]=m[8];d[6]=q[7];d[7]=119;
 d[8]=m[9];d[9]=129;d[10]=TT_Q38_PROJECTION_OUTPUT_OFFSET;d[11]=5120;d[12]=20480;d[13]=TT_Q38_PROJECTION_MATRIX_ROWS;d[14]=6144;d[15]=8;
 for(unsigned i=16;i<40;i++)d[i]=m[i];
 d[40]=m[4];d[41]=q[5];d[42]=m[3];d[43]=13;d[44]=119;d[45]=1;
 for(unsigned i=0;i<64;i++)Words(TT_Q38_PROJECTION_MATRIX_FRAME)[i]=d[i];
 const uint32_t h[]={TT_Q38_PROJECTION_MATRIX_MARKER,1,1,8,6144,119,q[7],TT_Q38_PROJECTION_MATRIX_INPUT,1,0,0,0,0,0,0,0};
 for(unsigned i=0;i<16;i++)Words(0x74000)[i]=h[i];
 for(unsigned i=0;i<8;i++)Words(0x74040)[i]=i<7?PioneerQ38ProjectionRoute(i):0;
 for(unsigned w=0;w<119;w++){
  for(unsigned i=0;i<8;i++)Words(0x74080)[w*8+i]=i==3?Words(TT_Q38_COHORT_SEQUENCE)[w]+1:q[64+w*8+i];
  Words(TT_Q38_PROJECTION_MATRIX_ROWS)[w]=q[1016+w];
  for(unsigned i=0;i<16;i++)Words(0x7d800)[w*16+i]=Source(w+1)[i];
 }
 if(!Audit() || !OutputAudit() || !Quiet())cohort_fault_park();
 if(q38_resident_output_matrix_once()!=119 || !Audit() || !OutputAudit() || !Quiet())cohort_fault_park();
 if(Words(0x7f800)[49]!=TT_Q38_PROJECTION_MATRIX_MARKER || Words(0x7f800)[55]!=5120 || Words(0x7f800)[56]!=1)cohort_fault_park();
 for(unsigned w=1;w<=119;w++){
  const auto* proof=reinterpret_cast<const uint32_t*>(0x76000+(w-1)*256);
  const auto* before=reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_COUNTERS+(w-1)*32);uint32_t actual[8];
  for(unsigned i=0;i<8;i++)if(proof[i]!=before[i])cohort_fault_park();
  Wait(w,Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1,1,false);Sources(w);Counters(w,actual,proof+8);
  Words(TT_Q38_COHORT_SEQUENCE)[w-1]++;
 }
 Words(TT_Q38_COHORT_STATS)[9]=5120;
 if(!Audit() || !OutputAudit())cohort_fault_park();
}
}
