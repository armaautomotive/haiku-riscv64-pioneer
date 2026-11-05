/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_COMPACT_FPU_PIPELINE_MODEL
#include "pioneer_q38_q4_fpu_pipeline_machine.h"
#include "../PioneerQ38Q4FpuCompactBlock.h"
#include "../PioneerQ38Q4FpuCompactData.h"
#define PIONEER_Q4_PIPELINE_FIXTURE_ONLY
#include "pioneer_q38_q4_fpu_pipeline_test.cpp"

struct CompactFpuPipelineMachine:Q4FpuPipelineMachine {
 unsigned compactImports=0,compactStores=0,extraZeroLoads=0;
 unsigned dropCompactStore=0,dropCompactImport=0;bool skipExtraZero=false;
 bool metadata=false;
 void Config(unsigned i,unsigned v){Q4FpuPipelineMachine::Config(i,v);if(i==1)metadata=v==0xa03c8000;}
 void Op(unsigned op){
  const unsigned code=op>>24;
  if(op==0x42000081 && metadata){
   assert(!Pending() && !source[0][unpackBank[0]].owner && !thread[0] && !thread[5]);
   const unsigned address=(config[76]+1)*16;
   assert(config[49]==0 && config[64]==0x10001e && config[65]==16 && config[72]==14 && config[73]==0x13
    && config[84]==0x00400040 && config[86]==0x100010 && xStart[0]==0 && xEnd[0]==255
    && address==0x94000+OperandOffset()+(compactImports%2)*320);
   if(!OperandReady(address,256))throw Fault{};
   for(unsigned h=0;h<64;h++)assert(!l1[address-64+h]);
   cycle++;compactImports++;unpacks[0]++;
   if(compactImports!=dropCompactImport)task={true,false,false,false,address,0,256,cycle+delay[0]};return;
  }
  if(op==0x42000091 && (config[84]&65535)/16-4>128){
   assert(!Pending() && !mathPending && !source[0][unpackBank[0]].owner && !thread[0] && !thread[5]);
   const unsigned row=(config[84]&65535)/16-4;
   assert(config[1]==0x20200000 && config[64]==0x100010 && config[65]==16 && !config[72]
    && config[73]==0x13 && config[86]==0x100010 && xStart[0]==0 && xEnd[0]==255
    && (config[76]+1)*16==0x97000 && row>=144 && row<=240 && !(row&15));
   cycle++;extraZeroLoads++;unpacks[0]++;if(!skipExtraZero)task={true,true,false,true,0x97000,row,256,cycle+delay[0]};return;
  }
  const unsigned address=op&1023,mode=(op>>16)&15;
  if((code==0x70 || code==0x72) && (metadata || (mode==3 && address>=128))){
   if(Pending() || !enabled)throw Fault{};
   cycle++;vector.cycle=cycle;const unsigned reg=(op>>20)&15;
   assert(reg<8 && !(address&1) && !(op&0xfc00));
   if(code==0x72)compactStores++;
   for(unsigned lane=0;lane<32;lane++){
    const unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+bool(address&2);
    if(code==0x70){
     if(mode==13){assert(metadata && row<16);if(!defined[row] || !written[row][col] || dstReady[row]>cycle)throw Fault{};vector.lanes[reg][lane]=BlackholeRawSfpLoad(physical[row][col],13);}
     else{assert(mode==3 && row>=128 && row<256);vector.lanes[reg][lane]=FloatDst(row,col);}
    }else{
     assert(metadata && mode==3 && row>=128 && row<256);
     if(!defined[Adj(row)] || !defined[Adj(row)+8])throw Fault{};
     const unsigned bits=vector.Read(reg,lane);if(compactStores!=dropCompactStore)StoreFloat(row,col,bits,cycle+1);
    }
   }
   if(code==0x70)vector.ready[reg]=cycle+1;return;
  }
  if(code==0x86){
   assert(metadata);if(Pending() || !enabled)throw Fault{};
   cycle++;vector.cycle=cycle;vector.Op(op);if(!vector.okay)throw Fault{};return;
  }
  Q4FpuPipelineMachine::Op(op);
 }
 void StageCompact(const std::array<uint8_t,4608>& bytes,const std::array<uint16_t,256>& input){
  assert(Clean());
  for(unsigned pair=0;pair<4;pair++)for(unsigned face=0;face<4;face++){
   const unsigned at=0x90000+(pair*4+face)*320,from=(face/2)*2048+pair*512+(face%2)*256;
   memset(l1.data()+at-64,0,64);memcpy(l1.data()+at,bytes.data()+from,256);
  }
  for(unsigned half=0;half<2;half++){
   const unsigned at=0x94000+half*320;memset(l1.data()+at-64,0,64);memcpy(l1.data()+at,bytes.data()+4096+half*256,256);
  }
  for(unsigned group=0;group<8;group++){
   const unsigned at=0x96000+group*128;memset(l1.data()+at-64,0,64);
   for(unsigned c=0;c<32;c++){l1[at+c*2]=uint8_t(input[group*32+c]);l1[at+c*2+1]=uint8_t(input[group*32+c]>>8);}
  }
  memset(l1.data()+0x97000-64,0,1088);
 }
};
struct CompactIssue {CompactFpuPipelineMachine& m;void Op(unsigned op){m.Op(op);}void Config(unsigned i,unsigned v){m.Config(i,v);}void Drain(){m.Drain();}};

#ifndef PIONEER_Q38_COMPACT_FPU_PIPELINE_MACHINE_ONLY
int main(){
 unsigned jobs=0;
 for(unsigned trial=0;trial<8;trial++){
  Q4PipelineFixture data(trial);std::array<uint8_t,4608> bytes;
  assert(PioneerQ38Q4FpuCompactData::Prepare(data.original.data(),data.original.size(),bytes.data(),bytes.size()));
  CompactFpuPipelineMachine m;m.delay[0]=100003;m.delay[1]=7;m.StageCompact(bytes,data.input);CompactIssue e{m};
  for(unsigned pass=0;pass<2;pass++){
   PioneerQ38Q4FpuCompactBlock::Run(e);assert(m.Clean() && m.Results()==data.expected);jobs++;
  }
  assert(m.compactImports==4 && m.compactStores==144 && m.extraZeroLoads==14 && m.metadataLoads==0
   && m.rawLoads==64 && m.inputLoads==16 && m.moves==256 && m.matrixOps==256);
  for(unsigned phase=0;phase<4;phase++)assert(m.visits[phase]==64);
 }
 printf("PASS_COMPACT_Q4_FPU_PIPELINE jobs%u ORIGINAL_RAW_METADATA_to_sameBF16_RNE_MOVD2A_fourPhaseMatrix_all32OutputsExact;noExpandedCoefficientInput_noScalarWeights_orHostProducts;EMITTED_MOCK_NOT_COMPILED_NATIVE_QUALITY_OR_SPEED\n",jobs);
}
#endif
