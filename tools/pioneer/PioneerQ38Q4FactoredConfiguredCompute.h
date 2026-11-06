/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Q4FactoredCompute.h" // repeats the separate diagnostic-role checks
#ifndef PIONEER_Q38_Q4_FACTORED_CONFIGURED_COMPUTE_H
#define PIONEER_Q38_Q4_FACTORED_CONFIGURED_COMPUTE_H
// Separate candidate: SAME arithmetic/issued operations, fewer configuration
// write/readback pairs. Not selected by the existing native113 or Work path.
// Own Config0 continuously from Begin through all blocks. No intervening
// foreign configuration writer is permitted, just as for the base producer.
struct PioneerQ38Q4FactoredConfiguredCompute: PioneerQ38Q4FactoredCompute {
private:
 // Only these fixed fields move to job initialization. Address/context words
 // 68/69/76/124 remain explicit; this is NOT a general MMIO register cache.
 static bool Fixed(unsigned i,unsigned v){
  switch(i){
   case 2:case 8:case 13:case 14:case 15:case 17:case 19:case 20:case 101:case 180:return v==0;
   case 12:return v==0x00100001;case 16:return v==1024;case 18:return v==4;
   case 24:return v==65535;case 28:return v==0x100;case 100:return v==0x1ffff;
   case 112:return v==0x00200015;case 113:return v==0x00010001;
   case 114:return v==1;case 115:return v==0;case 120:return v==0x25;case 121:return v==0xf000f;
   default:return false;
  }
 }
 template<class E> struct FixedIssue {
  E& e;
  __attribute__((always_inline)) inline void Op(unsigned v){e.Op(v);}
  __attribute__((always_inline)) inline void Drain(){e.Drain();}
  __attribute__((always_inline)) inline void Config(unsigned i,unsigned v){if(!Fixed(i,v))e.Config(i,v);}
 };
 // Scope is ONLY the native dot immediately after the ones dot and SFPU sum
 // extraction. All fields below are still the exact preceding matrix layout.
 // Pointer76 always changes; any unrecognized/different value is forwarded.
 static bool SameMatrix(unsigned i,unsigned v,unsigned input){
  if(Fixed(i,v))return true;
  switch(i){
   case 0:return v==0x42b6;case 1:return v==0x60ac0000;
   case 64:return v==0x01000016;case 65:return v==0x00040001;
   case 66:return v==1;case 67:return v==0;case 70:return v==8;case 71:return v==0;
   case 72:return v==0x26;case 73:return v==0x000f000f;
   case 84:return v==0x00400040;case 86:return v==0x01000100;
   case 124:return v==input;default:return false;
  }
 }
 template<class E> struct PairedIssue {
  E& e;unsigned input;
  __attribute__((always_inline)) inline void Op(unsigned v){e.Op(v);}
  __attribute__((always_inline)) inline void Drain(){e.Drain();}
  __attribute__((always_inline)) inline void Config(unsigned i,unsigned v){if(!SameMatrix(i,v,input))e.Config(i,v);}
 };
public:
 template<class E> static void Begin(E& e){
  PioneerQ38Q4FactoredCompute::Begin(e);
  e.Config(2,0);e.Config(8,0);e.Config(12,0x00100001);e.Config(13,0);e.Config(14,0);e.Config(15,0);
  e.Config(16,1024);e.Config(17,0);e.Config(18,4);e.Config(19,0);e.Config(20,0);e.Config(24,65535);
  e.Config(28,0x100);e.Config(100,0x1ffff);e.Config(101,0);e.Config(180,0);
  e.Config(112,0x00200015);e.Config(113,0x00010001);e.Config(114,1);e.Config(115,0);
  e.Config(120,0x25);e.Config(121,0xf000f);
 }
 template<class E> static bool Group(E& e,unsigned buffer,unsigned group){
  if(buffer>=2 || group>=8)return false; // ZERO IO
  FixedIssue<E> fixed{e};
  if(!F::PackCodes(fixed,group,buffer) || !F::Matrix<true>(fixed,F::Ones,group,buffer))return false;
  e.Op(0x8a001002);PioneerSfpuMove(e,7,15);PioneerSfpuAddI(e,7,-16);e.Op(0x7b000700);
  e.Op(0x70130020);
  PairedIssue<E> paired{e,(F::Input+buffer*0x10000+group*128)/16-1};
  if(!F::Matrix<true>(paired,F::Native,group,buffer))return false;
  for(unsigned half=0;half<2;half++)for(unsigned parity=0;parity<=2;parity+=2){
   const unsigned row=M::Row(group,half),acc=Accumulator+half*8;
   e.Op(0x70230000|row|parity);e.Op(0x70330000|(row+4)|parity);
   e.Op(0x70030000|(half?40:32)|parity);e.Op(0x70430000|acc|parity);
   PioneerQ38Q4FactoredBfp8::GroupAffine(e);
   e.Op(0x72430000|acc|parity);
  }
  e.Op(0x8a000002);e.Drain();return true;
 }
};
#endif
