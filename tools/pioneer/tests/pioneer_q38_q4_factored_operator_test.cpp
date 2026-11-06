/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_q4_factored_operator_machine.h"
#define PIONEER_Q38_FACTORED_TILE_FIXTURE_ONLY
#include "pioneer_q38_q4_factored_tile_test.cpp"
using Compute=PioneerQ38Q4FactoredCompute;
[[maybe_unused]] static void StageOperator(const TileFixture& f,FactoredOperatorMachine& m,unsigned buffer){
 m.buffer=buffer;
 // Lossless ORIGINAL bytes only,never expected products/affine coefficients.
 for(unsigned pair=0;pair<4;pair++)for(unsigned face=0;face<4;face++){
  const unsigned a=Tile::Raw+buffer*0x10000+(pair*4+face)*320,from=(face/2)*2048+pair*512+(face%2)*256;
  memset(m.l1.data()+a-64,0,64);memcpy(m.l1.data()+a,f.compact.data()+from,256);
 }
 for(unsigned half=0;half<2;half++){
  const unsigned a=Compute::Metadata+buffer*0x10000+half*320;
  memset(m.l1.data()+a-64,0,64);memcpy(m.l1.data()+a,f.compact.data()+4096+half*256,256);
 }
 for(unsigned group=0;group<8;group++){
  const unsigned a=Tile::Input+buffer*0x10000+group*128;memset(m.l1.data()+a-64,0,64);
  for(unsigned k=0;k<32;k++){m.l1[a+k*2]=uint8_t(f.input[group*32+k]);m.l1[a+k*2+1]=uint8_t(f.input[group*32+k]>>8);}
 }
}
static void ReferenceGroup(const TileFixture& f,unsigned group,std::array<unsigned,32>& expected){
 for(unsigned row=0;row<32;row++){
  const auto* p=f.original.data()+row*144;unsigned scale,minimum;
  if(group<4){scale=p[4+group]&63;minimum=p[8+group]&63;}
  else{scale=(p[8+group]&15)|((p[group]>>6)<<4);minimum=(p[8+group]>>4)|((p[4+group]>>6)<<4);}
  volatile float coefficient=PioneerHalfFloat(uint16_t(p[0])|uint16_t(p[1])<<8)*float(scale);
  volatile float offset=PioneerHalfFloat(uint16_t(p[2])|uint16_t(p[3])<<8)*float(minimum);
  const float negative=NativeBfpMachine::Float(NativeBfpMachine::Bits(offset)^0x80000000);
  volatile float a=coefficient*NativeBfpMachine::Float(f.Dot(row,group));
  volatile float b=negative*NativeBfpMachine::Float(f.Sum(group));
  volatile float contribution=a+b,result=NativeBfpMachine::Float(expected[row])+contribution;
  expected[row]=NativeBfpMachine::Bits(result);
 }
}
#ifndef PIONEER_Q38_FACTORED_OPERATOR_FIXTURE_ONLY
int main(){
 assert(Round(0x3f808000)==0x3f800000);unsigned jobs=0,groupWords=0,controls=0;
 for(unsigned width:{5120U,17408U}){
  auto m=std::make_unique<FactoredOperatorMachine>();m->delay[0]=100003;m->delay[1]=7;m->packDelay=100019;
  for(unsigned pass=0;pass<2;pass++){
   FactoredOperatorIssue e{*m};Compute::Constants(*m);Compute::Begin(e);std::array<unsigned,32> expected{};
   for(unsigned block=0;block<width/256;block++){
    TileFixture f(block+pass*101);StageOperator(f,*m,block&1);assert(Compute::LoadMetadata(e,block&1));
    for(unsigned group=0;group<8;group++){
     assert(Compute::Group(e,block&1,group));ReferenceGroup(f,group,expected);
     for(unsigned row=0;row<32;row++){
      const auto got=m->FloatDst(row<16?80:88,row%16);
      if(got!=expected[row]){fprintf(stderr,"FACTORED_OPERATOR_MISMATCH K%u pass%u block%u group%u row%u got%08x expected%08x\n",width,pass,block,group,row,got,expected[row]);assert(false);}
      groupWords++;
     }
    }
   }
   assert(m->Results()==expected && m->initialZeroLoads==9*(pass+1));jobs++;
  }
 }
 // Real dataflow controls:erase clear/mask/init/import or a result store.
 for(unsigned fault=0;fault<6;fault++){
  auto m=std::make_unique<FactoredOperatorMachine>();FactoredOperatorIssue e{*m};TileFixture f(3);
  Compute::Constants(*m);
  if(fault==0)m->dropScopedClear=true;if(fault==1)m->dropMask=true;if(fault==2)m->missingZeroLoad=1;
  if(fault==3)m->missingMetadataImport=1;if(fault==4)m->missingMetadataStore=1;if(fault==5)m->omitFinalStore=true;
  bool refused=false;std::array<unsigned,32> expected{};
  try{
   Compute::Begin(e);StageOperator(f,*m,0);assert(Compute::LoadMetadata(e,0));
   for(unsigned g=0;g<8;g++){assert(Compute::Group(e,0,g));ReferenceGroup(f,g,expected);}
   refused=m->Results()!=expected;
  }catch(const Q4FpuPipelineMachine::Fault&){refused=true;}catch(unsigned v){assert(v==NativeBfpMachine::Unknown);refused=true;}
  assert(refused);controls++;
 }
 struct NoIO{unsigned n=0;void Op(unsigned){n++;}void Config(unsigned,unsigned){n++;}void Drain(){n++;}} none;
 assert(!Compute::Group(none,2,0) && !Compute::Group(none,0,8) && !Compute::LoadMetadata(none,2) && !none.n);
 assert(jobs==4 && groupWords==45056 && controls==6);
 printf("PASS_FACTORED_OPERATOR fullK5120_17408 twoJobsEach ORIGINAL_Q4_METADATA_INPUT_to_exact_NEW_FP32_outputAffine_accumulation groupWords%u controls%u;genuineSFPU_sum_noScalarPayload_orHostProducts maskedFirstRow_noUndefinedPadding;LOCAL_INSTRUCTION_MODEL_NOT_NATIVE_HARDWARE_QUALITY_OR_TPS\n",groupWords,controls);
}
#endif
