/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Q4FactoredBfp8.h" // repeats the separate-role checks
#ifndef PIONEER_Q38_Q4_FACTORED_TILE_H
#define PIONEER_Q38_Q4_FACTORED_TILE_H
#include "PioneerQ38NativeBfpMatvec.h"
// OFFBOARD instruction producer,NOT a native owner/returning worker.
// Original raw byte-major panels remain at90000,private BF16 vectors96000.
// NEW owned temporary native tiles88000/88800 require a NEW memory journal.
// Caller owes exclusive idle/reset source banks and genuine original bytes.
// The only scalar numeric read is the completed ON-CARD ones-matrix sum.
// No scalar learned/input payload reads,host products or CPU fallback.
struct PioneerQ38Q4FactoredTile {
 enum : unsigned {Native=0x88000,Ones=0x88800,Raw=0x90000,Input=0x96000,TileBytes=1088};
 template<class IO> static void Constants(IO& io){
  for(unsigned tile=0;tile<2;tile++){
   const unsigned base=tile?Ones:Native;
   for(unsigned a=base-16;a<base;a+=4)io.Write(a,0);
   for(unsigned a=base;a<base+64;a+=4)io.Write(a,0x82828282);
  }
  for(unsigned a=Ones+64;a<Ones+TileBytes;a+=4)io.Write(a,0x08080808);
 }
 template<class E> static void Begin(E& e){PioneerQ38NativeBfpMatvec::Setup(e,8);}
 template<class E> static bool PackCodes(E& e,unsigned group){
  if(group>=8)return false; // ZERO IO
  e.Op(0x3700000f);e.Op(0xb2050000);e.Drain();
  e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(66,1);
  e.Config(67,0);e.Config(70,8);e.Config(71,0);e.Config(72,14);e.Config(73,0x13);
  e.Config(86,0x100010);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);e.Drain();
  for(unsigned face=0;face<4;face++){
   e.Config(76,(Raw+((group/2)*4+face)*320)/16-1);
   e.Config(84,0x00400000+(face*16+4)*16);
   e.Op(0x42000081);e.Drain(); // raw Dst ONLY,no dummy SrcA publication
  }
  e.Op(0x8a000002);PioneerQ38Q4FactoredBfp8::InitCodes(e);
  for(unsigned at=0;at<64;at+=4)for(unsigned parity=0;parity<=2;parity+=2){
   e.Op(0x700d0000|at|parity);
   if(group&1)PioneerQ38Q4FactoredBfp8::Codes<true>(e);
   else PioneerQ38Q4FactoredBfp8::Codes<false>(e);
   e.Op(0x72050000|(64+at)|parity); // INT8 Dst16,NOT BF16 or FP32
  }
  e.Drain();
  // Pinned cfg81989dcdb:raw INT8->INT8 preserves positive magnitude bytes.
  // Pack context0 aliases some UNPACK0 words;restore them BEFORE UNPACK.
  // Existing global Dstacc override otherwise wins over In_data_format.
  // Force INT8 intermediate (14) while PACK runs;Matrix restores FP32.
  e.Config(0,0x7ab6);e.Config(2,0);e.Config(8,0);e.Config(12,0x00100001);e.Config(13,0);
  e.Config(14,0);e.Config(15,0);e.Config(16,64*16);e.Config(17,0);
  e.Config(18,4);e.Config(19,0);e.Config(20,0);e.Config(24,65535);
  e.Config(28,0x00041004);e.Config(68,0);e.Config(69,(Native+64)/16);
  e.Config(70,0x9ee1);e.Config(71,65535);e.Config(100,0x1ffff);e.Config(101,0);e.Config(180,0);
  e.Op(0xb2250004); // PACK AddrMod0:Ysrc+=4,all other increments0
  e.Op(0x5e803c00);e.Op(0x5180000b);e.Op(0x5480000f);
  for(unsigned rows=0;rows<64;rows+=4)e.Op(rows==60?0x41000001:0x41000000);
  // C3 explicitly waits for PACK,missing from the old generic1816 mask.
  // A following generic drain cannot replace this latched wait.
  e.Op(0xa210181e);e.Drain(); // LAST,NEVER Flush (Blackhole hazard)
  return true;
 }
 template<class E> static bool Matrix(E& e,unsigned tile,unsigned group){
  if((tile!=Native && tile!=Ones) || group>=8)return false;
  e.Op(0xb2050004);e.Drain();
  // q<=15 and the ones tile have <=4 significant bits:the FPU's leading
  // five SrcA bits are exact. Need B high/low phases0/2,NOT A-low1/3.
  // This is NOT permission to use two phases for arbitrary BFP8 weights.
  e.Op(0xb21d4000);
  e.Config(0,0x42b6);e.Config(1,0x60ac0000);
  e.Config(64,0x01000016);e.Config(65,0x00040001);e.Config(66,1);e.Config(67,0);
  e.Config(70,8);e.Config(71,0);e.Config(72,0x26);e.Config(73,0x000f000f);
  e.Config(76,tile/16-1);e.Config(84,0x00400040);e.Config(86,0x01000100);
  e.Config(112,0x00200015);e.Config(113,0x00010001);e.Config(114,1);e.Config(115,0);
  e.Config(120,0x25);e.Config(121,0x000f000f);e.Config(124,(Input+group*128)/16-1);
  e.Op(0x3700000f);e.Op(0x10180000);e.Op(0x5e2ffc00);e.Op(0x5e407c00);
  e.Op(0x5160000b);e.Op(0x5460000f);e.Op(0x420000c1);e.Op(0x428000c1);e.Drain();
  for(unsigned face=0;face<4;face++){
   if(face){e.Op(0x37040201);e.Op(0x37040201);}
   e.Op(face&1?0x3700040a:0x3700000a);
   for(unsigned phase=0;phase<2;phase++)e.Op(0x26084000|(face<2?32:40)|((face==3 && phase==1)?0x00c00000:0));
  }
  e.Drain();return true;
 }
 template<class E,class IO> static bool Group(E& e,IO& io,unsigned group,unsigned& sum){
  if(group>=8)return false; // ZERO IO
  if(!PackCodes(e,group) || !Matrix(e,Ones,group))return false;
  sum=io.Read(0xffbd8800); // genuine completed Dst32[row32][0],on card
  return Matrix(e,Native,group); // qdot remains in genuine Dst32/40
 }
};
#endif
