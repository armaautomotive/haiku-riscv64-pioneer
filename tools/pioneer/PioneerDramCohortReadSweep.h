/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRAM_COHORT_READ_SWEEP_H
#define PIONEER_DRAM_COHORT_READ_SWEEP_H
#include "PioneerDramWideQueuedReadSweep.h"
// OFFBOARD ONLY: no driver, native owner or public request selects this image.
// Seven readers, one firmware-derived bank route EACH, plus a coordinator.
// The ONLY aggregate time is the coordinator's ONE clock: before the first
// release packet through receipt of ALL seven done flags. Includes release
// skew/poll overhead. Reader clocks are local observations, NEVER summed.
// All eight workers need an exclusive native lease/snapshot/STOP/HOLD/restore
// owner before deployment. These DATA frames do not supply that authority.
struct PioneerDramCohortReadSweep:PioneerDramWideQueuedReadSweep {
 enum : uint32_t {Magic=0x34415244,ReportMagic=0x34435244,Version=4,
  Ready=0x74200,Start=0x74220,Done=0x74240,Payload=0x74300,
  Scratch=0x74400,PeerReports=0x74500,Peers=7,Words=32};
 static uint32_t Worker(unsigned role){return (role<7?role+1:10)|(2U<<6);}
 static bool Route(uint32_t xy){
  const unsigned x=xy&63,y=xy>>6;
  return (x==17 || x==18) && (y==12 || y==15 || y==18 || y==21);
 }
 static bool Shape(const uint32_t* j){
  if(j[0]!=Magic || j[1]!=Version || j[2]>Peers || !j[3]
   || !j[4] || j[4]>256 || j[5]!=14 || !j[6] || j[6]>16 || (j[6]&(j[6]-1)))return false;
  for(unsigned i=0;i<Peers;i++){
   if(!Route(j[7+i]))return false;
   for(unsigned k=0;k<i;k++)if(j[7+i]==j[7+k])return false;
  }
  for(unsigned i=14;i<Words;i++)if(j[i])return false;
  return true;
 }
 template<class IO> static bool Seal(IO& io,const uint32_t* j){
  for(unsigned i=0;i<Words;i++)if(io.Read(Job+i*4)!=j[i])return false;
  return true;
 }
 template<class IO> static bool Self(IO& io,unsigned role){
  const uint32_t node=io.Read(0xffb20044),shape=io.Read(0xffb20048);
  const unsigned xy=role?Worker(role):129U;
  return (node&0xfff)==xy && ((node>>12)&127)==17 && ((node>>19)&127)==12
   && (shape&0xffffff00)==0x10000 && io.Read(0xffb20044)==node && io.Read(0xffb20048)==shape;
 }
 template<class IO> static bool Pristine(IO& io,unsigned role){
  for(unsigned i=0;i<Words;i++)if(io.Read(Report+i*4))return false;
  if(io.Read(Ready) || io.Read(Start) || io.Read(Done))return false;
  uint32_t available=0;
  return Self(io,role) && Fifo(io,available) && available==0x10101010 && PioneerDramReadSweep::Idle(io);
 }
 template<class IO> static void Publish(IO& io,const uint32_t* r){
  for(unsigned i=1;i<Words;i++)io.Write(Report+i*4,r[i]);
  io.Fence();io.Write(Report,ReportMagic);(void)io.Read(Report);io.Fence();
 }
 static void Clear(uint32_t* r){volatile uint32_t* out=r;for(unsigned i=0;i<Words;i++)out[i]=0;}
 template<class IO> static bool Reader(IO& io,const uint32_t* j){
  const unsigned role=j[2],depth=j[6],bytes=16384,blocks=Span/bytes;
  if(!role || !Pristine(io,role))return false;
  io.Fence();io.Write(Ready,j[3]);(void)io.Read(Ready);io.Fence();
  bool released=false;
  for(unsigned poll=0;poll<PollLimit;poll++){
   const uint32_t start=io.Read(Start);
   if(start && start!=j[3])return false;
   if(start==j[3]){released=true;break;}
  }
  if(!released || !Seal(io,j) || !Self(io,role))return false;
  uint32_t available=0;
  if(!Fifo(io,available) || available!=0x10101010 || !PioneerDramReadSweep::Idle(io))return false;
  const uint32_t command[10]={Base,0,j[6+role],Raw,0,Worker(role),0,0,bytes,0};
  for(unsigned i=0;i<10;i++)io.Write(0xffb20000+i*4,command[i]);
  for(unsigned i=0;i<10;i++)if(io.Read(0xffb20000+i*4)!=command[i])return false;
  const uint32_t initial=io.Read(0xffb20208);unsigned issued=0;
  uint64_t began=0,ended=0;if(!io.Cycles(began))return false;
  for(unsigned sweep=0;sweep<j[4];sweep++)for(unsigned first=0;first<blocks;first+=depth){
   for(unsigned slot=0;slot<depth;slot++){
    if(!Slot(io))return false;
    const uint32_t source=Base+(first+slot)*bytes,dest=Raw+slot*bytes;
    io.Write(0xffb20000,source);io.Write(0xffb2000c,dest);
    if(io.Read(0xffb20000)!=source || io.Read(0xffb2000c)!=dest)return false;
    io.Write(0xffb20040,1);if(io.Read(0xffb20040)>1)return false;
    io.Fence();issued++;
   }
   if(!Drain(io,initial,issued,depth))return false;
  }
  if(!io.Cycles(ended) || ended<=began || !Fifo(io,available) || available!=0x10101010
   || !PioneerDramReadSweep::Idle(io) || uint32_t(io.Read(0xffb20208)-initial)!=blocks*j[4]
   || !Seal(io,j) || !Self(io,role) || io.Read(Start)!=j[3] || io.Read(Ready)!=j[3] || io.Read(Done))return false;
  uint32_t r[Words];Clear(r);const uint64_t elapsed=ended-began;
  r[1]=Version;r[2]=role;r[3]=j[3];r[4]=j[4];r[5]=depth;r[6]=Span*j[4];
  r[8]=uint32_t(began);r[9]=uint32_t(began>>32);r[10]=uint32_t(ended);r[11]=uint32_t(ended>>32);
  r[12]=uint32_t(elapsed);r[13]=uint32_t(elapsed>>32);r[14]=j[6+role];r[15]=issued;
  r[16]=io.Read(Raw);r[17]=io.Read(Raw+depth*bytes-4);
  Publish(io,r);io.Write(Done,j[3]);(void)io.Read(Done);io.Fence();return true;
 }
 struct CoordinatorLink {
  uint32_t tracked[9]; // Begin fills every word before the first Packet.
  static unsigned Offset(unsigned i){
   // ACK,read response,read flits,accepted,read sent,write flits,write sent,
   // write started,read started. All are documented master counters.
   const unsigned index[9]={1,2,3,4,5,8,10,12,14};return 0x200+index[i]*4;
  }
  template<class IO> void Begin(IO& io){for(unsigned i=0;i<9;i++)tracked[i]=io.Read(0xffb20000+Offset(i));}
  template<class IO> bool Packet(IO& io,unsigned role,bool write,unsigned remote,unsigned bytes){
   if(!role || role>Peers || (write?(remote!=Start || bytes!=4):
    !((remote==Ready || remote==Done) && bytes==4) && !((remote==Job || remote==Report) && bytes==Words*4)))return false;
   uint32_t available=0;
   if(!Self(io,0) || !Fifo(io,available) || available!=0x10101010 || !PioneerDramReadSweep::Idle(io))return false;
   const unsigned local=write?Payload:Scratch;
   uint32_t expected[9];
   for(unsigned i=0;i<9;i++){
    if(io.Read(0xffb20000+Offset(i))!=tracked[i])return false;
    const unsigned delta=write?(i==0 || i==3 || i==5 || i==6 || i==7):
     (i==2?(bytes+63)/64:unsigned(i==1 || i==3 || i==4 || i==8));
    expected[i]=tracked[i]+delta;
   }
   const uint32_t command[10]={write?local:remote,0,write?129U:Worker(role),
    write?remote:local,0,write?Worker(role):129U,0,write?0x212U:0,bytes,0};
   for(unsigned i=0;i<10;i++)io.Write(0xffb20000+i*4,command[i]);
   for(unsigned i=0;i<10;i++)if(io.Read(0xffb20000+i*4)!=command[i])return false;
   io.Fence();io.Write(0xffb20040,1);if(io.Read(0xffb20040)>1)return false;io.Fence();
   bool finished=false;
   for(unsigned poll=0;poll<PollLimit;poll++){
    if(!Fifo(io,available))return false;
    if(available!=0x10101010)continue;
    const uint32_t busy=io.Read(0xffb20040);if(busy>1)return false;
    bool all=!busy;
    for(unsigned i=0;i<9;i++){
     const uint32_t v=io.Read(0xffb20000+Offset(i));
     if(uint32_t(v-tracked[i])>uint32_t(expected[i]-tracked[i]))return false;
     all&=v==expected[i];
    }
    if(all && PioneerDramReadSweep::Idle(io)){finished=true;break;}
   }
   if(!finished)return false;
   for(unsigned i=0;i<10;i++)if(io.Read(0xffb20000+i*4)!=command[i])return false;
   for(unsigned i=0;i<9;i++)tracked[i]=expected[i];
   io.Fence();return true;
  }
 };
 template<class IO> static bool Coordinator(IO& io,const uint32_t* j){
  if(j[2] || !Pristine(io,0))return false;
  CoordinatorLink link;link.Begin(io);
  for(unsigned role=1;role<=Peers;role++){
   bool ready=false;
   for(unsigned poll=0;poll<PollLimit;poll++){
    if(!link.Packet(io,role,false,Ready,4))return false;
    const uint32_t v=io.Read(Scratch);if(v && v!=j[3])return false;
    if(v==j[3]){ready=true;break;}
   }
   if(!ready || !link.Packet(io,role,false,Job,Words*4))return false;
   for(unsigned i=0;i<Words;i++)if(io.Read(Scratch+i*4)!=(i==2?role:j[i]))return false;
  }
  if(!Seal(io,j))return false;
  io.Write(Payload,j[3]);(void)io.Read(Payload);io.Fence();
  uint32_t r[Words];Clear(r);uint64_t began=0,ended=0,lastRelease=0;
  if(!io.Cycles(began))return false;
  for(unsigned role=1;role<=Peers;role++){
   if(!link.Packet(io,role,true,Start,4))return false;
   uint64_t now=0;if(!io.Cycles(now) || now<=began || now<lastRelease)return false;
   const uint64_t delta=now-began;lastRelease=now;
   r[16+role*2]=uint32_t(delta);r[17+role*2]=uint32_t(delta>>32);
  }
  unsigned mask=0;
  for(unsigned poll=0;poll<PollLimit && mask!=127;poll++)for(unsigned role=1;role<=Peers;role++){
   if(mask&(1U<<(role-1)))continue;
   if(!link.Packet(io,role,false,Done,4))return false;
   const uint32_t v=io.Read(Scratch);if(v && v!=j[3])return false;
   if(v==j[3])mask|=1U<<(role-1);
  }
  if(mask!=127 || !io.Cycles(ended) || ended<=lastRelease || !Seal(io,j))return false;
  // End was sampled BEFORE report readout. It includes all release packets
  // and completion observation, but not upload, post-run checking or restore.
  for(unsigned role=1;role<=Peers;role++){
   if(!link.Packet(io,role,false,Report,Words*4))return false;
   uint32_t p[Words];for(unsigned i=0;i<Words;i++)p[i]=io.Read(Scratch+i*4);
   const uint64_t a=(uint64_t(p[9])<<32)|p[8],b=(uint64_t(p[11])<<32)|p[10];
   if(p[0]!=ReportMagic || p[1]!=Version || p[2]!=role || p[3]!=j[3] || p[4]!=j[4] || p[5]!=j[6]
    || p[6]!=Span*j[4] || p[7] || b<=a || ((uint64_t(p[13])<<32)|p[12])!=b-a
    || p[14]!=j[6+role] || p[15]!=(Span/16384)*j[4])return false;
   for(unsigned i=18;i<Words;i++)if(p[i])return false;
   for(unsigned i=0;i<Words;i++)io.Write(PeerReports+(role-1)*Words*4+i*4,p[i]);
  }
  const uint64_t total=uint64_t(Span)*j[4]*Peers,elapsed=ended-began;
  r[1]=Version;r[2]=0;r[3]=j[3];r[4]=j[4];r[5]=j[6];r[6]=uint32_t(total);r[7]=uint32_t(total>>32);
  r[8]=uint32_t(began);r[9]=uint32_t(began>>32);r[10]=uint32_t(ended);r[11]=uint32_t(ended>>32);
  r[12]=uint32_t(elapsed);r[13]=uint32_t(elapsed>>32);r[15]=(Span/16384)*j[4]*Peers;
  Publish(io,r);return true;
 }
 template<class IO> static bool Run(IO& io){
  uint32_t j[Words];for(unsigned i=0;i<Words;i++)j[i]=io.Read(Job+i*4);
  if(!Shape(j))return false; // no MMIO/clock before full DATA shape acceptance
  return j[2]?Reader(io,j):Coordinator(io,j);
 }
};
#endif
