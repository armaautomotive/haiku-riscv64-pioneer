/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ResidentAdvanceImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentAdvanceShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGatherShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38NextProjectionMatrixShape.h"
extern "C" void cohort_fault_park() __attribute__((noreturn));
extern "C" unsigned q38_resident_layer_link_main();
extern "C" unsigned q38_resident_advance_matrix_once();
extern "C" unsigned q38_resident_advance_gather_once();
namespace Advance {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static const uint32_t* Snapshot(){return reinterpret_cast<const uint32_t*>(TT_Q38_ADVANCE_SHADOW);}
static const uint32_t* Cohort(){return reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_SHADOW);}
static const uint32_t* Source(unsigned w){return PioneerQ38CohortSource(Cohort(),w);}
static volatile uint32_t* Receipt(){return Words(TT_Q38_ADVANCE_RECEIPT);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static unsigned SourceOffset(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Current(){for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+SourceOffset(i))!=Snapshot()[16+i])return false;return true;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Audit(){
 if(!Current() || !Quiet())cohort_fault_park();
 for(unsigned i=0;i<TT_Q38_ADVANCE_WORDS;i++)if(Words(TT_Q38_ADVANCE_MANIFEST)[i]!=Snapshot()[i])cohort_fault_park();
 for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)if(Words(TT_Q38_COHORT_MANIFEST)[i]!=Cohort()[i])cohort_fault_park();
 for(unsigned i=0;i<TT_Q38_LAYER_LINK_WORDS;i++)if(Words(TT_Q38_LAYER_LINK_MANIFEST)[i]!=Words(TT_Q38_LAYER_LINK_SHADOW)[i])cohort_fault_park();
} // complete immutable audit at phase boundaries; exclusive native lifetime still OWED
static bool Csr(unsigned at){
 if(at==0xffb13140 || at==0xffb121b0)return true;
 for(unsigned i=0;i<16;i++)if(at==0xffb20000+SourceOffset(i))return true;
 for(unsigned i=0;i<8;i++)if(at==0xffb20000+Counter(i))return true;
 return false;
}
static void Packet(bool write,unsigned w,unsigned local,unsigned remote,unsigned bytes){
 bool permit=write?(remote==0x74000 && bytes==64) || (remote==0x74200 && bytes==256) || (remote==0x74100 && bytes==4)
  :(remote==0x74140 && bytes==64) || (remote==0x83f00 && bytes==256) || (Csr(remote) && bytes==4);
 if(!permit || !w || w>119 || local<0x75000 || bytes>2048 || local>0x75800-bytes || ((local^remote)&31)
  || !Current() || !Quiet())cohort_fault_park();
 uint32_t expected[8];for(unsigned i=0;i<8;i++){
  unsigned delta=i==2 || i==6?1U:i==0 || i==3?unsigned(!write):i==1?(!write?(bytes+63)/64:0U):i==5?(write?(bytes+63)/64:0U):unsigned(write);
  expected[i]=*Words(0xffb20000+Counter(i))+delta;if(i==6 && expected[6]!=expected[2])cohort_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:PioneerQ38ConvPeer(w),write?remote:local,0,write?PioneerQ38ConvPeer(w):129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){
   if(!Current() || !Quiet())cohort_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
   Receipt()[write?45:44]++;Receipt()[write?47:46]+=(bytes+63)/64;return;
  }
 }
 cohort_fault_park(); // UNKNOWN: no following packet/reset/STOP/restore/retry
}
static void Read(unsigned w,unsigned at,uint32_t* out,unsigned words){
 unsigned local=0x75000+(at&31);Packet(false,w,local,at,words*4);for(unsigned i=0;i<words;i++)out[i]=Words(local)[i];
}
static void Write(unsigned w,unsigned at,const uint32_t* data,unsigned words){
 unsigned local=0x75000+(at&31);for(unsigned i=0;i<words;i++)Words(local)[i]=data[i];Packet(true,w,local,at,words*4);
}
static void Sources(unsigned w){for(unsigned i=0;i<16;i++){uint32_t v;Read(w,0xffb20000+SourceOffset(i),&v,1);if(v!=Source(w)[i])cohort_fault_park();}}
static void Counters(unsigned w,uint32_t* out,const uint32_t* expected){
 for(unsigned i=0;i<8;i++){
  Read(w,0xffb20000+Counter(i),out+i,1);if((i==6 && out[6]!=out[2]) || (expected && out[i]!=expected[i]))cohort_fault_park();
 }
}
static void Wait(unsigned w,unsigned sequence,unsigned operation,bool previous){
 unsigned backoff=256;
 for(unsigned poll=0;poll<65536;poll++){
  uint32_t s[16];Read(w,0x74140,s,16);Receipt()[48]++;
  unsigned old=previous?sequence:sequence-1;
  if(s[2] || s[3]!=TT_Q38_FFN_PERSISTENT_MARKER || s[0]<old || s[0]>sequence || s[1]<old || s[1]>s[0]
   || (s[4] && s[4]!=operation))cohort_fault_park();
  for(unsigned i=5;i<16;i++)if(s[i])cohort_fault_park();
  if(s[0]==sequence && s[1]==sequence && !s[4]){
   uint32_t pc[2],reset;for(unsigned i=0;i<2;i++){
    Read(w,0xffb13140,pc+i,1);
    if((pc[i]&3) || !((pc[i]>=0x64000 && pc[i]<0x6e000) || (pc[i]>=0x70b00 && pc[i]<0x74000)
      || (pc[i]>=0xb8000 && pc[i]<0xc8000)) || pc[i]==0x72060 || pc[i]==0x7207c)cohort_fault_park();
   }
   Read(w,0xffb121b0,&reset,1);if(reset!=0x46800)cohort_fault_park();
   if((pc[0]==0x7204c || pc[0]==0x72050) && (pc[1]==0x7204c || pc[1]==0x72050)){Receipt()[49]++;return;}
  }
  for(unsigned idle=0;idle<backoff;idle++)asm volatile("nop");
  if(backoff<4096)backoff<<=1;
 }
 cohort_fault_park();
}
static void Before(unsigned count,unsigned operation){
 Audit();
 for(unsigned w=1;w<=count;w++){
  Wait(w,Words(TT_Q38_COHORT_SEQUENCE)[w-1],operation,true);Sources(w);uint32_t before[8];Counters(w,before,nullptr);
  for(unsigned i=0;i<8;i++)Words(TT_Q38_COHORT_COUNTERS+(w-1)*32)[i]=before[i];
 }
}
static void Input(unsigned w,uint32_t* d,uint32_t* j){
 const auto* m=Snapshot();for(unsigned i=0;i<64;i++)d[i]=0;for(unsigned i=0;i<16;i++)j[i]=0;
 d[0]=TT_Q38_PROJECTION_INPUT_MARKER;d[1]=1;d[2]=67108864;d[3]=m[6];d[4]=m[7];d[5]=w-1;d[6]=m[9];d[7]=w;
 d[8]=m[15];d[9]=PioneerQ38ConvPeer(w);d[10]=(w-1)*1024;d[11]=5120;d[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+(w-1)*512;
 d[13]=TT_Q38_PROJECTION_LINES_OFFSET+(w-1)*128;d[14]=8;d[15]=20;
 for(unsigned i=0;i<16;i++)d[16+i]=Source(w)[i];
 for(unsigned i=32;i<40;i++)d[i]=m[i];
 d[40]=m[4];d[41]=m[5];d[42]=m[3];d[43]=5;
 j[0]=TT_Q38_PROJECTION_INPUT_MARKER;j[1]=256;j[2]=576;j[3]=m[9];j[10]=TT_Q38_FFN_PERSISTENT_MARKER;j[11]=w;
 j[12]=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;j[13]=5;
}
static void Quantize(){
 Before(20,5);
 for(unsigned w=1;w<=20;w++){
  uint32_t d[64],j[16];Input(w,d,j);if(!PioneerQ38ProjectionInputJob(j,d,Source(w),w,PioneerQ38ConvPeer(w)))cohort_fault_park();
  Write(w,0x74200,d,64);Write(w,0x74000,j,16);
 }
 Audit();for(unsigned w=1;w<=20;w++){unsigned next=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;Write(w,0x74100,&next,1);}
 for(unsigned w=1;w<=20;w++){
  unsigned next=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;Wait(w,next,5,false);
  uint32_t d[64],j[16],p[64],after[8];Input(w,d,j);Read(w,0x83f00,p,64);
  const auto* before=reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_COUNTERS+(w-1)*32);
  if(!PioneerQ38ProjectionInputProof(p,j,d,Source(w),w,PioneerQ38ConvPeer(w),before,p+8))cohort_fault_park();
  Sources(w);Counters(w,after,p+8);Words(TT_Q38_COHORT_SEQUENCE)[w-1]=next;Receipt()[50]++;
 }
 Audit(); // ONE actual Q8 producer group reused by ALL four SAME-input matrices
}
static void Gather(){
 const auto* m=Snapshot();uint32_t d[64]={},before[8],after[8];
 d[0]=TT_Q38_PROJECTION_GATHER_MARKER;d[1]=1;d[2]=67108864;d[3]=m[6];d[4]=m[7];d[5]=20;d[6]=m[9];d[8]=m[15];d[9]=129;
 d[10]=TT_Q38_PROJECTION_NATIVE_OFFSET;d[11]=TT_Q38_PROJECTION_LINES_OFFSET;d[12]=TT_Q38_PROJECTION_COMPACT_OFFSET;d[13]=10240;d[14]=640;d[15]=8;
 for(unsigned i=16;i<40;i++)d[i]=m[i];
 d[40]=m[4];d[41]=m[5];d[42]=m[3];d[43]=5;
 for(unsigned i=0;i<64;i++)Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]=d[i];
 for(unsigned i=0;i<8;i++)before[i]=*Words(0xffb20000+Counter(i));
 Audit();if(q38_resident_advance_gather_once()!=20)cohort_fault_park();Audit();
 uint32_t summary[32];for(unsigned i=0;i<32;i++)summary[i]=Words(TT_Q38_FFN_RESIDENT_SUMMARY)[i];
 for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 if(!PioneerQ38ProjectionGatherSummary(summary,d,before,after))cohort_fault_park();
 for(unsigned i=0;i<32;i++)Receipt()[64+i]=summary[i];
}
static void Matrix(unsigned role){
 const auto* m=Snapshot();const auto* r=PioneerQ38ResidentAdvanceRecord(m,role);
 if(!r)cohort_fault_park();
 Before(r[2],1);uint32_t d[64]={},before[8],after[8];
 d[0]=TT_Q38_NEXT_PROJECTION_MATRIX_MARKER;d[1]=1;d[2]=67108864;d[3]=m[6];d[4]=m[7];d[6]=r[5];d[7]=r[2];d[8]=m[15];d[9]=129;
 d[10]=r[6];d[11]=r[4];d[12]=r[7];d[13]=TT_Q38_NEXT_PROJECTION_MATRIX_ROWS;d[14]=5120;d[15]=8;
 for(unsigned i=16;i<40;i++)d[i]=m[i];
 d[40]=m[4];d[41]=m[5];d[42]=m[3];d[43]=r[0];d[44]=r[2];d[45]=1;
 for(unsigned i=0;i<64;i++)Words(TT_Q38_NEXT_PROJECTION_MATRIX_FRAME)[i]=d[i];
 const uint32_t h[]={TT_Q38_NEXT_PROJECTION_MATRIX_MARKER,1,1,8,5120,r[2],r[5],TT_Q38_NEXT_PROJECTION_MATRIX_INPUT,1,0,0,0,0,0,0,0};
 for(unsigned i=0;i<16;i++)Words(0x74000)[i]=h[i];
 for(unsigned i=0;i<8;i++)Words(0x74040)[i]=i<7?PioneerQ38ProjectionRoute(i):0;
 for(unsigned w=0;w<119;w++){
  for(unsigned i=0;i<8;i++)Words(0x74080)[w*8+i]=w>=r[2]?0:i==3?Words(TT_Q38_COHORT_SEQUENCE)[w]+1:r[64+w*8+i];
  Words(TT_Q38_NEXT_PROJECTION_MATRIX_ROWS)[w]=r[1016+w];
  for(unsigned i=0;i<16;i++)if(Words(0x7d800)[w*16+i]!=Source(w+1)[i])cohort_fault_park();
 }
 for(unsigned i=0;i<8;i++)before[i]=*Words(0xffb20000+Counter(i));
 Audit();if(q38_resident_advance_matrix_once()!=r[2])cohort_fault_park();Audit();
 uint32_t stats[64];for(unsigned i=0;i<64;i++)stats[i]=Words(0x7f800)[i];
 for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 if(!PioneerQ38ResidentAdvanceMatrixReceipt(stats,m,r,before,after))cohort_fault_park();
 for(unsigned i=0;i<64;i++)Receipt()[96+role*64+i]=stats[i];
 for(unsigned w=1;w<=r[2];w++){
  unsigned next=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;Wait(w,next,1,false);
  const auto* p=reinterpret_cast<const uint32_t*>(0x76000+(w-1)*256);const auto* part=r+64+(w-1)*8;
  const auto* baseline=reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_COUNTERS+(w-1)*32);uint32_t physical[8];
  unsigned reads=part[2]*160+2,flits=part[2]*1440+170;
  for(unsigned i=0;i<8;i++){
   unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
   if(p[i]!=baseline[i] || uint32_t(p[8+i]-baseline[i])!=delta)cohort_fault_park();
  }
  if(p[16]!=part[2]*320 || p[17]!=next || p[18]!=w || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER
   || p[30]!=Source(w)[0] || p[31]!=PioneerQ38ConvPeer(w))cohort_fault_park();
  const uint32_t last[]={part[0]+(part[2]*160-1)*576,0,part[4],0x90000,0,PioneerQ38ConvPeer(w),0,0,576,0};
  for(unsigned i=0;i<10;i++)if(p[20+i]!=last[i])cohort_fault_park();
  for(unsigned i=0;i<16;i++)if(p[32+i]!=Source(w)[i])cohort_fault_park();
  for(unsigned i=48;i<64;i++)if(p[i])cohort_fault_park();
  Sources(w);Counters(w,physical,p+8);Words(TT_Q38_COHORT_SEQUENCE)[w-1]=next;Receipt()[51]++;
 }
 Receipt()[52]++;Audit();
}
}
extern "C" void q38_resident_advance_capture(){
 using namespace Advance;
 for(unsigned i=0;i<TT_Q38_ADVANCE_WORDS;i++)Words(TT_Q38_ADVANCE_SHADOW)[i]=Words(TT_Q38_ADVANCE_MANIFEST)[i];
 if(!PioneerQ38ResidentAdvanceShape(Snapshot(),reinterpret_cast<const uint32_t*>(TT_Q38_LAYER_LINK_MANIFEST),
  reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_MANIFEST)) || !Current() || !Quiet())cohort_fault_park();
 for(unsigned i=0;i<TT_Q38_ADVANCE_RECEIPT_WORDS;i++)Receipt()[i]=0;
}
extern "C" unsigned q38_resident_advance_once(){
 using namespace Advance;Audit();
 const auto* link=reinterpret_cast<const uint32_t*>(TT_Q38_LAYER_LINK_SHADOW);
 const auto* parent=reinterpret_cast<const uint32_t*>(TT_Q38_RESIDENT_LAYER_SHADOW);
 const auto* done=reinterpret_cast<const uint32_t*>(TT_Q38_LAYER_LINK_RECEIPT);uint32_t after[8],command[10];
 for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<10;i++)command[i]=Words(0xffb20000)[i];
 if(!PioneerQ38ResidentAdvanceShape(Snapshot(),link,Cohort())
  || !PioneerQ38ResidentLayerLinkReceipt(done,link,parent,done+48,after,command))cohort_fault_park();
 for(unsigned i=0;i<8;i++)Receipt()[352+i]=after[i];
 Quantize();Gather();for(unsigned role=0;role<4;role++)Matrix(role);
 Before(119,1); // ALL retained peers,including workers inactive in small matrices
 for(unsigned w=0;w<119;w++)Receipt()[378+w]=Words(TT_Q38_COHORT_SEQUENCE)[w];
 Receipt()[53]=119;Audit();
 for(unsigned i=0;i<8;i++)Receipt()[360+i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<10;i++)Receipt()[368+i]=Words(0xffb20000)[i];
 if(!PioneerQ38ResidentAdvanceTraffic(reinterpret_cast<const uint32_t*>(TT_Q38_ADVANCE_RECEIPT)))cohort_fault_park();
 for(unsigned i=0;i<44;i++)Receipt()[i]=i==0?0:i==2?unsigned(TT_Q38_ADVANCE_RECEIPT_WORDS):Snapshot()[i];
 Fence();Receipt()[0]=TT_Q38_ADVANCE_RECEIPT_MARKER;Fence();return 1;
}
extern "C" unsigned q38_resident_advance_main(){
 q38_resident_advance_capture();
 if(q38_resident_layer_link_main()!=5120)cohort_fault_park();
 if(q38_resident_advance_once()!=1)cohort_fault_park();
 return 5120; // next projections, not a completed second layer or model token
}
