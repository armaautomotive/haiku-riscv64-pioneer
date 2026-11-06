/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_q4_factored_tile_machine.h"
#include "../PioneerQ38Q4FpuCompactData.h"
#include <cstdio>
#include <memory>
using Tile=PioneerQ38Q4FactoredTile;
struct TileFixture {
 std::array<uint8_t,4608> original,compact;
 std::array<uint16_t,256> input;
 std::array<int,256> integer;
 TileFixture(unsigned trial){
  uint32_t seed=0x27b83201+trial;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
  for(unsigned row=0;row<32;row++){
   auto* p=original.data()+row*144;for(unsigned b=0;b<144;b++)p[b]=uint8_t(random());
   for(unsigned h:{1U,3U})if((p[h]&0x7c)==0x7c)p[h]^=4;
  }
  for(unsigned k=0;k<256;k++){
   integer[k]=trial<32?int(random()%511)-255:128+int(k%32);
   input[k]=uint16_t(NativeBfpMachine::Bits(float(integer[k])/128)>>16);
  }
  assert(PioneerQ38Q4FpuCompactData::Prepare(original.data(),original.size(),compact.data(),compact.size()));
 }
 unsigned Code(unsigned row,unsigned k)const{
  const auto* p=original.data()+row*144;
  return (p[16+(k/64)*32+k%32]>>((k/32%2)*4))&15;
 }
 unsigned Dot(unsigned row,unsigned group)const{
  int dot=0;for(unsigned k=0;k<32;k++)dot+=int(Code(row,group*32+k))*integer[group*32+k];
  return NativeBfpMachine::Bits(float(dot)/128);
 }
 unsigned Sum(unsigned group)const{
  int sum=0;for(unsigned k=0;k<32;k++)sum+=integer[group*32+k];
  return NativeBfpMachine::Bits(float(sum)/128);
 }
 void Stage(FactoredTileMachine& m)const{
  for(unsigned pair=0;pair<4;pair++)for(unsigned face=0;face<4;face++){
   const unsigned a=Tile::Raw+(pair*4+face)*320,from=(face/2)*2048+pair*512+(face%2)*256;
   memset(m.l1.data()+a-64,0,64);memcpy(m.l1.data()+a,compact.data()+from,256);
  }
  for(unsigned g=0;g<8;g++){
   const unsigned a=Tile::Input+g*128;memset(m.l1.data()+a-64,0,64);
   for(unsigned k=0;k<32;k++){m.l1[a+k*2]=uint8_t(input[g*32+k]);m.l1[a+k*2+1]=uint8_t(input[g*32+k]>>8);}
  }
 }
};
struct MutantIssue:FactoredTileIssue {
 bool flush=false,badPackFormat=false;
 explicit MutantIssue(FactoredTileMachine& m):FactoredTileIssue{m}{}
 void Op(unsigned w){FactoredTileIssue::Op(flush && w==0x41000001?0x41000003:w);}
 void Config(unsigned i,unsigned v){FactoredTileIssue::Config(i,badPackFormat && i==0 && v==0x7ab6?0x42b6:v);}
};
// A full four-phase control over the SAME actual packed tiles/source banks.
// Only the fixture enables this distinct reference schedule,not the producer.
struct FourPhaseIssue:FactoredTileIssue {
 explicit FourPhaseIssue(FactoredTileMachine& m):FactoredTileIssue{m}{m.fourPhaseReference=true;}
 void Op(unsigned w){
  if(w==0xb21d4000){m.Op(0xb21d2000);return;}
  if((w>>24)==0x26){m.Op(w&~0xc00000U);m.Op(w);return;}
  m.Op(w);
 }
};
#ifndef PIONEER_Q38_FACTORED_TILE_FIXTURE_ONLY
int main(){
 assert(Round(0x3f808000)==0x3f800000); // old RNE remains a distinct recipe
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned groups=0,words=0,failures=0,numerical=0,fidelityWords=0;
 for(unsigned trial=0;trial<32;trial++){
  TileFixture data(trial);auto m=std::make_unique<FactoredTileMachine>();data.Stage(*m);
  Tile::Constants(*m);m->packDelay=trial%2?100003:3;m->delay[0]=trial%3?7:100019;m->delay[1]=trial%2?11:100031;
  const auto before=m->l1;FactoredTileIssue e{*m};Tile::Begin(e);
  for(unsigned group=0;group<8;group++){
   unsigned sum=~0U;assert(Tile::Group(e,*m,group,sum));const auto dots=m->QDots();
   assert(sum==data.Sum(group));
   for(unsigned row=0;row<32;row++){assert(dots[row]==data.Dot(row,group));words++;}
   assert(m->Clean() && m->packClosures==group+1 && m->sumReads==group+1 && m->nativeInputs==(group+1)*2);
   for(unsigned i=0;i<1088;i++){
    if(i<64)assert(m->l1[Tile::Native+i]==130);
    else{
     const unsigned qindex=i-64,face=qindex/256,kr=qindex/16%16,out=qindex%16;
     const unsigned row=(face/2)*16+out,k=group*32+(face%2)*16+kr;
     assert(m->l1[Tile::Native+i]==data.Code(row,k)*8);
    }
   }
   for(unsigned a=0;a<m->l1.size();a++)if(a<Tile::Native+64 || a>=Tile::Native+1088)assert(m->l1[a]==before[a]);
   groups++;
  }
  assert(m->packs==128 && m->packWaits==8 && m->codeStores==256 && m->matrixOps==128 && m->visits[0]==64 && m->visits[2]==64 && !m->visits[1] && !m->visits[3]);
 }
 // Arbitrary finite normal/zero BF16 inputs across exponents,not just the
 // exactly representable integer-sum fixture. No CPU ideal-sum comparator.
 for(unsigned trial=0;trial<16;trial++){
  TileFixture data(32+trial);uint32_t seed=0xf8bf1600+trial;
  for(auto& value:data.input){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
   value=uint16_t((seed&0x8000)|((96+seed%57)<<7)|((seed>>17)&127));}
  if(!trial)for(unsigned k=0;k<256;k++)data.input[k]=k&1?0x8000:0;
  auto two=std::make_unique<FactoredTileMachine>(),four=std::make_unique<FactoredTileMachine>();
  data.Stage(*two);data.Stage(*four);Tile::Constants(*two);Tile::Constants(*four);
  FactoredTileIssue a{*two};FourPhaseIssue b(*four);Tile::Begin(a);Tile::Begin(b);
  for(unsigned group=0;group<8;group++){
   unsigned sa=0,sb=0;assert(Tile::Group(a,*two,group,sa) && Tile::Group(b,*four,group,sb));
   const auto qa=two->QDots(),qb=four->QDots();assert(sa==sb && qa==qb);fidelityWords+=32;
  }
  assert(two->matrixOps==128 && four->matrixOps==256 && four->visits[0]==64 && four->visits[1]==64 && four->visits[2]==64 && four->visits[3]==64);
 }
 // Unknown producer/pack/source cases may expose NO accepted group.
 for(unsigned fault=0;fault<23;fault++){
  TileFixture data(32);auto m=std::make_unique<FactoredTileMachine>();data.Stage(*m);Tile::Constants(*m);
  MutantIssue e(*m);Tile::Begin(e);
  if(fault<16)m->skippedPack=fault+1;
  else if(fault==16)m->skipPackWait=true;
  else if(fault==17)m->skipLast=true;
  else if(fault==18)m->skippedStore=17;
  else if(fault==19)m->omitFlip=true;
  else if(fault==20)m->dropProducer=1;
  else if(fault==21)e.flush=true;
  else e.badPackFormat=true;
  bool rejected=false;try{unsigned sum=~0U;(void)Tile::Group(e,*m,0,sum);(void)m->QDots();}
  catch(const FactoredTileMachine::Fault&){rejected=true;}catch(unsigned v){assert(v==NativeBfpMachine::Unknown);rejected=true;}
  assert(rejected && m->groups==0);failures++;
 }
 // Actual matrix input/layout or fidelity errors MUST alter the numbers.
 for(unsigned fault=0;fault<2;fault++){
  TileFixture data(32);auto m=std::make_unique<FactoredTileMachine>();data.Stage(*m);Tile::Constants(*m);
  if(!fault)m->forceTranspose=true;else m->dropPhase=2;
  FactoredTileIssue e{*m};Tile::Begin(e);unsigned sum=0;assert(Tile::Group(e,*m,0,sum));const auto dots=m->QDots();
  bool wrong=sum!=data.Sum(0);for(unsigned row=0;row<32;row++)wrong|=dots[row]!=data.Dot(row,0);
  assert(wrong);numerical++;
 }
 struct NoIO{unsigned calls=0;void Op(unsigned){calls++;}void Config(unsigned,unsigned){calls++;}void Drain(){calls++;}unsigned Read(unsigned){calls++;return 0;}} noIO;
 for(unsigned invalid:{8U,9U,~0U}){unsigned sum=0x12345678;assert(!Tile::Group(noIO,noIO,invalid,sum) && sum==0x12345678);assert(!Tile::PackCodes(noIO,invalid));}
 assert(!Tile::Matrix(noIO,Tile::Native+64,0) && noIO.calls==0);
 assert(groups==256 && words==8192 && failures==23 && numerical==2 && fidelityWords==4096);
 printf("PASS_FACTORED_TILE groups%u qdotWords%u genuineInstructionSumAndDot fromORIGINAL_BYTES_SFPU_INT8_PACK_LAST_WAIT_L1_NATIVE_BFP8_UNPACK_twoFidelityMatrix sharedPhysicalDst bothSourceBanks inputUntouched;two_vs_four_phase_exactWords%u unknownRefusals%u wrongTransposeOrMissingPhaseControls%u;HOST_ISA_OBSERVER_NOT_NATIVE_HARDWARE_FULL_MODEL_QUALITY_OR_TPS\n",groups,words,fidelityWords,failures,numerical);
}
#endif
