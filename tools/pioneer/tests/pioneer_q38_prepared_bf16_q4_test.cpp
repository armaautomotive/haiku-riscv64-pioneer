/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_prepared_bf16_q4_machine.h"
#include "../PioneerQ38PreparedBf16Q4Reference.h"
#include "../PioneerQ38Q4FactoredConfiguredCompute.h"
#include "../PioneerQ38Q4FpuCompactStreamCompute.h"
#include "../PioneerQ38Q4EscapeTiles.h"
#define PIONEER_Q38_FACTORED_OPERATOR_FIXTURE_ONLY
#include "pioneer_q38_q4_factored_operator_test.cpp"
using Prepared=PioneerQ38PreparedBf16Q4Compute;
struct CountIssue {
 unsigned ops=0,configs=0,drains=0,masks=0,writes=0,seals=0,packs=0,matrices=0;
 void Op(unsigned v){ops++;packs+=(v>>24)==0x41;matrices+=(v>>24)==0x26;}
 void Config(unsigned,unsigned){configs++;}void Drain(){drains++;}
 unsigned MaskWord(unsigned,unsigned,unsigned,unsigned){masks++;return 0;}
 void EscapeWord(unsigned,unsigned,unsigned){writes++;}void SealData(){seals++;}
 template<class Compute> static void Report(const char* name){
  CountIssue e;Compute::Begin(e);e={};assert(Compute::LoadMetadata(e,0));
  for(unsigned group=0;group<8;group++)assert(Compute::Group(e,0,group));
  printf("PREPARED_BF16_STATIC_K256 recipe%s ops%u configPairs%u drains%u packs%u matrices%u maskWords%u escapeWrites%u fences%u;NOT_CYCLES_OR_TPS\n",
   name,e.ops,e.configs,e.drains,e.packs,e.matrices,e.masks,e.writes,e.seals);
 }
};
static void StagePrepared(const TileFixture& f,PreparedBf16Q4Machine& m,unsigned buffer){
 StageOperator(f,m,buffer);
 for(unsigned half=0;half<2;half++){
  std::array<uint8_t,2304> tiled{};std::array<uint8_t,2816> encoded{};
  memcpy(tiled.data(),f.compact.data()+4096+half*256,256);
  memcpy(tiled.data()+256,f.compact.data()+half*2048,2048);
  assert(PioneerQ38Q4EscapeTiles::Encode(tiled.data(),tiled.size(),encoded.data(),encoded.size()));
  for(unsigned p=0;p<16;p++){
   const unsigned address=Prepared::Face(buffer,p/2,half,p%2);
   memset(m.l1.data()+address-64,0,64);memcpy(m.l1.data()+address,encoded.data()+256+p*128,128);
  }
  memcpy(m.l1.data()+0x8c000+buffer*0x10000+half*512,encoded.data()+2304,512);
 }
}
int main(){
 assert(Round(0x3f808000)==0x3f800000);static_cast<void>(&ReferenceGroup);
 unsigned jobs=0,words=0,controls=0;
 auto m=std::make_unique<PreparedBf16Q4Machine>();
 // Two fresh jobs at BOTH real widths; then arbitrary-BF16 exponent/low-bit
 // cases, signed zeros and q15-heavy correction data. Not model quality.
 for(unsigned trial=0;trial<8;trial++){
  const unsigned width=trial<4?(trial%2?17408:5120):256;
  PreparedBf16Q4Issue e{*m};
  const auto oldPanels=m->panels,oldMatrices=m->matrixOps,oldReads=m->maskReads,oldWrites=m->escapeWrites;
  m->delay[0]=trial%2?7:100003;m->delay[1]=trial%2?100019:3;
  Prepared::Constants(*m);Prepared::Begin(e);std::array<unsigned,32> expected{};
  for(unsigned block=0;block<width/256;block++){
   TileFixture f(trial*101+block);
   if(trial>=4){
    for(unsigned k=0;k<256;k++)f.input[k]=trial==4?(k&1?0x8000:0):uint16_t(((k%5==2)?0x8000:0)|((96+k%57)<<7)|((k*13+trial)&127));
    if(trial==7){for(unsigned row=0;row<32;row++)memset(f.original.data()+row*144+16,255,128);
     assert(PioneerQ38Q4FpuCompactData::Prepare(f.original.data(),4608,f.compact.data(),4608));}
   }
   StagePrepared(f,*m,block&1);assert(Prepared::LoadMetadata(e,block&1));
   for(unsigned group=0;group<8;group++){
    assert(PioneerQ38PreparedBf16Q4Reference::Group(f.original.data(),f.input.data(),group,expected.data()));
    assert(Prepared::Group(e,block&1,group));
    for(unsigned row=0;row<32;row++){
     const auto got=m->FloatDst(row<16?80:88,row%16);
     if(got!=expected[row]){fprintf(stderr,"PREPARED_BF16_MISMATCH trial%u block%u group%u row%u got%08x expected%08x\n",trial,block,group,row,got,expected[row]);assert(false);}words++;
    }
   }
  }
  assert(m->Results()==expected && m->panels-oldPanels==width/256*80 && m->matrixOps-oldMatrices==width/256*160
   && m->maskReads-oldReads==width/256*256 && m->escapeWrites-oldWrites==width/256*512 && !m->packs && !m->codeStores);
  jobs++;
 }
 for(unsigned fault=0;fault<5;fault++){
  TileFixture f(3);auto m=std::make_unique<PreparedBf16Q4Machine>();m->reportFaults=false;PreparedBf16Q4Issue e{*m};
  Prepared::Constants(*m);Prepared::Begin(e);StagePrepared(f,*m,0);assert(Prepared::LoadMetadata(e,0));
  if(fault==0)m->dropEscape=true;if(fault==1)m->dropSeal=true;if(fault==2)m->dropPhase=2;
  if(fault==3)m->omitFlip=true;if(fault==4)m->dropProducer=1;
  std::array<unsigned,32> expected{};assert(PioneerQ38PreparedBf16Q4Reference::Group(f.original.data(),f.input.data(),0,expected.data()));
  bool refused=false;try{
   assert(Prepared::Group(e,0,0));for(unsigned r=0;r<32;r++)refused|=m->FloatDst(r<16?80:88,r%16)!=expected[r];
  }catch(const Q4FpuPipelineMachine::Fault&){refused=true;}catch(unsigned v){assert(v==NativeBfpMachine::Unknown);refused=true;}
  assert(refused);controls++;
 }
 struct NoIO {
  unsigned calls=0;void Op(unsigned){calls++;}void Config(unsigned,unsigned){calls++;}void Drain(){calls++;}
  unsigned MaskWord(unsigned,unsigned,unsigned,unsigned){calls++;return 0;}void EscapeWord(unsigned,unsigned,unsigned){calls++;}void SealData(){calls++;}
 } noIO;
 assert(!Prepared::Group(noIO,2,0) && !Prepared::Group(noIO,0,8) && !noIO.calls);
 assert(!Prepared::LoadMetadata(noIO,2) && !noIO.calls);
 CountIssue::Report<PioneerQ38Q4FpuCompactStreamCompute>("oldBF16");
 CountIssue::Report<PioneerQ38Q4FactoredConfiguredCompute>("rejected114");
 CountIssue::Report<Prepared>("preparedBF16");
 printf("PASS_PREPARED_BF16_Q4 retainedContextJobs%u exactGroupWords%u mutationControls%u;existingSM3_escapeFormat_originalMetadata_fullBF16_phases0_2_noPACK_noWeightFloatDecode;OFFBOARD_NOT_NATIVE_MODEL_QUALITY_OR_SPEED\n",jobs,words,controls);
}
