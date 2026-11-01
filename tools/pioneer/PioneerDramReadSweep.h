/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRAM_READ_SWEEP_H
#define PIONEER_DRAM_READ_SWEEP_H
#ifdef PIONEER_DRAM_SWEEP_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
#else
#include <stdint.h>
#endif
// OFFLINE diagnostic leaf, NOT selected by any driver. A future native owner
// must reserve the SAME complete 8MiB bank window, bind the firmware-derived
// route, save ALL modified L1/NIU words, exclude other owners, and prove the
// actual STOP/HOLD/restore lifecycle. A valid frame is DATA, not that authority.
// Fixed logical worker(1,2)/single outstanding READ: NOT the card's peak
// bandwidth test. The command's return-coordinate word is fixed to1|(2<<6).
struct PioneerDramReadSweep {
 enum : uint32_t { Magic=0x31425244, ReportMagic=0x31524244,
  Base=0x04000000, Span=0x00800000, Raw=0x90000, Job=0x74000,
  Report=0x74100, PollLimit=65536 };
 static bool Shape(const uint32_t* job) {
  if(job[0]!=Magic || job[1]!=1 || !job[2] || job[2]>256
   || (job[3]!=6 && job[3]!=10 && job[3]!=12 && job[3]!=14)
   || !job[5])return false;
  const unsigned x=job[4]&63,y=job[4]>>6;
  if((x!=17 && x!=18) || (y!=12 && y!=15 && y!=18 && y!=21))return false;
  for(unsigned i=6;i<16;i++)if(job[i])return false;
  return true;
 }
 template<class IO> static bool Idle(IO& io) {
  for(unsigned i=0;i<4;i++)if(io.Read(0xffb20040+i*0x800))return false;
  for(unsigned i=0;i<32;i++)if(io.Read(0xffb20240+i*4)&255)return false;
  return true;
 }
 template<class IO> static bool Run(IO& io) {
  uint32_t job[16],result[16]; // every published word1..15 is assigned below
  for(unsigned i=0;i<16;i++)job[i]=io.Read(Job+i*4);
  if(!Shape(job))return false; // inert: no NIU/CSR read or write
  for(unsigned i=0;i<16;i++)if(io.Read(Report+i*4))return false;
  if(!Idle(io))return false;
  const unsigned bytes=1U<<job[3],blocks=Span>>job[3],requests=blocks*job[2];
  const uint32_t command[10]={Base,0,job[4],Raw,0,129,0,0,bytes,0};
  for(unsigned i=0;i<10;i++)io.Write(0xffb20000+i*4,command[i]);
  for(unsigned i=0;i<10;i++)if(io.Read(0xffb20000+i*4)!=command[i])return false;
  uint32_t responses=io.Read(0xffb20208),initial=responses;
  uint64_t began=0,ended=0;
  if(!io.Cycles(began))return false;
  for(unsigned sweep=0;sweep<job[2];sweep++)for(unsigned at=0;at<Span;at+=bytes) {
   const uint32_t source=Base+at;
   io.Write(0xffb20000,source);
   if(io.Read(0xffb20000)!=source)return false;
   io.Write(0xffb20040,1);(void)io.Read(0xffb20040);io.Fence();
   bool completed=false;
   for(unsigned poll=0;poll<PollLimit;poll++) {
    const uint32_t busy=io.Read(0xffb20040);
    if(busy>1)return false;
    const uint32_t count=io.Read(0xffb20208),outstanding=io.Read(0xffb20240)&255;
    if(uint32_t(count-responses)>1)return false;
    if(!busy && count==uint32_t(responses+1) && !outstanding){completed=true;break;}
   }
   if(!completed)return false; // uncertainty: caller parks, NEVER another packet
   responses++;io.Fence();
  }
  if(!io.Cycles(ended) || ended<=began || !Idle(io))return false;
  if(uint32_t(io.Read(0xffb20208)-initial)!=requests)return false;
  for(unsigned i=0;i<16;i++)if(io.Read(Job+i*4)!=job[i])return false;
  // Cycles cover real READ issue/waits/fences, NOT upload or payload checking.
  // Samples are DATA only: NOT an 8MiB integrity proof or a completion lease.
  const uint64_t elapsed=ended-began;
  result[1]=1;result[2]=job[5];result[3]=job[4];result[4]=bytes;
  result[5]=job[2];result[6]=Span*job[2];result[7]=requests;
  result[8]=uint32_t(began);result[9]=uint32_t(began>>32);
  result[10]=uint32_t(ended);result[11]=uint32_t(ended>>32);
  result[12]=uint32_t(elapsed);result[13]=uint32_t(elapsed>>32);
  result[14]=io.Read(Raw);result[15]=io.Read(Raw+bytes-4);
  for(unsigned i=1;i<16;i++)io.Write(Report+i*4,result[i]);
  io.Fence();io.Write(Report,ReportMagic);(void)io.Read(Report);io.Fence();
  return true;
 }
};
#endif
