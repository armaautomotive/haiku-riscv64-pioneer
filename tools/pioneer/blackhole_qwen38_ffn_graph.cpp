/* SPDX-License-Identifier: MIT */
// NEW OFFLINE complete-FFN root. No native selector/controller installs it.
// The kernel must own ALL learned/mutable/source/code/host/clock contexts.
// QFG1 additionally needs an independently completed initial GDDR producer.
// QFG2 needs exact staged L1 input ownership BEFORE launch;its actual copy/
// readback below produces GDDR input BEFORE any peer computation. Neither
// image supplies the missing native scope/host/terminal authority itself.
#ifndef PIONEER_Q38_FFN_GRAPH_ROOT
#error Separate complete FFN root flag required
#endif
#define block_main q38_ffn_graph_matrix_once
#include "blackhole_qwen38_dispatch.cpp"
#undef block_main
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGather2Shape.h"
extern "C" unsigned q38_ffn_graph_gather_once();
#define GRAPH_FN __attribute__((noinline,section(".q38_graph")))
namespace Graph {
static const uint32_t* Snapshot(){return reinterpret_cast<const uint32_t*>(TT_Q38_FFN_GRAPH_SHADOW);}
static const uint32_t* Sources(unsigned w){return reinterpret_cast<const uint32_t*>(0x7d800+(w-1)*64);}
static volatile uint32_t* Seq(){return Words(TT_Q38_FFN_GRAPH_SEQUENCE);}
static volatile uint32_t* Stats(){return Words(TT_Q38_FFN_GRAPH_STATS);}
static void Clear(uint32_t* data,unsigned count){volatile uint32_t* out=data;for(unsigned i=0;i<count;i++)out[i]=0;}
static GRAPH_FN bool Sealed() {
 for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)if(Words(TT_Q38_FFN_GRAPH_MANIFEST)[i]!=Words(TT_Q38_FFN_GRAPH_SHADOW)[i])return false;
 const auto* d=Snapshot();for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+SourceOffset(i))!=d[16+i])return false;
 return true;
}
// EXACT already-known CSR words ONLY. No speculative contiguous read of
// unclassified/reserved NIU registers. All endpoints are internal constants.
static GRAPH_FN void Packet(bool write,unsigned w,unsigned local,unsigned remote,unsigned bytes) {
 bool csr=remote==0xffb20100 || (remote>=0xffb20118 && remote<=0xffb20144 && !(remote&3))
  || remote==0xffb20150 || remote==0xffb20154 || remote==0xffb2016c
  || (remote>=0xffb20204 && remote<=0xffb20214 && !(remote&3)) || remote==0xffb20220 || remote==0xffb20228;
 bool permit=write?(remote==0x74000 && bytes==64) || (remote==0x74200 && bytes==256) || (remote==0x74100 && bytes==4)
  :(remote==0x74140 && bytes==64) || (remote==0x83f00 && bytes==256)
  || ((remote==0xffb13140 || remote==0xffb121b0 || csr) && bytes==4);
 if(!permit || !w || w>119 || local<0x75000 || local>0x75800-bytes || ((local^remote)&31) || !Idle())block_fault_park();
 const auto* d=Snapshot();for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+SourceOffset(i))!=d[16+i])block_fault_park();
 unsigned flits=(bytes+63)/64;uint32_t expected[8];
 for(unsigned i=0;i<8;i++) {
  expected[i]=*Words(0xffb20000+Offset(i))+(i==2 || i==6?1U:i==0 || i==3?unsigned(!write):i==1?(!write?flits:0U):i==5?(write?flits:0U):unsigned(write));
  // Both entries sample the SAME accepted-transaction counter with the
  // SAME modulo32 delta. A change while idle invalidates this baseline;
  // stop BEFORE even reading the next counter or writing a command word.
  if(i==6 && expected[6]!=expected[2])block_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:XY(w),write?remote:local,0,write?XY(w):129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++) {
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Offset(i))==expected[i];
  if(done){if(!Idle())block_fault_park();for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+SourceOffset(i))!=d[16+i])block_fault_park();
   Stats()[write?7:6]++;Stats()[write?9:8]+=flits;Fence();return;}
 }
 block_fault_park(); // UNKNOWN:no later packet/reset/restore/retry
}
static GRAPH_FN void SourceCurrent(unsigned w) {
 // Blackhole NoC MMIO transfers permit ONLY4bytes,even for adjacent CSRs.
 // https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#noc_at_len_be
 for(unsigned i=0;i<16;i++) {
  unsigned offset=SourceOffset(i),local=0x75000+(offset&31);Packet(false,w,local,0xffb20000+offset,4);
  if(Words(local)[0]!=Sources(w)[i])block_fault_park();
 }
}
#ifdef PIONEER_Q38_FFN_GRAPH_INPUT_ROOT
// NEW QFG2 ONLY. Future native owner must exclusively retain/prove the
// exact staged5120F32 words at80000..85000. This image never grants that
// authority from a frame flag or digest. Readback88000..8c000 and the input
// are disjoint,and both are reused ONLY after producer completion.
static GRAPH_FN void InputPacket(bool write,unsigned offset,unsigned bytes) {
 const auto* d=Snapshot();
 if((offset!=0 && offset!=16384) || bytes!=(offset?4096U:16384U) || !Sealed() || !Idle())block_fault_park();
 unsigned local=write?0x80000+offset:0x88000,remote=d[4]+offset,flits=bytes/64;
 uint32_t expected[8];
 for(unsigned i=0;i<8;i++) {
  expected[i]=*Words(0xffb20000+Offset(i))+(i==2 || i==6?1U:i==0 || i==3?unsigned(!write):i==1?(!write?flits:0U):i==5?(write?flits:0U):unsigned(write));
  if(i==6 && expected[6]!=expected[2])block_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:d[8],write?remote:local,0,write?d[8]:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++) {
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Offset(i))==expected[i];
  if(done){if(!Sealed() || !Idle())block_fault_park();
   Stats()[write?7:6]++;Stats()[write?9:8]+=flits;Fence();return;}
 }
 block_fault_park(); // UNKNOWN:no readback,next packet,peer job,STOP or retry
}
static GRAPH_FN void InitialInput() {
 if(!Sealed() || !Idle())block_fault_park();
 // Validate the COMPLETE source before the FIRST input write. This is
 // on-card wire/finite safety,not CPU learned-product validation.
 for(unsigned i=0;i<5120;i++)if((Words(0x80000)[i]&0x7f800000U)==0x7f800000U)block_fault_park();
 for(unsigned offset=0;offset<20480;offset+=16384) {
  unsigned bytes=offset?4096:16384;InputPacket(true,offset,bytes);InputPacket(false,offset,bytes);
  for(unsigned i=0;i<bytes/4;i++) {
   uint32_t value=Words(0x80000+offset)[i];
   if((value&0x7f800000U)==0x7f800000U || Words(0x88000)[i]!=value)block_fault_park();
  }
 }
 // Observed completion DATA only. The future native caller still must
 // independently prove exact source ownership and root physical terminal.
 Stats()[57]=0x32494651;Stats()[58]=20480;Stats()[59]=2;Stats()[60]=2;
 Stats()[61]=Snapshot()[13];Stats()[62]=Snapshot()[40];Stats()[63]=Snapshot()[41];Fence();
}
#endif
// BEFORE samples are independently stored,never taken from a peer proof.
// AFTER each physical subread must match its expected word BEFORE another
// packet may issue. Observed mismatches therefore stop at the FIRST group.
static GRAPH_FN void Counters(unsigned w,volatile uint32_t* out,const uint32_t* expected) {
 for(unsigned i=0;i<8;i++) {
  if(i==6){out[i]=out[2];if(expected && out[i]!=expected[i])block_fault_park();continue;}
  unsigned offset=Offset(i),local=0x75000+(offset&31);Packet(false,w,local,0xffb20000+offset,4);
  uint32_t v=Words(local)[0];if(expected && v!=expected[i])block_fault_park();out[i]=v;
 }
}
static GRAPH_FN bool Physical(unsigned w,bool stopped) {
 Packet(false,w,0x75000,0xffb13140,4);unsigned a=Words(0x75000)[0];
 // Exact qualified QFP2 companion code extents,NOT caller-supplied bounds.
 // Native factory MUST pin that full image before this ABI can be used.
 if((a&3) || a==TT_Q38_PEER_FAULT_ENTRY || a==TT_Q38_PEER_FAULT_PARK
  || (!stopped && PioneerQ38PeerTerminalPc(a))
  || ((a<0x64000 || a>=0x66b9c) && (a<0x72000 || a>=0x73ff4)))block_fault_park();
 Packet(false,w,0x75000,0xffb13140,4);unsigned b=Words(0x75000)[0];
 if((b&3) || b==TT_Q38_PEER_FAULT_ENTRY || b==TT_Q38_PEER_FAULT_PARK
  || (!stopped && PioneerQ38PeerTerminalPc(b))
  || ((b<0x64000 || b>=0x66b9c) && (b<0x72000 || b>=0x73ff4)))block_fault_park();
 Packet(false,w,0x75010,0xffb121b0,4);if(Words(0x75010)[0]!=0x46800)block_fault_park();
 // A producer publishes its flag before returning through the NEW helper.
 // Transitional known-code PCs may wait,but NEVER grant completion/cleanup.
 return stopped?PioneerQ38PeerTerminalPc(a) && PioneerQ38PeerTerminalPc(b):WaitPc(a) && WaitPc(b);
}
static GRAPH_FN void Wait(unsigned w,unsigned sequence,unsigned operation,bool previous) {
 for(unsigned attempt=0;attempt<65536;attempt++) {
  Packet(false,w,0x75000,0x74140,64);Stats()[10]++;
  const volatile uint32_t* s=Words(0x75000);unsigned old=previous?sequence:sequence-1;
  if(s[2] || s[3]!=TT_Q38_FFN_PERSISTENT_MARKER || s[0]<old || s[0]>sequence || s[1]<old || s[1]>s[0]
   || (s[4] && s[4]!=operation))block_fault_park();
  for(unsigned i=5;i<16;i++)if(s[i])block_fault_park();
  if(s[0]==sequence && s[1]==sequence && s[4]==(!previous && operation==2?2U:0U) && Physical(w,!previous && operation==2)) {
   Stats()[11]++;return;
  }
 }
 block_fault_park();
}
static GRAPH_FN void StageProducer(unsigned w,unsigned operation,unsigned phase) {
 const auto* d=Snapshot();uint32_t frame[64],job[16];for(unsigned i=0;i<64;i++)frame[i]=d[i];
 Clear(job,16); // explicit word stores,NO libc memset/byte-store dependency
 if(operation!=2) {
  frame[0]=operation==4?unsigned(TT_Q38_FFN_INPUT_MARKER):unsigned(TT_Q38_FFN_RESIDENT_MARKER);frame[5]=w-1;frame[6]=phase;frame[7]=w;frame[9]=XY(w);
#ifdef PIONEER_Q38_FFN_GRAPH_INPUT_ROOT
  frame[1]=1; // QFP2 producer interface stays EXACT version1
#endif
  frame[10]=operation==4?(w-1)*1024:30848+(w-1)*1024;frame[11]=operation==4?0:100480+(w-1)*1024;
  frame[12]=(operation==4?20480U:239744U)+(w-1)*512;frame[13]=295360+(w-1)*64;frame[14]=256;frame[15]=0;
  for(unsigned i=0;i<16;i++)frame[16+i]=Sources(w)[i];
  for(unsigned i=0;i<64;i++)Words(0x75000)[i]=frame[i];
  Packet(true,w,0x75000,0x74200,256);
  job[0]=frame[0]==TT_Q38_FFN_INPUT_MARKER?unsigned(TT_Q38_FFN_INPUT_MARKER):unsigned(TT_Q38_FFN_PERSISTENT_JOB);job[1]=256;job[2]=576;job[3]=phase;
 }
 job[10]=TT_Q38_FFN_PERSISTENT_MARKER;job[11]=w;job[12]=Seq()[w-1]+1;job[13]=operation;
 for(unsigned i=0;i<16;i++)Words(0x75000)[i]=job[i];
 Packet(true,w,0x75000,0x74000,64);
}
static GRAPH_FN void Producer(unsigned operation,unsigned count,unsigned phase) {
 if(!Sealed())block_fault_park();
 for(unsigned w=1;w<=count;w++){Wait(w,Seq()[w-1],operation,true);SourceCurrent(w);Counters(w,Words(TT_Q38_FFN_GRAPH_COUNTERS+(w-1)*32),nullptr);}
 for(unsigned w=1;w<=count;w++)StageProducer(w,operation,phase);
 if(!Sealed())block_fault_park();
 for(unsigned w=1;w<=count;w++){Words(0x75000)[0]=Seq()[w-1]+1;Packet(true,w,0x75000,0x74100,4);}
 for(unsigned w=1;w<=count;w++) {
  unsigned next=Seq()[w-1]+1;Wait(w,next,operation,false);
  const uint32_t* before=reinterpret_cast<const uint32_t*>(TT_Q38_FFN_GRAPH_COUNTERS+(w-1)*32);
  uint32_t expected[8],after[8];const unsigned delta[]={operation==2?0U:operation==4?1U:2U,operation==2?0U:operation==4?16U:32U,operation==2?0U:operation==4?3U:4U,operation==2?0U:operation==4?1U:2U,operation==2?0U:2U,operation==2?0U:9U,operation==2?0U:operation==4?3U:4U,operation==2?0U:2U};
  for(unsigned i=0;i<8;i++)expected[i]=before[i]+delta[i];
  if(operation!=2) {
   Packet(false,w,0x75000,0x83f00,256);uint32_t proof[64],frame[64],job[16];for(unsigned i=0;i<64;i++){proof[i]=Words(0x75000)[i];frame[i]=Snapshot()[i];}
   frame[0]=operation==4?unsigned(TT_Q38_FFN_INPUT_MARKER):unsigned(TT_Q38_FFN_RESIDENT_MARKER);frame[5]=w-1;frame[6]=phase;frame[7]=w;frame[9]=XY(w);
#ifdef PIONEER_Q38_FFN_GRAPH_INPUT_ROOT
   frame[1]=1;
#endif
   frame[10]=operation==4?(w-1)*1024:30848+(w-1)*1024;frame[11]=operation==4?0:100480+(w-1)*1024;
   frame[12]=(operation==4?20480U:239744U)+(w-1)*512;frame[13]=295360+(w-1)*64;frame[14]=256;frame[15]=0;
   for(unsigned i=0;i<16;i++)frame[16+i]=Sources(w)[i];
   Clear(job,16);
   job[0]=operation==4?unsigned(TT_Q38_FFN_INPUT_MARKER):unsigned(TT_Q38_FFN_PERSISTENT_JOB);job[1]=256;job[2]=576;job[3]=phase;
   job[10]=TT_Q38_FFN_PERSISTENT_MARKER;job[11]=w;job[12]=next;job[13]=operation;
   // Logical DATA expectation ONLY. Current physical counters are sampled
   // below and matched word-by-word before this may advance ANY sequence.
   if(!(operation==4?PioneerQ38FfnPersistentInputProof(proof,job,frame,Sources(w),w,XY(w),before,expected)
    :PioneerQ38FfnPersistentActivationProof(proof,job,frame,Sources(w),w,XY(w),before,expected)))block_fault_park();
  }
  SourceCurrent(w);Counters(w,after,expected);Seq()[w-1]=next;
 }
 if(!Sealed())block_fault_park();
}
static GRAPH_FN void GatherInput(unsigned count,unsigned phase) {
 uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Snapshot()[i];
#ifdef PIONEER_Q38_FFN_GRAPH_INPUT_ROOT
 d[1]=1; // unchanged existing gather interface
#endif
 d[0]=TT_Q38_FFN_GATHER2_MARKER;d[5]=count;d[6]=phase;d[7]=0;d[10]=count==20?20480:239744;d[11]=295360;
 d[12]=count==20?30720:274560;d[13]=count==20?10240:34816;d[14]=count==20?128:320;d[15]=0;
 for(unsigned i=0;i<64;i++)Words(0x74200)[i]=d[i];
 Fence();
 if(q38_ffn_graph_gather_once()!=count || !Sealed())block_fault_park();
 const volatile uint32_t* s=Words(0x74300);Stats()[6]+=s[0];Stats()[7]+=s[2];Stats()[8]+=s[1];Stats()[9]+=s[3];
}
static GRAPH_FN unsigned StageMatrix(unsigned role,unsigned phase) {
 const auto* d=Snapshot();const uint32_t* m=d+PioneerQ38FfnGraphRoleOffset(role);unsigned n=m[2];
 for(unsigned w=1;w<=n;w++)Counters(w,Words(TT_Q38_FFN_GRAPH_COUNTERS+(w-1)*32),nullptr);
 for(unsigned w=0;w<119;w++){for(unsigned i=0;i<8;i++)Words(0x74080)[w*8+i]=m[16+w*8+i];if(w<n)Words(0x74080)[w*8+3]=Seq()[w]+1;Words(TT_Q38_FFN_MATRIX_ROWS)[w]=m[16+119*8+w];}
 uint32_t frame[64];for(unsigned i=0;i<64;i++)frame[i]=d[i];frame[0]=TT_Q38_FFN_MATRIX_MARKER;frame[5]=role;frame[6]=phase;frame[7]=n;
#ifdef PIONEER_Q38_FFN_GRAPH_INPUT_ROOT
 frame[1]=1; // unchanged existing resident matrix interface
#endif
 frame[10]=role==0?30848:role==1?100480:274880;frame[11]=m[4];frame[12]=m[4]*4;frame[13]=TT_Q38_FFN_MATRIX_ROWS;frame[14]=m[3];frame[15]=m[1];
 for(unsigned i=0;i<64;i++)Words(TT_Q38_FFN_MATRIX_FRAME)[i]=frame[i];
 for(unsigned b=0;b<8;b++)Words(0x74040)[b]=b<7?PioneerQ38SpanRoute(b):0;
 const uint32_t header[]={TT_Q38_FFN_MATRIX_MARKER,1,1,m[1],m[3],n,phase,3,1,0,0,0,0,0,0,0};
 for(unsigned i=0;i<16;i++)Words(0x74000)[i]=header[i];
 Fence();return n;
}
static GRAPH_FN void MatrixFinished(unsigned role) {
 const auto* d=Snapshot();const uint32_t* m=d+PioneerQ38FfnGraphRoleOffset(role);
 for(unsigned w=1;w<=m[2];w++) {
  const auto* before=reinterpret_cast<const uint32_t*>(TT_Q38_FFN_GRAPH_COUNTERS+(w-1)*32);unsigned groups=m[16+(w-1)*8+2],blocks=m[3]/256;
  unsigned reads=groups*blocks+(m[3]*2+16383)/16384+1,flits=groups*blocks*Tile(m[1])/64+(m[3]*2+((blocks*4+63)&~63U))/64;
  uint32_t expected[8],after[8];for(unsigned i=0;i<8;i++)expected[i]=before[i]+(i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0U);
  SourceCurrent(w);Counters(w,after,expected);Seq()[w-1]++;
 }
 const volatile uint32_t* s=Words(0x7f800);Stats()[6]+=s[6];Stats()[7]+=s[7];Stats()[8]+=s[8];Stats()[9]+=s[9];Stats()[10]+=s[10];Stats()[11]+=s[11];
 if(!Sealed())block_fault_park();
}
}
extern "C" GRAPH_FN unsigned block_main() {
 using namespace Graph;
 for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)Words(TT_Q38_FFN_GRAPH_SHADOW)[i]=Words(TT_Q38_FFN_GRAPH_MANIFEST)[i];
 Fence();
 const auto* d=Snapshot();if(!PioneerQ38FfnGraphManifest(d,Sources(1)))return 0;
 if(!Sealed() || !Idle())block_fault_park();
 for(unsigned i=0;i<64;i++)Stats()[i]=0;
 Stats()[0]=TT_Q38_FFN_GRAPH_MARKER;Stats()[1]=TT_Q38_FFN_GRAPH_VERSION;Stats()[2]=d[5];Stats()[12]=d[13];
 for(unsigned i=0;i<8;i++) {
  Stats()[16+i]=*Words(0xffb20000+Offset(i));
  if(i==6 && Stats()[22]!=Stats()[18])block_fault_park();
 }
 for(unsigned w=0;w<119;w++)Seq()[w]=d[64+w];
 for(unsigned w=1;w<=119;w++){Wait(w,Seq()[w-1],1,true);SourceCurrent(w);}
 if(d[5]==2)Producer(2,119,d[13]);
 else {
#ifdef PIONEER_Q38_FFN_GRAPH_INPUT_ROOT
  InitialInput(); // actual producer BEFORE any peer reads the initial GDDR
#endif
  Producer(4,20,d[6]+1);GatherInput(20,d[6]+1);
  if(!Sealed())block_fault_park();
  unsigned n=StageMatrix(0,d[6]+2);if(q38_ffn_graph_matrix_once()!=n)block_fault_park();MatrixFinished(0);
  n=StageMatrix(1,d[6]+3);if(q38_ffn_graph_matrix_once()!=n)block_fault_park();MatrixFinished(1);
  Producer(3,68,d[6]+4);GatherInput(68,d[6]+4);
  n=StageMatrix(2,d[6]+5);if(q38_ffn_graph_matrix_once()!=n)block_fault_park();MatrixFinished(2);
 }
 if(!Sealed() || !Idle())block_fault_park();
 for(unsigned w=0;w<119;w++)if(Seq()[w]!=PioneerQ38FfnGraphNextPeer(d,w))block_fault_park();
 for(unsigned i=0;i<8;i++) {
  Stats()[24+i]=*Words(0xffb20000+Offset(i));unsigned expected=i==0 || i==3?Stats()[6]:i==1?Stats()[8]:i==2 || i==6?Stats()[6]+Stats()[7]:i==5?Stats()[9]:Stats()[7];
  if(uint32_t(Stats()[24+i]-Stats()[16+i])!=expected)block_fault_park();
 }
 for(unsigned i=0;i<16;i++)Stats()[32+i]=d[16+i];
 Stats()[48]=d[14];Stats()[49]=d[5]==1?7:1;Stats()[50]=d[5]==1?3:0;
#ifdef PIONEER_Q38_FFN_GRAPH_INPUT_ROOT
 Stats()[49]=d[5]==1?8:1;
#endif
 Stats()[51]=d[5]==1?20:0;Stats()[52]=d[5]==1?68:0;Stats()[53]=d[5]==1?20480:0;Stats()[54]=d[13];Stats()[55]=d[42];Stats()[56]=1;Fence();
 return d[5]==1?5120:119; // complete device FFN/STOP only,NOT model/native authority
}
