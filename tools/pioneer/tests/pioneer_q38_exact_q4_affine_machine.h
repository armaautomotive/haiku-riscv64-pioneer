/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_AFFINE_MACHINE_H
#define PIONEER_Q38_EXACT_Q4_AFFINE_MACHINE_H
#include "pioneer_q38_exact_q4_corrected_machine.h"
#define PIONEER_Q38_VECTOR_AFFINE_TEST_MODEL_ONLY
#include "pioneer_q38_vector_affine_test.cpp"
#include "../PioneerQ38ExactQ4Affine.h"
// Functional instruction/physical-Dst observer,NOT timing or hardware proof.
// The previously qualified corrected FPU observer consumes actual L1 bytes;
// SFPU consumes the SAME physical Dst after real completion. No shortcut
// inserts expected dots or affine results into a simulated register.
struct ExactAffineVector:AffineVector {
 void Op(unsigned w) {
  const unsigned code=w>>24;
  if(w==0x90000660){
   // Additional exact SM32 operand used ONLY by the two-product fixture.
   // The original shared vector model's sum4 limit remains unchanged.
   for(unsigned lane=0;lane<32;lane++){
    const uint32_t bits=Read(6,lane),magnitude=bits&0x7fffffff;
    assert(magnitude<=14336 && (magnitude || !bits));
    lr[6][lane]=PioneerFmaBits(float(int(magnitude)*((bits>>31)?-1:1)));
   }
   ready[6]=cycle+1;cycle++;return;
  }
  if(code!=0x71 && code!=0x7e && code!=0x79 && code!=0x7a){AffineVector::Op(w);return;}
  unsigned d=(w>>4)&15,c=(w>>8)&15,b=(w>>12)&15,mode=(w>>16)&15;
  if(code==0x71)d=(w>>20)&15;
  assert(d<8);uint32_t values[32];
  for(unsigned lane=0;lane<32;lane++) {
   if(code==0x71){assert(mode==0 || mode==2 || mode==10);values[lane]=mode==0?(w&65535)<<16:mode==2?w&65535:(Read(d,lane)&0xffff0000)|(w&65535);}
   else if(code==0x7e){assert((w&15)==1 && b<8 && c<8);values[lane]=Read(b,lane)&Read(c,lane);}
   else if(code==0x79){assert((w&15)==4 && c<8);values[lane]=Read(d,lane)+Read(c,lane);}
   else {assert((w&15)==3 && c==d && (int(w<<8)>>20)==-31);values[lane]=(Read(d,lane)&0x80000000)?~0U:0;}
  }
  for(unsigned lane=0;lane<32;lane++)lr[d][lane]=values[lane];ready[d]=cycle+1;cycle++;
 }
};
struct ExactAffineMachine:ExactCorrectedMachine {
 using Kernel=PioneerQ38ExactQ4Affine;
 ExactAffineVector vector;
 uint16_t physical[1024][16]={};bool defined[1024]={},written[1024][16]={};unsigned ready[1024]={};
 unsigned metadataLoads=0,zeroLoads=0,affineStores=0,stageCalls=0;
 bool enabled=false,metaPending=false,skipZero=false,wrongDotCast=false,broadClear=false;
 bool sfpuPending=false;unsigned storeDue=0,metaOp=0,metaAddress=0,metaRow=0,metaCount=0;
 static unsigned Adj(unsigned row){return ((row&0x1f8)<<1)|(row&0x207);}
 bool Pending()const{return metaPending || ExactCorrectedMachine::Pending();}
 bool Clean()const{return !metaPending && !sfpuPending && ExactCorrectedMachine::Clean();}
 void StoreFloat(unsigned row,unsigned col,uint32_t bits,unsigned due) {
  const unsigned hi=Adj(row),lo=hi+8,h=bits>>16;
  physical[hi][col]=uint16_t((h&0x8000)|((h&127)<<8)|((h>>7)&255));physical[lo][col]=uint16_t(bits);
  written[hi][col]=written[lo][col]=true;ready[hi]=ready[lo]=due;defined[hi]=defined[lo]=true;
 }
 uint32_t FloatDst(unsigned row,unsigned col) {
  const unsigned hi=Adj(row),lo=hi+8;
  if(!defined[hi] || !defined[lo] || !written[hi][col] || !written[lo][col] || ready[hi]>cycle || ready[lo]>cycle)throw Unknown;
  const unsigned h=physical[hi][col];return (((h&0x8000)|((h&255)<<7)|((h>>8)&127))<<16)|physical[lo][col];
 }
 void Config(unsigned i,unsigned v){if(Pending() || sfpuPending)throw Unknown;ExactCorrectedMachine::Config(i,v);}
 void Drain() {
  if(metaPending){
   if(dropProducer==1)throw Unknown;cycle+=delay[0];
   for(unsigned i=0;i<metaCount;i++)StoreFloat(metaRow+i/16,i%16,metaOp==0x42000091?0:Read(metaAddress+i*4),cycle);
   metaPending=false;
  }
  for(unsigned r=0;r<8;r++)cycle=std::max(cycle,vector.ready[r]);
  if(sfpuPending)cycle=std::max(cycle,storeDue);
  sfpuPending=false;ExactCorrectedMachine::Drain();
 }
 void ZeroData(){for(unsigned i=0;i<1040;i+=4)Write(Kernel::Zero-16+i,0);for(unsigned i=0;i<16;i+=4){Write(Kernel::Coefficient-16+i,0);Write(Kernel::Minimum-16+i,0);}}
 bool StagePanel(unsigned p,unsigned& scale,int& sum) {
  if(!Clean())throw Unknown;stageCalls++;
  const unsigned at=Kernel::Source+p*Kernel::PacketBytes;
  for(unsigned i=0;i<32;i++)Write(0x90000+i*4,Read(at+i*4));
  for(unsigned i=0;i<8;i++)Write(0x90300+i*4,Read(at+128+i*4));
  for(unsigned r=0;r<8;r++)for(unsigned i=0;i<8;i++)Write(0x91000+r*32+i*4,Read(at+160+i*4));
  for(unsigned i=0;i<16;i++){Write(Kernel::Coefficient+i*4,Read(at+192+i*4));Write(Kernel::Minimum+i*4,Read(at+256+i*4));}
  scale=Read(at+320);sum=int(Read(at+324));return true;
 }
 void FloatUnpack(unsigned op) {
  if(Pending() || mathPending || sfpuPending || dirty)throw Unknown;
  for(const auto& bank:source[0])if(bank.owner || bank.pending)throw Unknown;
  assert(thread[5]==0 && config[1]==0x20200000 && config[50]==0x100 && config[64]==0x100010 && config[72]==0
   && config[73]==0x13 && config[86]==0x100010 && !config[49] && !xStart[0]);
  const unsigned row=(config[84]&65535)/16-4,address=(config[76]+1)*16,count=xEnd[0]+1;
  assert((config[84]>>16)==64 && !(address&15));
  if(op==0x42000091){assert(count==256 && config[65]==16 && row==96 && address==Kernel::Zero);zeroLoads++;if(skipZero)return;}
  else {assert(op==0x42000081 && count==16 && config[65]==1 && (row==96 || row==100)
    && address==(row==96?Kernel::Coefficient:Kernel::Minimum));metadataLoads++;}
  metaPending=true;metaOp=op;metaAddress=address;metaRow=row;metaCount=count;
 }
 void Sfpu(unsigned op) {
  if(Pending() || mathPending || !enabled)throw Unknown;
  assert(thread[5]==4 && config[1]==0x60aa0000 && config[3]==0);
  vector.cycle=cycle;const unsigned code=op>>24;
  if(code==0x70 || code==0x72) {
   unsigned reg=(op>>20)&15,mode=(op>>16)&15,address=op&1023;assert(reg<8 && !(address&1) && !(op&0xfc00) && mode==3);
   for(unsigned lane=0;lane<32;lane++) {
    const unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+bool(address&2);
    if(code==0x70){assert((reg==0 && row>=32 && row<36) || (reg==1 && row>=96 && row<100)
      || (reg==2 && row>=100 && row<104) || (reg==5 && row>=108 && row<112));vector.lr[reg][lane]=FloatDst(row,col);}
    else {assert(!reg && row>=108 && row<112);if(!defined[Adj(row)] || !defined[Adj(row)+8])throw Unknown;StoreFloat(row,col,vector.Read(reg,lane),cycle+1);}
   }
   if(code==0x70){vector.ready[reg]=cycle+1;if(!reg && wrongDotCast)for(unsigned lane=0;lane<32;lane++)vector.lr[0][lane]=Bits(float(int(vector.lr[0][lane]&0x7fffffff))*(vector.lr[0][lane]>>31?-1:1));}
   else {affineStores++;sfpuPending=true;storeDue=cycle+1;}
  }else vector.Op(op);
  if(!vector.okay)throw Unknown;
 }
 void Op(unsigned op) {
  const unsigned code=op>>24;
  if(op==0x5120000a || op==0x5420000f){cycle++;assert(thread[5]==0);return;} // metadata Y/Z/W counters zero
  if(code==0x42 && thread[5]==0){cycle++;FloatUnpack(op);return;}
  if(code==0x10 && op!=0x10180000){
   cycle++;if(Pending() || sfpuPending || mathPending)throw Unknown;
   assert(op==0x10080004 || op==0x10080005);
   if(broadClear)for(auto& value:defined)value=false;
   for(unsigned row=32;row<48;row++)valid[row]=false;
   for(unsigned row=op==0x10080004?64:80;row<(op==0x10080004?80U:96U);row++)defined[row]=false;
   return;
  }
  if(op==0x8a000002){cycle++;assert(!Pending() && !mathPending);enabled=true;return;}
  if(code==0x70 || code==0x71 || code==0x72 || code==0x7e || code==0x79 || code==0x7a || code==0x90 || code==0x85 || code==0x86 || code==0x8f){cycle++;Sfpu(op);return;}
  if(sfpuPending || metaPending)throw Unknown;
  ExactCorrectedMachine::Op(op);
  if(code==0x26)for(unsigned row=32;row<40;row++)for(unsigned col=0;col<16;col++)StoreFloat(row,col,Bits(destination[row][col]),cycle);
 }
 std::array<uint32_t,16> Results(){if(!Clean())throw Unknown;std::array<uint32_t,16> result;for(unsigned n=0;n<16;n++)result[n]=FloatDst(108,n);outputReads+=16;return result;}
};
// Independent original q and scalar FP32 reference is VALIDATION ONLY.
// Neither this fixture nor its expected results is linked into the RV image.
static std::array<uint32_t,16> ExactAffineFixture(ExactAffineMachine& m,unsigned width,unsigned seed) {
 assert(PioneerQ38ExactQ4Affine::Width(width) && m.Clean());std::array<float,16> result{};
 for(unsigned p=0;p<width/16;p++) {
  const unsigned at=PioneerQ38ExactQ4Affine::Source+p*352;for(unsigned i=0;i<352;i++)m.l1[at+i]=0;
  int dot[16]={},sum=0;
  for(unsigned k=0;k<16;k++) {
   const int x=int((seed*29+p*13+k*37)&255)-128;sum+=x;
   const unsigned bits=ExactAffineMachine::Bits(float(x));assert(!(bits&65535));m.l1[at+160+k*2]=uint8_t(bits>>16);m.l1[at+161+k*2]=uint8_t(bits>>24);
   for(unsigned n=0;n<16;n++) {
    const unsigned q=(seed+p*11+k*17+n*3)&15,i=k*16+n;const int z=int(std::min(q,14U))-7;
    m.l1[at+i/2]|=uint8_t((z<0?unsigned(-z)|8:unsigned(z))<<((i&1)*4));
    m.l1[at+128+i/8]|=uint8_t(unsigned(q==15)<<(i&7));dot[n]+=int(q)*x;
   }
  }
  unsigned scale=(95+((seed+p/16)%49))<<23;scale|=((seed+p/16)*7919)&0x7fffff;if((scale>>23)==143)scale&=~0x7fffffU;
  if((seed+p/16)%19==0)scale=0x80000000;else if((seed+p/16)&1)scale|=0x80000000;
  m.Write(at+320,scale);m.Write(at+324,unsigned(sum));
  for(unsigned n=0;n<16;n++) {
   const unsigned group=p/2;uint32_t coefficient,minimum;
   const unsigned half=((seed+group+n)%31==0?0U:1U+(seed*131+group*313+n*71)%0x7bff)|(((seed+group+n)&1)<<15);
   const unsigned minHalf=((seed+group+n)%23==0?0x8000U:1U+(seed*173+group*191+n*43)%0x7bff);
   assert(PioneerQ38BinaryScale(uint16_t(half),int((seed+group*3+n)&63),coefficient));
   assert(PioneerQ38BinaryScale(uint16_t(minHalf),-int((seed*7+group+n*3)&63),minimum));
   m.Write(at+192+n*4,coefficient);m.Write(at+256+n*4,minimum);
   const float product=(m.Float(coefficient)*m.Float(scale))*float(dot[n]);
   const float correction=(m.Float(minimum)*m.Float(scale))*float(sum);
   result[n]=result[n]+(product+correction);assert(std::isfinite(result[n]));
  }
 }
 m.ObserveFence();std::array<uint32_t,16> words;for(unsigned n=0;n<16;n++)words[n]=m.Bits(result[n]);return words;
}
#endif
