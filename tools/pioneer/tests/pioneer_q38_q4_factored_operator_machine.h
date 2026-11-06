/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FACTORED_OPERATOR_MACHINE_H
#define PIONEER_Q38_Q4_FACTORED_OPERATOR_MACHINE_H
#define PIONEER_Q38_COMPACT_FPU_PIPELINE_MODEL
#include "pioneer_q38_q4_factored_tile_machine.h"
#include "../PioneerQ38Q4FactoredCompute.h"
// Extend the existing shared physical-Dst observer,not a new whole-card
// emulator. New cases:original metadata import,scoped ZEROACC and explicit
// lane flags. The existing strict FMA arithmetic/latency is retained.
struct FactoredOperatorMachine:FactoredTileMachine {
 using C=PioneerQ38Q4FactoredCompute;
 unsigned compactImports=0,metadataStores=0,accumulatorStores=0,initialZeroLoads=0;
 unsigned scopeClears=0,maskSelections=0,groupStoreBits=0,groupSumLoads=0,buffer=0;
 bool useFlags=false,dropScopedClear=false,dropMask=false,omitFinalStore=false;
 unsigned missingMetadataImport=0,missingMetadataStore=0,missingZeroLoad=0;
 unsigned OperandOffset()const override{return buffer*0x10000;}
 void Op(unsigned op){
  const unsigned code=op>>24,mode=(op>>16)&15,address=op&1023;
  const bool metadata=config[1]==0xa03c8000;
  if(op==0x100c0002){
   Need(!Pending() && !packPending && !mathPending && !config[3] && !thread[48]);cycle++;scopeClears++;
   if(dropScopedClear)return;
   // Independently decode mode1/32b/face2. All Dst outside32..47 survives.
   for(unsigned row=32;row<48;row++){
    valid[row]=false;
    for(unsigned p:{Adj(row),Adj(row)+8}){defined[p]=false;memset(written[p],0,sizeof(written[p]));}
   }return;
  }
  if(op==0x42000091){
   Need(!Pending() && !packPending && !mathPending && !thread[0] && !thread[5]);
   const unsigned row=(config[84]&65535)/16-4;
   Need(config[1]==0x20200000 && config[64]==0x100010 && config[65]==16 && !config[72]
    && config[73]==0x13 && config[86]==0x100010 && xStart[0]==0 && xEnd[0]==255
    && (config[76]+1)*16==C::Zero && (row==80 || (row>=128 && row<=240 && !(row&15))));
   for(unsigned h=0;h<64;h++)Need(!l1[C::Zero-64+h]);
   cycle++;initialZeroLoads++;unpacks[0]++;
   if(initialZeroLoads!=missingZeroLoad)task={true,true,false,true,C::Zero,row,256,cycle+delay[0]};
   return;
  }
  if(op==0x42000081 && metadata){
   Need(!Pending() && !packPending && !mathPending && !thread[0] && !thread[5]);
   const unsigned a=(config[76]+1)*16;
   Need(config[64]==0x10001e && config[65]==16 && config[72]==14 && config[73]==0x13
    && config[84]==0x400040 && config[86]==0x100010 && xStart[0]==0 && xEnd[0]==255
    && a==C::Metadata+OperandOffset()+(compactImports%2)*320 && OperandReady(a,256));
   for(unsigned h=0;h<64;h++)Need(!l1[a-64+h]);
   cycle++;compactImports++;unpacks[0]++;
   if(compactImports!=missingMetadataImport)task={true,false,false,false,a,0,256,cycle+delay[0]};
   return;
  }
  if(op==0x8a001002){
   Need(enabled && !Pending() && !packPending && !metadata);cycle++;useFlags=true;vector.activeLanes=UINT32_MAX;return;
  }
  if(op==0x7c000f70){
   Need(useFlags && vector.activeLanes==UINT32_MAX);cycle++;
   for(unsigned lane=0;lane<32;lane++)vector.lanes[7][lane]=2*lane;
   vector.ready[7]=cycle+1;return;
  }
  if(op==0x7b000700){
   Need(useFlags && vector.activeLanes==UINT32_MAX);cycle++;vector.cycle=cycle;
   uint32_t mask=0;for(unsigned lane=0;lane<32;lane++)if(int32_t(vector.Read(7,lane))<0)mask|=1U<<lane;
   Need(mask==255);if(!dropMask)vector.activeLanes=mask;maskSelections++;return;
  }
  if(op==0x8a000002){
   if(useFlags){
    Need(groupStoreBits==15 && groupSumLoads==1 && nativeInputs%2==0 && Clean());
    groups++;groupStoreBits=groupSumLoads=0;useFlags=false;
   }
   vector.activeLanes=UINT32_MAX;FactoredTileMachine::Op(op);return;
  }
  if((code==0x70 || code==0x72) && (metadata || mode==3)){
   Need(!Pending() && !packPending && enabled && !(op&0xfc00) && !(address&1));cycle++;vector.cycle=cycle;
   const unsigned reg=(op>>20)&15;Need(reg<8);
   if(metadata){
    Need(vector.activeLanes==UINT32_MAX && !useFlags);
    if(code==0x72)metadataStores++;
   }else{
    Need(useFlags && vector.activeLanes==255 && mode==3 && compactImports>=2);
    if(code==0x70 && reg==1){Need(op==0x70130020 && nativeInputs%2==1 && !groupSumLoads);groupSumLoads++;sumReads++;}
    else Need(nativeInputs%2==0 && groupSumLoads==1);
    if(code==0x72){
     Need(reg==4 && (address&~2U)==(address<88?80U:88U));
     const unsigned bit=1U<<((address>=88?2:0)+bool(address&2));Need(!(groupStoreBits&bit));
     groupStoreBits|=bit;accumulatorStores++;
    }
   }
   for(unsigned lane=0;lane<32;lane++)if(vector.LaneEnabled(lane)){
    const unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+bool(address&2);
    if(code==0x70){
     if(mode==13){Need(metadata && row<16 && defined[row] && written[row][col] && dstReady[row]<=cycle);vector.lanes[reg][lane]=BlackholeRawSfpLoad(physical[row][col],13);}
     else{
      Need(mode==3 && (metadata?(row>=128 && row<256):(row==32 || row==40 || row==80 || row==88 || (row>=128 && row<256))));
      vector.lanes[reg][lane]=FloatDst(row,col);
     }
    }else{
     Need(mode==3 && (metadata?(row>=128 && row<256):(row==80 || row==88)));
     Need(defined[Adj(row)] && defined[Adj(row)+8]);
     if((metadata && metadataStores==missingMetadataStore) || (!metadata && omitFinalStore && groupStoreBits==15))continue;
     unsigned bits=vector.Read(reg,lane);if(!(bits&0x7f800000))bits&=0x80000000;
     StoreFloat(row,col,bits,cycle+1);
    }
   }
   if(code==0x70)vector.ready[reg]=cycle+1;return;
  }
  if(code==0x85 || code==0x86){
   Need(!Pending() && !packPending && enabled);cycle++;vector.cycle=cycle;vector.Op(op);Need(vector.okay);return;
  }
  if(NativeMode() && op==0x428000c1)Need(OperandReady((config[124]+1)*16,64));
  FactoredTileMachine::Op(op);
 }
 void Write(unsigned a,unsigned v){
  if(a>=C::Zero-64 && a<C::Zero+1024){Need(!(a&3) && !v);memset(l1.data()+a,0,4);return;}
  FactoredTileMachine::Write(a,v);
 }
 std::array<unsigned,32> Results(){
  Need(Clean() && !useFlags && !groupStoreBits && !groupSumLoads && !(groups%8));
  std::array<unsigned,32> out;for(unsigned r=0;r<32;r++)out[r]=FloatDst(r<16?80:88,r%16);return out;
 }
};
struct FactoredOperatorIssue {
 FactoredOperatorMachine& m;
 void Op(unsigned w){m.Op(w);}void Config(unsigned i,unsigned v){m.Config(i,v);}void Drain(){m.Drain();}
};
#endif
