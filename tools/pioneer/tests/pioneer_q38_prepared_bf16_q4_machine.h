/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_BF16_Q4_MACHINE_H
#define PIONEER_Q38_PREPARED_BF16_Q4_MACHINE_H
#include "pioneer_q38_q4_factored_operator_machine.h"
#define PIONEER_Q38_PREPARED_BF16_Q4_OFFBOARD 1
#include "../PioneerQ38PreparedBf16Q4Compute.h"
// Extend the existing physical-Dst/metadata/SFPU observer only for the
// direct forced-exponent A256 and broadcast B16 descriptors. No timing,
// device ownership, NoC, or native integration is simulated by this test.
struct PreparedBf16Q4Machine:FactoredOperatorMachine {
 using P=PioneerQ38PreparedBf16Q4Compute;
 unsigned panels=0,maskReads=0,escapeWrites=0,sourceReads=0;
 bool scratchDirty=false,dropEscape=false,dropSeal=false,reportFaults=true;
 bool PreparedMode()const{return config[0]==0x42b5 && config[1]==0x60aa0000 && thread[5]==4;}
 void Write(unsigned a,unsigned v){
  if((a>=P::Ones-64 && a<P::Ones+128) || (a>=P::Escape-64 && a<P::Escape+4*128)){
   Need(!(a&3));for(unsigned i=0;i<4;i++)l1[a+i]=uint8_t(v>>(8*i));return;
  }
  FactoredOperatorMachine::Write(a,v);
 }
 unsigned MaskWord(unsigned b,unsigned g,unsigned face,unsigned word){
  Need(Clean() && b<2 && g<8 && face<4 && word<8);maskReads++;
  const unsigned a=0x8c000+b*0x10000+(face/2)*512+(g*2+face%2)*32+word*4;
  return l1[a]|unsigned(l1[a+1])<<8|unsigned(l1[a+2])<<16|unsigned(l1[a+3])<<24;
 }
 void EscapeWord(unsigned face,unsigned word,unsigned value){
  Need(Clean() && face<4 && word<16);scratchDirty=true;escapeWrites++;
  Write(P::Escape+face*P::EscapeStride+word*4,dropEscape?0:value);
 }
 void SealData(){if(!dropSeal)scratchDirty=false;}
 void PreparedUnpack(unsigned which){
  Need(!scratchDirty && !task.pending && !packPending && !mathPending && thread[0]==0 && thread[5]==4);
  const unsigned address=(config[which?124:76]+1)*16;
  auto& s=source[which][unpackBank[which]];Need(!s.pending && !s.owner);
  if(which){
   Need(config[112]==0x00100015 && config[113]==0x10001 && config[114]==1 && !config[115]
    && config[120]==0x25 && config[121]==0xf000f && !config[61] && !xStart[1] && xEnd[1]==15);
   const unsigned in=(panels-1)%10<2?(panels-1)%2:((panels-3)%4)%2;
   Need(address==P::F::Input+buffer*0x10000+(groups%8)*128+in*32);
   for(unsigned k=0;k<16;k++)s.values[0][k]=Float(uint32_t(l1[address+2*k]|unsigned(l1[address+2*k+1])<<8)<<16);
  }else{
   const bool escape=(config[64]&15)==15;const unsigned width=escape?2:4;
   Need(config[50]==(escape?0x17fU:0x181U) && config[64]==(escape?0x0100001fU:0x01000017U)
    && config[65]==0x10001 && config[66]==1 && !config[67] && config[70]==8 && !config[71]
    && config[72]==(escape?15U:0x27U) && config[73]==0xf010f && config[84]==0x400040
    && config[86]==0x1000100 && !config[49] && !xStart[0] && xEnd[0]==255);
   const unsigned within=panels%10;
   if(within<2)Need(address==P::Ones && !escape);
   else if(within<6)Need(address==P::Face(buffer,groups%8,(within-2)/2,(within-2)%2) && !escape);
   else Need(address==P::Escape+(within-6)*128 && escape);
   for(unsigned h=0;h<64;h++)Need(!l1[address-64+h]);
   for(unsigned i=0;i<256;i++){
    const unsigned code=(l1[address+i*width/8]>>((i*width)%8))&((1U<<width)-1),mag=code&((1U<<(width-1))-1);
    Need(code!=(1U<<(width-1)));
    s.values[i/16][i%16]=float(std::ldexp((code>>(width-1)?-1.L:1.L)*mag,int(config[50]&255)-127-int(width-2)));
   }
   panels++;sourceReads+=escape?64:128;
   if(panels%10==2 || panels%10==0)nativeInputs++;
  }
  s.pending=true;s.due=cycle+delay[which];unpacks[which]++;
 }
 void PreparedMatrix(unsigned op){
  Need(!Pending() && !packPending && thread[2]==1 && thread[3]==1 && thread[5]==4
   && !config[2] && !config[3] && !thread[11] && !thread[13] && !thread[48]
   && ((thread[29]>>13)&3)==2 && (op&~0xc003ffU)==0x26084000);
  auto& a=source[0][matrixBank[0]];auto& b=source[1][matrixBank[1]];Need(a.owner && b.owner);
  const unsigned dr=op&1023,phase=fidelity&3,within=(panels-1)%10;
  Need(!rwcA && !rwcB && (phase==0 || phase==2) && dr==(within<4 || within==6 || within==7?32U:40U));
  visits[phase]++;if(phase!=dropPhase)valid[dr]=true;
  if(!omitFlip)for(unsigned w=0;w<2;w++)if(op&(1U<<(22+w))){source[w][matrixBank[w]].owner=false;matrixBank[w]^=1;}
  fidelity=(fidelity+2)&3;matrixOps++;mathPending=true;
 }
 void ExecuteOp(unsigned op){
  if(PreparedMode() && (op==0x420000c1 || op==0x428000c1)){cycle++;PreparedUnpack((op>>23)&1);return;}
  if(PreparedMode() && (op>>24)==0x26){
   cycle++;PioneerBlackholeArithmeticObserver::Matrix(*this,op,true,[&]{PreparedMatrix(op);});
   const unsigned dr=op&1023;
   if(valid[dr]){
    for(unsigned c=0;c<16;c++)StoreFloat(dr,c,Bits(destination[dr][c]),cycle);
    defined[Adj(dr)]=defined[Adj(dr)+8]=true;
   }
   return;
  }
  Need((op>>24)!=0x41 && !((op>>24)==0x72 && (op&0x000f0000)==0x00050000)); // no PACK/INT8-code stores
  FactoredOperatorMachine::Op(op);
 }
 void Op(unsigned op){
  try{ExecuteOp(op);}catch(const Fault&){
   if(reportFaults)fprintf(stderr,"PREPARED_BF16_ISA_REFUSAL op%08x config0%08x config1%08x panels%u groups%u nativeInputs%u cycle%u\n",
    op,config[0],config[1],panels,groups,nativeInputs,cycle);throw;
  }
 }
};
struct PreparedBf16Q4Issue {
 PreparedBf16Q4Machine& m;
 void Op(unsigned v){m.Op(v);}void Config(unsigned i,unsigned v){m.Config(i,v);}void Drain(){m.Drain();}
 unsigned MaskWord(unsigned b,unsigned g,unsigned f,unsigned w){return m.MaskWord(b,g,f,w);}
 void EscapeWord(unsigned f,unsigned w,unsigned v){m.EscapeWord(f,w,v);}void SealData(){m.SealData();}
};
#endif
