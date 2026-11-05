/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_CORRECTED_MACHINE_H
#define PIONEER_Q38_EXACT_Q4_CORRECTED_MACHINE_H
#define PIONEER_Q38_EXACT_Q4_MACHINE_ONLY
#include "pioneer_q38_exact_q4_primary_test.cpp"
#include "../PioneerQ38ExactQ4Corrected.h"
// Independent functional ISA extension for no-flip partial SrcA writes,
// forced BFP2_B and explicit BF16 matrix format. NOT a cycle/timing model.
struct ExactCorrectedMachine:ExactPrimaryMachine {
 struct Transfer {bool active=false,flip=false;unsigned bank=0,row=0,count=0,due=0;float values[256]={};} pending[2];
 unsigned partial[2]={},completed[2]={},maskReads=0,sevenWrites=0,escapeWrites=0,fences=0;
 bool dirty=false;
 bool Pending()const{return pending[0].active || pending[1].active;}
 bool Clean()const{return !Pending() && !partial[0] && !partial[1] && NativeBfpMachine::Clean();}
 void Config(unsigned word,unsigned value){if(Pending())throw Unknown;NativeBfpMachine::Config(word,value);}
 void Seven(unsigned i,unsigned value){assert(i<32 && !Pending() && !mathPending);Write(0x90100+i*4,value);sevenWrites++;}
 void Escape(unsigned i,unsigned value){assert(i<16 && !Pending() && !mathPending);Write(0x90200+i*4,value);escapeWrites++;}
 void ObserveFence(){dirty=false;fences++;} // RISC ordering ONLY,never FPU completion
 void SealData(){assert(!Pending() && !mathPending);ObserveFence();}
 void Write(unsigned at,unsigned v){for(unsigned b=0;b<4;b++)l1[at+b]=uint8_t(v>>(8*b));dirty=true;}
 unsigned Mask(unsigned i){assert(i<8 && !Pending() && !mathPending);maskReads++;return Read(0x90300+i*4);}
 unsigned Read(unsigned at)const{return l1[at]|(unsigned(l1[at+1])<<8)|(unsigned(l1[at+2])<<16)|(unsigned(l1[at+3])<<24);}
 void StageCorrected(const uint8_t* primary,const uint8_t* mask,const std::array<int,16>& input) {
  assert(Clean());StagePrimary(primary,input);memcpy(l1.data()+0x90300,mask,32);
 }
 void Drain() {
  for(unsigned w=0;w<2;w++)if(pending[w].active) {
   if(dropProducer==w+1)throw Unknown;
   auto& p=pending[w];cycle=std::max(cycle,p.due);auto& s=source[w][p.bank];
   assert(!s.owner && !s.pending);
   for(unsigned i=0;i<p.count;i++)s.values[p.row+i/16][i%16]=p.values[i];
   if(!w) {
    partial[p.bank]|=1U<<(p.row/16);
    if(p.flip){assert(partial[p.bank]==7);completed[p.bank]=7;partial[p.bank]=0;}
   }
   if(p.flip){s.owner=true;unpackBank[w]^=1;}
   p.active=false;
  }
  NativeBfpMachine::Drain();
 }
 void UnpackCorrected(unsigned which,bool flip) {
  if(dirty)throw Unknown;
  assert(which<2 && thread[0]==0 && thread[5]==4);
  auto& p=pending[which];assert(!p.active);p=Transfer{};
  p.active=true;p.flip=flip;p.bank=unpackBank[which];p.due=cycle+delay[which];
  const auto& s=source[which][p.bank];if(s.owner || s.pending)throw Unknown;
  const unsigned base=64+which*48,format=config[base]&15,address=(config[base+12]+1)*16;
  assert(!xStart[which] && config[base+1]==0x00010001 && config[base+2]==1 && !config[base+3]
   && config[base+10]==0x17fff && !config[base+11] && !config[base+28]);
  if(which) {
   assert(flip && format==5 && config[base]==0x00800015 && config[base+8]==0x25
    && config[base+9]==0x000f000f && address==0x91000 && xEnd[which]==127 && !config[61]);
   p.count=128;p.row=0;
  } else {
   assert((format==7 || format==15) && config[base]==(0x01000010|format) && config[base+9]==0x000f010f
    && config[base+8]==(format==7?0x27U:15U) && xEnd[which]==255 && !config[49]
    && (config[84]>>16)==64 && ((config[84]&65535)%16)==0 && !(address&15));
   p.count=256;p.row=(config[84]&65535)/16-4;
   assert((p.row==0 && address==0x90000 && !flip && !partial[p.bank])
    || (p.row==16 && address==0x90100 && !flip && partial[p.bank]==1)
    || (p.row==32 && address==0x90200 && flip && partial[p.bank]==3));
   assert(format==(p.row==32?15U:7U) && config[50]==(p.row==32?0x17fU:0x181U));
  }
  for(unsigned i=0;i<p.count;i++) {
   float value;
   if(which)value=Float(uint32_t(l1[address+2*i]|(unsigned(l1[address+2*i+1])<<8))<<16);
   else {
    const unsigned width=format==7?4:2,datum=(l1[address+i*width/8]>>((i*width)%8))&((1U<<width)-1),mag=datum&((1U<<(width-1))-1);
    assert(datum!=(1U<<(width-1)));
    value=float(std::ldexp((datum>>(width-1)?-1.L:1.L)*mag,int(config[50]&255)-127-int(width-2)));
   }
   p.values[i]=value;
  }
  unpacks[which]++;
 }
 void MatrixCorrected(unsigned op) {
  assert(!Pending() && config[0]==0x42b5 && config[1]==0x60aa0000 && !config[2] && !config[3]
   && thread[2]==1 && thread[3]==1 && !thread[11] && !thread[13] && !thread[48]);
  assert(((op>>14)&3)==1 && !(op&0x003fbc00));
  auto& a=source[0][matrixBank[0]];auto& b=source[1][matrixBank[1]];
  if(!a.owner || !b.owner || completed[matrixBank[0]]!=7)throw Unknown;
  const unsigned ar=rwcA&0x38,br=rwcB&0x38,dr=op&1023,phase=(fidelity+thread[11])&3;
  assert((ar==0 || ar==16 || ar==32) && br==0 && dr==32 && phase==0);visits[phase]++;
  for(unsigned row=0;row<8;row++)for(unsigned n=0;n<16;n++) {
   long double sum=0;for(unsigned k=0;k<16;k++)sum+=static_cast<long double>(Part(b.values[row][k],phase,false))*Part(a.values[ar+k][n],phase,true);
   if(phase!=dropPhase)destination[dr+row][n]=float(float(sum)+(valid[dr+row]?destination[dr+row][n]:0));
  }
  for(unsigned row=0;row<8;row++)valid[dr+row]=true;
  if(!omitFlip)for(unsigned w=0;w<2;w++)if(op&(1U<<(22+w))) {
   if(!w)completed[matrixBank[w]]=0;source[w][matrixBank[w]].owner=false;matrixBank[w]^=1;
  }
  fidelity=(fidelity+((thread[29]>>13)&3))&3;matrixOps++;mathPending=true;
 }
 void Op(unsigned op) {
  if((op>>24)==0x42){cycle++;UnpackCorrected((op>>23)&1,bool(op&64));}
  else if((op>>24)==0x26){cycle++;MatrixCorrected(op);}
  else NativeBfpMachine::Op(op);
 }
 std::array<int,16> CorrectedResults(){if(!Clean())throw Unknown;return PrimaryResults();}
};
#endif
