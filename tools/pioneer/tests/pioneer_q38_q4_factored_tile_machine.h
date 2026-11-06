/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FACTORED_TILE_MACHINE_H
#define PIONEER_Q38_Q4_FACTORED_TILE_MACHINE_H
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#include "../PioneerQ38Q4FactoredTile.h"
#include "pioneer_q38_q4_fpu_pipeline_machine.h"
// Extend the EXISTING physical-Dst/UNPACK/FPU observer with ONLY the new
// raw INT8 PACK subset. This is not hardware,clock or native-owner proof.
struct FactoredTileMachine:Q4FpuPipelineMachine {
 using F=PioneerQ38Q4FactoredTile;
 unsigned packs=0,packY=0,packX=99,packEnd=99,packBytes=0,packDelay=0,packDue=0,packClosures=0;
 unsigned nativeInputs=0,groups=0,sumReads=0,codeStores=0,packWaits=0;
 bool packOpen=false,packLast=false,packPending=false,packWait=false,missingPack=false;
 bool fourPhaseReference=false;
 std::array<uint8_t,1024> pendingBytes{};
 unsigned skippedPack=~0U,skippedStore=~0U;bool skipPackWait=false,skipLast=false,forceTranspose=false;
 static void Need(bool okay){if(!okay)throw Fault{};}
 bool RawPublishesLast()const override{return false;}
 bool NativeMode()const{return config[0]==0x42b6 && config[1]==0x60ac0000;}
 void Config(unsigned i,unsigned v){Need(!packPending);Q4FpuPipelineMachine::Config(i,v);}
 void Drain(){
  Q4FpuPipelineMachine::Drain();
  if(packPending){
   Need(packWait && packLast && packBytes==1024 && !missingPack);
   cycle=std::max(cycle,packDue);
   std::copy(pendingBytes.begin(),pendingBytes.end(),l1.begin()+F::Native+64);
   packPending=packOpen=packLast=packWait=false;packBytes=0;packClosures++;
  }
 }
 void Pack(unsigned op){
  Need(!Pending() && !mathPending && (op==0x41000000 || op==0x41000001));
  // Independent decoded global-override rule,not just local config70.
  const unsigned intermediate=config[0]&0x4000?(config[0]>>10)&15:(config[70]>>8)&15;
  Need(intermediate==14);
  Need(config[2]==0 && config[8]==0 && config[12]==0x100001 && !config[13] && !config[14] && !config[15]);
  Need(config[16]==1024 && config[17]==0 && config[18]==4 && config[19]==0 && config[20]==0 && config[24]==65535);
  Need(config[28]==0x41004 && config[68]==0 && config[69]==(F::Native+64)/16 && config[70]==0x9ee1 && config[71]==65535);
  Need(config[100]==0x1ffff && !config[101] && !config[180] && thread[37]==4 && packX==0 && packEnd==15);
  if(!packOpen){Need(packY==0 && !packPending);packOpen=true;packBytes=0;missingPack=false;}
  Need(packY<64 && !(packY&3) && packBytes==packY*16 && !packLast);
  const unsigned row0=(config[16]+packY*(config[12]>>16))/16;
  Need(row0==64+packY && row0+4<=128);
  for(unsigned row=row0;row<row0+4;row++)for(unsigned col=0;col<16;col++){
   Need(defined[row] && written[row][col] && dstReady[row]<=cycle && decoded[row-64][col]);
   const unsigned datum=physical[row][col];
   Need((datum&31)==16 && !(datum&0x8000));
   pendingBytes[packBytes++]=uint8_t((datum>>5)&127);
  }
  packs++;if(packs==skippedPack)missingPack=true;
  packY+=thread[37]&15;packLast=bool(op&1) && !skipLast;
  if(packLast)Need(packBytes==1024);
  packPending=true;packDue=cycle+packDelay;
 }
 void NativeA(){
  Need(!packPending && !task.pending && !mathPending && thread[0]==0 && thread[5]==4);
  Need(config[64]==0x01000016 && config[65]==0x40001 && config[66]==1 && config[67]==0);
  Need(config[70]==8 && config[71]==0 && config[72]==0x26 && config[73]==0xf000f && config[86]==0x1000100);
  Need(config[49]==0 && config[84]==0x400040 && xStart[0]==0 && xEnd[0]==1023);
  const unsigned address=(config[76]+1)*16;
  Need(address==(nativeInputs%2?F::Native:F::Ones));
  if(address==F::Native)Need(packClosures==groups+1);
  for(unsigned i=0;i<16;i++)Need(l1[address-16+i]==0);
  for(unsigned i=0;i<64;i++)Need(l1[address+i]==130);
  auto& a=source[0][unpackBank[0]];Need(!a.pending && !a.owner);
  for(unsigned i=0;i<1024;i++){
   unsigned row=i/16,col=i%16;
   if(forceTranspose){const unsigned low=row&15;row=(row&~15U)|col;col=low;}
   a.values[row][col]=Datum(address,i,Bfp8B,64);
   Need(std::isfinite(a.values[row][col]) && a.values[row][col]>=0 && a.values[row][col]<=15);
  }
  a.pending=true;a.due=cycle+delay[0];unpacks[0]++;nativeInputs++;
 }
 void NativeB(){
  Need(!packPending && !task.pending && thread[0]==0 && thread[5]==4);
  Need(config[112]==0x200015 && config[113]==0x10001 && config[114]==1 && config[115]==0);
  Need(config[120]==0x25 && config[121]==0xf000f && config[122]==0x17fff && config[123]==0 && config[140]==0 && config[61]==0);
  Need(xStart[1]==0 && xEnd[1]==31 && rawLoads>=4);
  const unsigned group=(rawLoads/4-1)%8,address=(config[124]+1)*16;
  Need(address==F::Input+OperandOffset()+group*128);
  for(unsigned h=0;h<64;h++)Need(!l1[address-64+h]);
  auto& b=source[1][unpackBank[1]];Need(!b.pending && !b.owner);
  for(unsigned k=0;k<32;k++)b.values[k/16][k%16]=Float(uint32_t(l1[address+k*2]|unsigned(l1[address+k*2+1])<<8)<<16);
  b.pending=true;b.due=cycle+delay[1];unpacks[1]++;
 }
 void NativeMatrix(unsigned op){
  Need(NativeMode() && !packPending && !Pending() && thread[2]==1 && thread[3]==1 && thread[5]==4);
  const unsigned increment=fourPhaseReference?1:2;
  Need(!config[2] && !config[3] && !thread[11] && !thread[13] && !thread[48] && ((thread[29]>>13)&3)==increment);
  Need((op&~0xc003ffU)==0x26084000);
  auto& a=source[0][matrixBank[0]];auto& b=source[1][matrixBank[1]];Need(a.owner && b.owner);
  const unsigned ar=rwcA&0x30,br=rwcB&63,dr=op&1023,phase=fidelity&3;
  Need(ar<=48 && br==(ar/16&1) && dr==(ar<32?32U:40U) && (fourPhaseReference || phase==0 || phase==2));
  visits[phase]++;
  if(phase!=dropPhase){valid[dr]=true;for(unsigned row=4;row<8;row++)valid[dr+row]=true;}
  if(!omitFlip)for(unsigned which=0;which<2;which++)if(op&(1U<<(22+which))){source[which][matrixBank[which]].owner=false;matrixBank[which]^=1;}
  fidelity=(fidelity+increment)&3;matrixOps++;mathPending=true;
 }
 void Op(unsigned op){
  const unsigned code=op>>24;
  if(code==0x41){cycle++;Pack(op);return;}
  if(op==0xa210181e){cycle++;Need(packPending);if(!skipPackWait){packWait=true;packWaits++;}return;}
  if(op==0x5e803c00){Need(!packPending);cycle++;packX=0;packEnd=15;return;}
  if(op==0x5180000b || op==0x5480000f){Need(!packPending);cycle++;packY=0;return;}
  if(op==0x7a003001 || op==0x7afff001){
   Need(!packPending && !Pending() && enabled);cycle++;vector.cycle=cycle;
   for(unsigned lane=0;lane<32;lane++){
    const unsigned input=vector.Read(0,lane);Need(input<=255);
    vector.lanes[0][lane]=op==0x7a003001?input<<3:input>>1;
   }vector.ready[0]=cycle+1;return;
  }
  if(code==0x72 && (op&0x000f0000)==0x00050000){
   Need(!packPending && !Pending() && enabled && !(op&0x00f0fc00));cycle++;vector.cycle=cycle;
   const unsigned address=op&1023;Need(address>=64 && address<128 && !(address&1));
   codeStores++;
   for(unsigned lane=0;lane<32;lane++){
    const unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+bool(address&2),value=vector.Read(0,lane);
    Need(!decoded[row-64][col] && !(value&~0x78U));
    if(codeStores!=skippedStore){physical[row][col]=uint16_t((value<<5)|16);written[row][col]=defined[row]=decoded[row-64][col]=true;dstReady[row]=cycle+1;}
   }return;
  }
  if(NativeMode() && op==0x420000c1){cycle++;NativeA();return;}
  if(NativeMode() && op==0x428000c1){cycle++;NativeB();return;}
  if(NativeMode() && code==0x26){
   cycle++;Need(!Pending() && !packPending);
   PioneerBlackholeArithmeticObserver::Matrix(*this,op,true,[&]{NativeMatrix(op);});
   const unsigned dr=op&1023;
   if(valid[dr]){for(unsigned c=0;c<16;c++)StoreFloat(dr,c,Bits(destination[dr][c]),cycle);defined[Adj(dr)]=defined[Adj(dr)+8]=true;}
   return;
  }
  Need(!packPending);Q4FpuPipelineMachine::Op(op);
 }
 unsigned Read(unsigned address){
  Need(address==0xffbd8800 && Clean() && nativeInputs%2==1 && valid[32]);sumReads++;
  return FloatDst(32,0);
 }
 void Write(unsigned address,unsigned value){
  const bool header=(address>=F::Native-16 && address<F::Native)||(address>=F::Ones-16 && address<F::Ones);
  const bool exponent=(address>=F::Native && address<F::Native+64)||(address>=F::Ones && address<F::Ones+64);
  const bool ones=address>=F::Ones+64 && address<F::Ones+F::TileBytes;
  Need(!(address&3) && ((header && !value)||(exponent && value==0x82828282)||(ones && value==0x08080808)));
  for(unsigned i=0;i<4;i++)l1[address+i]=uint8_t(value>>(i*8));
 }
 bool Clean()const{return !packPending && !packOpen && Q4FpuPipelineMachine::Clean();}
 std::array<unsigned,32> QDots(){
  Need(Clean() && nativeInputs%2==0 && packClosures==groups+1 && sumReads==groups+1);
  std::array<unsigned,32> out;
  for(unsigned i=0;i<32;i++)out[i]=FloatDst(i<16?32:40,i%16);
  groups++;return out;
 }
};
struct FactoredTileIssue {
 FactoredTileMachine& m;
 void Op(unsigned w){m.Op(w);}void Config(unsigned i,unsigned v){m.Config(i,v);}void Drain(){m.Drain();}
};
#endif
