/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRAM_WIDE_QUEUED_READ_SWEEP_H
#define PIONEER_DRAM_WIDE_QUEUED_READ_SWEEP_H
#include "PioneerDramQueuedReadSweep.h"
// Separate version3 candidate. Preserve the frozen16KiB v2 control above.
// Only the return working set grows:16 disjoint16KiB slots,unchanged8MiB
// READ-only source window. All packets remain <=16KiB. Not launch authority.
struct PioneerDramWideQueuedReadSweep:PioneerDramQueuedReadSweep {
 enum : uint32_t {Magic=0x33425244,ReportMagic=0x33524244,RawBytes=0x40000};
 static bool Shape(const uint32_t* job){
  if(job[0]!=Magic || job[1]!=3 || !job[2] || job[2]>256
   || (job[3]!=6 && job[3]!=10 && job[3]!=12 && job[3]!=14) || !job[5]
   || !job[6] || job[6]>16 || (job[6]&(job[6]-1)))return false;
  if((1U<<job[3])*job[6]>RawBytes)return false;
  const unsigned x=job[4]&63,y=job[4]>>6;
  if((x!=17 && x!=18) || (y!=12 && y!=15 && y!=18 && y!=21))return false;
  for(unsigned i=7;i<16;i++)if(job[i])return false;
  return true;
 }
 template<class IO> static bool Run(IO& io){
  uint32_t job[16],result[16];
  for(unsigned i=0;i<16;i++)job[i]=io.Read(Job+i*4);
  if(!Shape(job))return false;
  for(unsigned i=0;i<16;i++)if(io.Read(Report+i*4))return false;
  uint32_t available=0;
  if(!Fifo(io,available) || available!=0x10101010 || !PioneerDramReadSweep::Idle(io))return false;
  const unsigned bytes=1U<<job[3],blocks=Span>>job[3],depth=job[6],requests=blocks*job[2];
  const uint32_t command[10]={Base,0,job[4],Raw,0,129,0,0,bytes,0};
  for(unsigned i=0;i<10;i++)io.Write(0xffb20000+i*4,command[i]);
  for(unsigned i=0;i<10;i++)if(io.Read(0xffb20000+i*4)!=command[i])return false;
  const uint32_t initial=io.Read(0xffb20208);unsigned issued=0;
  uint64_t began=0,ended=0;if(!io.Cycles(began))return false;
  for(unsigned sweep=0;sweep<job[2];sweep++)for(unsigned first=0;first<blocks;first+=depth){
   for(unsigned slot=0;slot<depth;slot++){
    if(!Slot(io))return false;
    const uint32_t source=Base+((first+slot)<<job[3]),dest=Raw+(slot<<job[3]);
    io.Write(0xffb20000,source);io.Write(0xffb2000c,dest);
    if(io.Read(0xffb20000)!=source || io.Read(0xffb2000c)!=dest)return false;
    io.Write(0xffb20040,1);
    if(io.Read(0xffb20040)>1)return false; // ordered doorbell readback
    io.Fence();issued++;
   }
   if(!Drain(io,initial,issued,depth))return false;
  }
  if(!io.Cycles(ended) || ended<=began || !Fifo(io,available) || available!=0x10101010
   || !PioneerDramReadSweep::Idle(io) || uint32_t(io.Read(0xffb20208)-initial)!=requests)return false;
  for(unsigned i=0;i<16;i++)if(io.Read(Job+i*4)!=job[i])return false;
  const uint64_t elapsed=ended-began;
  result[1]=3;result[2]=job[5];result[3]=job[4];result[4]=bytes;
  result[5]=job[2];result[6]=Span*job[2];result[7]=requests;
  result[8]=uint32_t(began);result[9]=uint32_t(began>>32);
  result[10]=uint32_t(ended);result[11]=uint32_t(ended>>32);
  result[12]=uint32_t(elapsed);result[13]=uint32_t(elapsed>>32);
  const uint32_t last=Raw+(depth-1)*bytes;
  result[14]=io.Read(last);result[15]=io.Read(last+bytes-4);
  for(unsigned i=1;i<16;i++)io.Write(Report+i*4,result[i]);
  io.Fence();io.Write(Report,ReportMagic);(void)io.Read(Report);io.Fence();return true;
 }
};
#endif
