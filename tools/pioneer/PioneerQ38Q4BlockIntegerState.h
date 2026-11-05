/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Q4BlockIntegerAffine.h"
#ifndef PIONEER_Q38_Q4_BLOCK_INTEGER_STATE_H
#define PIONEER_Q38_Q4_BLOCK_INTEGER_STATE_H
#include "PioneerQ38VectorQ4Metadata.h"

// Emitted offboard prerequisite,not a worker/native memory grant. Caller
// owns completed raw metadata,validated inputs and the whole Dst/LReg scope.
// Additional logical FP32 rows112..127 MUST be initialized through Init;
// old accumulator108..111 and metadata128..191 remain separate. No source
// bank publication,FPU dot,NoC or source authentication is supplied here.
struct PioneerQ38Q4BlockIntegerState {
 enum : unsigned {Base=112,Minimum=116,FirstHalf=120,ScratchRows=16};
 unsigned initialized=0,active=0,next=0,failed=0;
 bool Refuse(){failed=1;return false;}
 static bool Scale(unsigned scale){
  const unsigned magnitude=scale&0x7fffffff;
  return !magnitude || (magnitude>=0x2f800000 && magnitude<=0x47800000);
 }
 static unsigned SM(int value){return (value<0?0x80000000U:0)|unsigned(value<0?-value:value);}
 template<class E> bool Init(E& e){
  if(failed || active)return Refuse();
  e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
  e.Op(0xb2050000);e.Drain();
  e.Config(1,0x20200000);e.Config(49,0);e.Config(50,0x100);
  e.Config(64,0x100010);e.Config(65,16);e.Config(72,0);e.Config(73,0x13);e.Config(76,0x9fff);
  e.Config(84,0x00400000+(Base+4)*16);e.Op(0x42000091);e.Drain();
  e.Op(0xb2050004);e.Drain();initialized=1;return true;
 }
private:
 template<unsigned First,unsigned Parity,class E> static void StoreFactors(E& e){
  e.Op(0x90000000);e.Op(0x90000110);
  PioneerSfpuMove(e,4,1);
  PioneerSfpuLoad32(e,1,0);PioneerSfpuLoad32(e,2,0);PioneerSfpuLoad32(e,3,0);
  PioneerSfpuLoad32(e,5,0);PioneerSfpuLoad32(e,6,0);PioneerSfpuLoad32(e,7,0);
  e.Op(0x8c000000);
  e.Op(0x72030000|(128+First*8)|Parity);e.Op(0x72430000|(132+First*8)|Parity);
  e.Op(0x72130000|(136+First*8)|Parity);e.Op(0x72530000|(140+First*8)|Parity);
  e.Op(0x72230000|(144+First*8)|Parity);e.Op(0x72630000|(148+First*8)|Parity);
  e.Op(0x72330000|(152+First*8)|Parity);e.Op(0x72730000|(156+First*8)|Parity);
 }
 template<unsigned Parity,class E> static void MetadataParity(E& e){
  e.Op(0x700d0000|Parity);
  PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
  PioneerSfpuLoad32(e,4,0);PioneerSfpuLoad32(e,5,0);PioneerSfpuLoad32(e,6,0);PioneerSfpuLoad32(e,7,0);
  e.Op(0x8c000000);PioneerQ38VectorQ4Metadata::RawBases(e);
  e.Op(0x72430000|Base|Parity);e.Op(0x72530000|Minimum|Parity);
  e.Op(0x700d0000|4|Parity);e.Op(0x701d0000|8|Parity);
  PioneerQ38VectorQ4Metadata::Factors<0>(e);StoreFactors<0,Parity>(e);
  e.Op(0x700d0000|12|Parity);e.Op(0x701d0000|4|Parity);e.Op(0x702d0000|8|Parity);
  PioneerQ38VectorQ4Metadata::Factors<4>(e);StoreFactors<4,Parity>(e);
 }
 template<class E> static void Metadata(E& e,unsigned raw){
  e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
  e.Op(0xb2050000);e.Drain();
  e.Config(1,0xa03c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Config(73,0x13);
  e.Config(84,0x00400040);e.Config(76,(raw-16)/16);e.Op(0x42000081);e.Drain();
  e.Op(0xb2050004);e.Op(0x3700000f);e.Op(0x8a000002);e.Drain();
  MetadataParity<0>(e);MetadataParity<2>(e);e.Drain();
 }
 template<unsigned Parity,class E> static void PanelParity(E& e,unsigned panel,int sum){
  e.Op(0x70030000|32|Parity);
  e.Op(0x70130000|(128+(panel/2)*8)|Parity);
  e.Op(0x70230000|(132+(panel/2)*8)|Parity);
  PioneerSfpuLoad32(e,3,SM(sum*7));e.Op(0x90000330);
  PioneerQ38VectorAffineAdd(e,0,3,0); // reconstruct original qdot EXACTLY
  PioneerSfpuLoad32(e,3,SM(sum));e.Op(0x90000330);
  PioneerQ38Q4BlockIntegerAffine::Panel<Parity>(e);
 }
 template<unsigned Parity,class E> static void FinishParity(E& e,unsigned scale){
  e.Op(0x70030000|FirstHalf|Parity);
  PioneerQ38VectorAffineAdd(e,0,4+Parity,0);
  e.Op(0x70130000|Base|Parity);e.Op(0x70230000|Minimum|Parity);
  PioneerSfpuLoad32(e,3,0x80000000);PioneerSfpuXor(e,2,3);
  PioneerSfpuLoad32(e,3,scale);e.Op(0x70430000|108|Parity);
  PioneerQ38Q4BlockIntegerAffine::Finish<Parity>(e);
  e.Op(0x72030000|108|Parity);
 }
public:
 // FiniteBlock must already have accepted the actual completed raw tile.
 // No reuse of metadata or accumulator values from a previous block.
 template<class E> bool Begin(E& e,unsigned raw){
  if(failed || active || !initialized || !PioneerQ38VectorQ4Metadata::RawBase(raw))return Refuse();
  active=1;next=0;Metadata(e,raw);
  for(unsigned reg=4;reg<8;reg++)PioneerSfpuLoad32(e,reg,0);
  return true;
 }
 // Entry requires the real TWO-product FPU dot and source-bank release.
 // Both output parities consume the same original K16 scalar input sum.
 template<class E> bool Panel(E& e,unsigned panel,int sum){
  if(failed || !active || panel!=next || panel>=16 || sum<-2048 || sum>2048)return Refuse();
  if(panel==8){
   e.Op(0x72430000|FirstHalf);e.Op(0x72630000|FirstHalf|2);
   PioneerSfpuLoad32(e,4,0);PioneerSfpuLoad32(e,6,0);
  }
  PanelParity<0>(e,panel,sum);PanelParity<2>(e,panel,sum);
  e.Drain();next++;return true;
 }
 template<class E> bool Finish(E& e,unsigned scale){
  if(failed || !active || next!=16 || !Scale(scale))return Refuse();
  FinishParity<0>(e,scale);FinishParity<2>(e,scale);
  e.Drain();active=0;return true;
 }
};
#endif
