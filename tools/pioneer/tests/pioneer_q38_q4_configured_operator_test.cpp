/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#include "../PioneerQ38Q4FactoredConfiguredCompute.h"
#define PIONEER_Q38_FACTORED_OPERATOR_FIXTURE_ONLY
#include "pioneer_q38_q4_factored_operator_test.cpp"
using Configured=PioneerQ38Q4FactoredConfiguredCompute;
struct RecordedIssue {
 FactoredOperatorMachine& m;unsigned pairs=0;std::vector<uint64_t> operations;
 void Op(unsigned w){operations.push_back(w);m.Op(w);}
 void Drain(){operations.push_back(UINT64_C(1)<<32);m.Drain();}
 void Config(unsigned i,unsigned v){pairs++;m.Config(i,v);}
};
int main(){
 assert(Round(0x3f808000)==0x3f800000);
 unsigned cases=0,words=0,saved=0;
 for(unsigned width:{5120U,17408U}){
  auto old=std::make_unique<FactoredOperatorMachine>(),candidate=std::make_unique<FactoredOperatorMachine>();
  for(auto* m:{old.get(),candidate.get()}){
   m->delay[0]=100003;m->delay[1]=7;m->packDelay=100019;
   for(auto& v:m->config)v=0xa7a7a7a7;
  }
  for(unsigned pass=0;pass<2;pass++){
   RecordedIssue a{*old,0,{}},b{*candidate,0,{}};
   Compute::Constants(*old);Configured::Constants(*candidate);Compute::Begin(a);Configured::Begin(b);
   assert(b.pairs==a.pairs+22 && a.operations==b.operations);a.pairs=b.pairs=0;
   std::array<unsigned,32> expected{};
   for(unsigned block=0;block<width/256;block++){
    TileFixture f(block+pass*101);StageOperator(f,*old,block&1);StageOperator(f,*candidate,block&1);
    assert(Compute::LoadMetadata(a,block&1) && Configured::LoadMetadata(b,block&1));
    for(unsigned group=0;group<8;group++){
     assert(Compute::Group(a,block&1,group) && Configured::Group(b,block&1,group));ReferenceGroup(f,group,expected);
     assert(a.operations==b.operations);
     for(unsigned row=0;row<32;row++){
      const unsigned r=row<16?80:88,c=row%16;
      assert(old->FloatDst(r,c)==expected[row] && candidate->FloatDst(r,c)==expected[row]);words++;
     }
    }
   }
   assert(old->Results()==expected && candidate->Results()==expected);
   assert(a.pairs==664*(width/256) && b.pairs==336*(width/256));
   saved+=a.pairs-b.pairs;cases++;
  }
 }
 struct NoIO {unsigned n=0;void Op(unsigned){n++;}void Config(unsigned,unsigned){n++;}void Drain(){n++;}} none;
 assert(!Configured::Group(none,2,0) && !Configured::Group(none,0,8) && !Configured::LoadMetadata(none,2) && !none.n);
 // Dropping fixed initialization or the changed tile pointer must not pass.
 struct DropIssue {
  FactoredOperatorMachine& m;unsigned fault;bool begin=true;
  void Op(unsigned w){m.Op(w);}void Drain(){m.Drain();}
  void Config(unsigned i,unsigned v){
   if((begin && ((fault==0 && i==28)||(fault==1 && i==112 && v==0x200015)))
    || (!begin && fault==2 && i==76 && v==Tile::Native/16-1))return;
   m.Config(i,v);
  }
 };
 for(unsigned fault=0;fault<3;fault++){
  auto m=std::make_unique<FactoredOperatorMachine>();for(auto& v:m->config)v=0xa7a7a7a7;
  TileFixture f(3);DropIssue e{*m,fault};bool refused=false;
  try{Configured::Constants(*m);Configured::Begin(e);e.begin=false;StageOperator(f,*m,0);
   assert(Configured::LoadMetadata(e,0));assert(Configured::Group(e,0,0));
  }catch(const Q4FpuPipelineMachine::Fault&){refused=true;}catch(unsigned v){assert(v==NativeBfpMachine::Unknown);refused=true;}
  assert(refused);
 }
 assert(cases==4 && words==45056 && saved==57728);
 printf("PASS_CONFIGURED_OPERATOR cases%u exactGroupWords%u identical_ALL_TensixWords_Drains_eachGroup twoJobsEach5120_17408_poisonedConfig;blockPairs664to336_plus22jobSetup_saved%u_missingInitPointerRefused3_invalidZeroIO;INSTRUCTION_MODEL_COUNTS_NOT_CARD_SPEED_OR_QUALITY\n",cases,words,saved);
}
