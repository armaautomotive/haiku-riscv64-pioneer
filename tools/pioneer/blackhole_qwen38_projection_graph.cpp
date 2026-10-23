/* SPDX-License-Identifier: MIT */
// NEW resident-input complete-projection parent. No native selector/owner
// installs this image. Before entry a real owner MUST authenticate canonical
// weights/initialized pages,retain the completed GDDR F32 input producer,
// full119 QFP3/source/sequence/clock/host context and every new mapped view.
#ifndef PIONEER_Q38_PROJECTION_GRAPH_ROOT
#error Separate projection parent image required
#endif
#if PIONEER_Q38_PROJECTION_GRAPH_ROOT != 1
#error Exact new parent role required
#endif
#define block_main q38_projection_matrix_once
#include "blackhole_qwen38_dispatch.cpp"
#undef block_main
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphShape.h"
extern "C" unsigned q38_projection_gather_once();
#define PARENT_FN __attribute__((noinline,section(".q38_projection_graph")))
namespace ProjectionParent {
static const uint32_t* Snapshot(){return reinterpret_cast<const uint32_t*>(TT_Q38_PROJECTION_GRAPH_SHADOW);}
static const uint32_t* Sources(unsigned w){return reinterpret_cast<const uint32_t*>(TT_Q38_PROJECTION_GRAPH_SOURCES+(w-1)*64);}
static volatile uint32_t* Seq(){return Words(TT_Q38_PROJECTION_GRAPH_SEQUENCE);}
static volatile uint32_t* Stats(){return Words(TT_Q38_PROJECTION_GRAPH_STATS);}
static void Clear(uint32_t* p,unsigned n){volatile uint32_t* out=p;for(unsigned i=0;i<n;i++)out[i]=0;}
static PARENT_FN bool Current(){
 const auto* d=Snapshot();for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+SourceOffset(i))!=d[16+i])return false;
 return true;
}
// Full immutable manifest check at phase boundaries,not an O(full manifest
// * every packet) scan. Exact pinned code writes the shadow ONLY on entry;
// all command/response/output windows are disjoint. The missing native
// exclusive owner is STILL REQUIRED to exclude every other writer. Each
// individual packet retains FIRST-failure source/idle/counter checks below.
static PARENT_FN bool Audit(){
 for(unsigned i=0;i<TT_Q38_PROJECTION_GRAPH_WORDS;i++)if(Words(TT_Q38_PROJECTION_GRAPH_MANIFEST)[i]!=Words(TT_Q38_PROJECTION_GRAPH_SHADOW)[i])return false;
 return Current();
}
static PARENT_FN void Packet(bool write,unsigned w,unsigned local,unsigned remote,unsigned bytes){
 bool csr=remote==0xffb20100 || (remote>=0xffb20118 && remote<=0xffb20144 && !(remote&3))
  || remote==0xffb20150 || remote==0xffb20154 || remote==0xffb2016c
  || (remote>=0xffb20204 && remote<=0xffb20214 && !(remote&3)) || remote==0xffb20220 || remote==0xffb20228;
 bool permit=write?(remote==0x74000 && bytes==64) || (remote==0x74200 && bytes==256) || (remote==0x74100 && bytes==4)
  :(remote==0x74140 && bytes==64) || (remote==0x83f00 && bytes==256)
  || ((remote==0xffb13140 || remote==0xffb121b0 || csr) && bytes==4);
 if(!permit || !w || w>119 || local<0x75000 || local>0x75800-bytes || ((local^remote)&31)
  || !Current() || !Idle())block_fault_park();
 unsigned flits=(bytes+63)/64;uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==2 || i==6?1U:i==0 || i==3?unsigned(!write):i==1?(!write?flits:0U):i==5?(write?flits:0U):unsigned(write);
  expected[i]=*Words(0xffb20000+Offset(i))+delta;if(i==6 && expected[6]!=expected[2])block_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:XY(w),write?remote:local,0,write?XY(w):129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Offset(i))==expected[i];
  if(complete){if(!Current() || !Idle())block_fault_park();
   Stats()[write?7:6]++;Stats()[write?9:8]+=flits;Fence();return;}
 }
 block_fault_park(); // UNKNOWN:NO following packet/read/reset/restore/retry
}
static PARENT_FN void SourceCurrent(unsigned w){
 for(unsigned i=0;i<16;i++){
  unsigned offset=SourceOffset(i),local=0x75000+(offset&31);Packet(false,w,local,0xffb20000+offset,4);
  if(Words(local)[0]!=Sources(w)[i])block_fault_park();
 }
}
static PARENT_FN void Counters(unsigned w,volatile uint32_t* out,const uint32_t* expected){
 for(unsigned i=0;i<8;i++){
  if(i==6){out[i]=out[2];if(expected && out[i]!=expected[i])block_fault_park();continue;}
  unsigned offset=Offset(i),local=0x75000+(offset&31);Packet(false,w,local,0xffb20000+offset,4);
  uint32_t value=Words(local)[0];if(expected && value!=expected[i])block_fault_park();out[i]=value;
 }
}
static PARENT_FN bool Physical(unsigned w,bool stopped){
 unsigned pc[2];for(unsigned sample=0;sample<2;sample++){
  Packet(false,w,0x75000,0xffb13140,4);unsigned value=pc[sample]=Words(0x75000)[0];
  if((value&3) || value==TT_Q38_PEER_FAULT_ENTRY || value==TT_Q38_PEER_FAULT_PARK
   || (!stopped && PioneerQ38PeerTerminalPc(value))
   || !((value>=0x64000 && value<0x67fe0) || (value>=0x70b00 && value<0x71f80) || (value>=0x72000 && value<0x73fe4)))block_fault_park();
 }
 Packet(false,w,0x75010,0xffb121b0,4);if(Words(0x75010)[0]!=0x46800)block_fault_park();
 return stopped?PioneerQ38PeerTerminalPc(pc[0]) && PioneerQ38PeerTerminalPc(pc[1]):WaitPc(pc[0]) && WaitPc(pc[1]);
}
static PARENT_FN void Wait(unsigned w,unsigned sequence,unsigned operation,bool previous){
 for(unsigned attempt=0;attempt<65536;attempt++){
  Packet(false,w,0x75000,0x74140,64);Stats()[10]++;
  const volatile uint32_t* s=Words(0x75000);unsigned old=previous?sequence:sequence-1;
  if(s[2] || s[3]!=TT_Q38_FFN_PERSISTENT_MARKER || s[0]<old || s[0]>sequence || s[1]<old || s[1]>s[0]
   || (s[4] && s[4]!=operation))block_fault_park();
  for(unsigned i=5;i<16;i++)if(s[i])block_fault_park();
  if(s[0]==sequence && s[1]==sequence && s[4]==(!previous && operation==2?2U:0U) && Physical(w,!previous && operation==2)){
   Stats()[11]++;return;
  }
 }
 block_fault_park();
}
static PARENT_FN void ProducerFrame(uint32_t* frame,uint32_t* job,unsigned w,unsigned operation,unsigned phase,unsigned sequence){
 const auto* d=Snapshot();Clear(frame,64);Clear(job,16);
 if(operation==5){
  frame[0]=TT_Q38_PROJECTION_INPUT_MARKER;frame[1]=1;frame[2]=67108864;frame[3]=d[3];frame[4]=d[4];frame[5]=w-1;frame[6]=phase;
  frame[7]=w;frame[8]=d[8];frame[9]=XY(w);frame[10]=(w-1)*1024;frame[11]=d[44];
  frame[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+(w-1)*512;frame[13]=TT_Q38_PROJECTION_LINES_OFFSET+(w-1)*128;
  frame[14]=d[48];frame[15]=d[44]/256;for(unsigned i=0;i<16;i++)frame[16+i]=Sources(w)[i];
  for(unsigned i=32;i<44;i++)frame[i]=d[i];
  job[0]=TT_Q38_PROJECTION_INPUT_MARKER;job[1]=256;job[2]=576;job[3]=phase;
 }
 job[10]=TT_Q38_FFN_PERSISTENT_MARKER;job[11]=w;job[12]=sequence;job[13]=operation;
}
static PARENT_FN void Producer(unsigned operation,unsigned count,unsigned phase){
 if(!Audit())block_fault_park();
 for(unsigned w=1;w<=count;w++){
  Wait(w,Seq()[w-1],operation,true);SourceCurrent(w);Counters(w,Words(TT_Q38_PROJECTION_GRAPH_COUNTERS+(w-1)*32),nullptr);
 }
 for(unsigned w=1;w<=count;w++){
  uint32_t frame[64],job[16];ProducerFrame(frame,job,w,operation,phase,Seq()[w-1]+1);
  if(operation==5){for(unsigned i=0;i<64;i++)Words(0x75000)[i]=frame[i];Packet(true,w,0x75000,0x74200,256);}
  for(unsigned i=0;i<16;i++)Words(0x75000)[i]=job[i];
  Packet(true,w,0x75000,0x74000,64);
 }
 if(!Audit())block_fault_park();
 for(unsigned w=1;w<=count;w++){Words(0x75000)[0]=Seq()[w-1]+1;Packet(true,w,0x75000,0x74100,4);}
 for(unsigned w=1;w<=count;w++){
  unsigned next=Seq()[w-1]+1;Wait(w,next,operation,false);
  const auto* before=reinterpret_cast<const uint32_t*>(TT_Q38_PROJECTION_GRAPH_COUNTERS+(w-1)*32);
  uint32_t expected[8],after[8];const unsigned delta[]={1,16,3,1,2,10,3,2};
  for(unsigned i=0;i<8;i++)expected[i]=before[i]+(operation==5?delta[i]:0U);
  if(operation==5){
   Packet(false,w,0x75000,0x83f00,256);uint32_t proof[64],frame[64],job[16];
   for(unsigned i=0;i<64;i++)proof[i]=Words(0x75000)[i];
   ProducerFrame(frame,job,w,operation,phase,next);
   if(!PioneerQ38ProjectionInputProof(proof,job,frame,Sources(w),w,XY(w),before,expected))block_fault_park();
  }
  SourceCurrent(w);Counters(w,after,expected);Seq()[w-1]=next;
 }
 if(!Audit())block_fault_park();
}
static PARENT_FN void Gather(unsigned phase){
 const auto* m=Snapshot();uint32_t d[64];Clear(d,64);
 d[0]=TT_Q38_PROJECTION_GATHER_MARKER;d[1]=1;d[2]=67108864;d[3]=m[3];d[4]=m[4];d[5]=m[44]/256;d[6]=phase;
 d[8]=m[8];d[9]=129;d[10]=TT_Q38_PROJECTION_NATIVE_OFFSET;d[11]=TT_Q38_PROJECTION_LINES_OFFSET;d[12]=TT_Q38_PROJECTION_COMPACT_OFFSET;
 d[13]=m[44]*2;unsigned scales=m[44]>>(m[48]==8?5:8);d[14]=(scales*4+63)&~63U;d[15]=m[48];
 for(unsigned i=16;i<44;i++)d[i]=m[i];
 for(unsigned i=0;i<64;i++)Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]=d[i];
 Fence();
 if(!Audit() || q38_projection_gather_once()!=d[5] || !Audit())block_fault_park();
 const volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);Stats()[6]+=s[0];Stats()[7]+=s[2];Stats()[8]+=s[1];Stats()[9]+=s[3];
}
static PARENT_FN unsigned StageMatrix(unsigned wave,unsigned phase){
 if(!Audit())block_fault_park();
 const auto* m=Snapshot();unsigned active=PioneerQ38ProjectionGraphWorkers(m,wave),bytes=0;
 for(unsigned w=1;w<=active;w++)Counters(w,Words(TT_Q38_PROJECTION_GRAPH_COUNTERS+(w-1)*32),nullptr);
 for(unsigned w=0;w<119;w++){
  const auto* r=w<active?m+192+(wave*119+w)*8:nullptr;
  for(unsigned i=0;i<8;i++)Words(0x74080)[w*8+i]=w>=active?0:i==3?Seq()[w]+1:i==5?Sources(w+1)[0]:r[i];
  Words(TT_Q38_PROJECTION_MATRIX_ROWS)[w]=w<active?r[3]:0;if(w<active)bytes+=r[2]*64;
 }
 uint32_t d[64];Clear(d,64);d[0]=TT_Q38_PROJECTION_MATRIX_MARKER;d[1]=1;d[2]=67108864;d[3]=m[3];d[4]=m[4];d[5]=wave;d[6]=phase;
 d[7]=active;d[8]=m[8];d[9]=129;d[10]=PioneerQ38ProjectionOutputOffset(m[42],m[43]);d[11]=m[45];d[12]=bytes;d[13]=TT_Q38_PROJECTION_MATRIX_ROWS;
 d[14]=m[44];d[15]=m[48];for(unsigned i=16;i<44;i++)d[i]=m[i];d[44]=m[12];d[45]=m[7];
 for(unsigned i=0;i<64;i++)Words(TT_Q38_PROJECTION_MATRIX_FRAME)[i]=d[i];
 for(unsigned b=0;b<8;b++)Words(0x74040)[b]=b<7?PioneerQ38ProjectionRoute(b):0;
 const uint32_t h[]={TT_Q38_PROJECTION_MATRIX_MARKER,1,1,m[48],m[44],active,phase,TT_Q38_PROJECTION_MATRIX_INPUT,1,0,0,0,0,0,0,0};
 for(unsigned i=0;i<16;i++)Words(0x74000)[i]=h[i];
 Fence();return active;
}
static PARENT_FN void MatrixFinished(unsigned wave){
 if(!Audit())block_fault_park();
 const auto* m=Snapshot();unsigned active=PioneerQ38ProjectionGraphWorkers(m,wave);
 for(unsigned w=1;w<=active;w++){
  const auto* before=reinterpret_cast<const uint32_t*>(TT_Q38_PROJECTION_GRAPH_COUNTERS+(w-1)*32);
  unsigned groups=m[192+(wave*119+w-1)*8+2],blocks=m[44]>>(m[48]==8?5:8);
  unsigned reads=groups*blocks+(m[44]*2+16383)/16384+1,flits=groups*blocks*Tile(m[48])/64+(m[44]*2+((blocks*4+63)&~63U))/64;
  uint32_t expected[8],after[8];for(unsigned i=0;i<8;i++)expected[i]=before[i]+(i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0U);
  SourceCurrent(w);Counters(w,after,expected);Seq()[w-1]++;
 }
 const volatile uint32_t* s=Words(0x7f800);Stats()[6]+=s[6];Stats()[7]+=s[7];Stats()[8]+=s[8];Stats()[9]+=s[9];Stats()[10]+=s[10];Stats()[11]+=s[11];
 if(!Audit())block_fault_park();
}
}
extern "C" PARENT_FN unsigned block_main(){
 using namespace ProjectionParent;
 for(unsigned i=0;i<TT_Q38_PROJECTION_GRAPH_WORDS;i++)Words(TT_Q38_PROJECTION_GRAPH_SHADOW)[i]=Words(TT_Q38_PROJECTION_GRAPH_MANIFEST)[i];
 Fence();
 const auto* d=Snapshot();if(!PioneerQ38ProjectionGraphManifest(d,Sources(1)))return 0;
 if(!Audit() || !Idle())block_fault_park();
 for(unsigned i=0;i<64;i++)Stats()[i]=0;
 Stats()[0]=TT_Q38_PROJECTION_GRAPH_MARKER;Stats()[1]=1;Stats()[2]=d[5];Stats()[12]=d[13];
 for(unsigned i=0;i<8;i++){Stats()[16+i]=*Words(0xffb20000+Offset(i));if(i==6 && Stats()[22]!=Stats()[18])block_fault_park();}
 for(unsigned w=0;w<119;w++)Seq()[w]=d[64+w];
 for(unsigned w=1;w<=119;w++){Wait(w,Seq()[w-1],1,true);SourceCurrent(w);}
 if(d[5]==2)Producer(2,119,d[13]);
 else{
  Producer(5,d[44]/256,d[6]+1);Gather(d[6]+1);
  for(unsigned wave=0;wave<d[7];wave++){
   unsigned active=StageMatrix(wave,d[6]+2+wave);if(q38_projection_matrix_once()!=active)block_fault_park();MatrixFinished(wave);
  }
 }
 if(!Audit() || !Idle())block_fault_park();
 for(unsigned w=0;w<119;w++)if(Seq()[w]!=PioneerQ38ProjectionGraphNextPeer(d,w))block_fault_park();
 for(unsigned i=0;i<8;i++){
  Stats()[24+i]=*Words(0xffb20000+Offset(i));unsigned expected=i==0 || i==3?Stats()[6]:i==1?Stats()[8]:i==2 || i==6?Stats()[6]+Stats()[7]:i==5?Stats()[9]:Stats()[7];
  if(uint32_t(Stats()[24+i]-Stats()[16+i])!=expected)block_fault_park();
 }
 for(unsigned i=0;i<16;i++)Stats()[32+i]=d[16+i];
 Stats()[48]=d[48];Stats()[49]=d[5]==1?d[44]/256:0;Stats()[50]=d[5]==1?d[7]:0;Stats()[51]=d[5]==1?d[12]:0;
 Stats()[52]=d[44];Stats()[53]=d[45];Stats()[54]=d[13];Stats()[55]=d[42]|(d[43]<<8);Stats()[56]=1;
 Stats()[57]=d[46];Stats()[58]=d[47];Stats()[59]=d[40];Stats()[60]=d[41];Stats()[61]=d[5];Fence();
 return d[5]==1?d[45]:119; // complete projection/STOP DATA,NOT model/native authority
}
