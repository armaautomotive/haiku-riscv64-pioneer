/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_Q4_METADATA_MODEL_ONLY
#include "pioneer_q38_vector_q4_metadata_test.cpp"
#include "pioneer_blackhole_sfp_load.h"
#include "../PioneerQ38Q4FpuCompactData.h"
#include "../PioneerQ38Q4FpuCompactMetadata.h"
#include "../PioneerQ38Q4FpuBlockData.h"
using Compact=PioneerQ38Q4FpuCompactMetadata;

// Explicit synthetic completed raw-prefix and initialized Dst endpoints.
// Shared physical Dst and unchanged strict MetadataMachine arithmetic.
// NO import/initialization/compiled worker/native execution is claimed.
struct CompactMetadataMachine {
 struct Hazard{};
 MetadataMachine vector;
 uint16_t physical[512][16]={};bool defined[512][16]={};unsigned ready[512][16]={};
 unsigned stores=0,transposes=0,drains=0,dropStore=0,dropTranspose=0;bool dirty=false;
 static unsigned Adj(unsigned r){return ((r&0x1f8)<<1)|(r&0x207);}
 CompactMetadataMachine(){
  for(unsigned r=0;r<512;r++)for(unsigned c=0;c<16;c++){physical[r][c]=uint16_t(r*713+c*17);defined[r][c]=true;}
  for(unsigned r=128;r<256;r++)for(unsigned c=0;c<16;c++)Put(r,c,0x3f000000+r*16+c);
  for(auto& row:ready)for(auto& v:row)v=0;
 }
 void Put(unsigned r,unsigned c,uint32_t bits){
  const unsigned hi=Adj(r),lo=hi+8,h=bits>>16;
  assert(!(bits&0x7fffffff) || (bits&0x7f800000));
  physical[hi][c]=uint16_t((h&0x8000)|((h&127)<<8)|((h>>7)&255));physical[lo][c]=uint16_t(bits);
  defined[hi][c]=defined[lo][c]=true;ready[hi][c]=ready[lo][c]=vector.cycle+1;
 }
 uint32_t Get(unsigned r,unsigned c)const{
  const unsigned hi=Adj(r),lo=hi+8,h=physical[hi][c];
  if(!defined[hi][c] || !defined[lo][c] || ready[hi][c]>vector.cycle || ready[lo][c]>vector.cycle)throw Hazard{};
  return (((h&0x8000)|((h&255)<<7)|((h>>8)&127))<<16)|physical[lo][c];
 }
 void Raw(const uint8_t* prefix){
  assert(!dirty);for(unsigned i=0;i<256;i++){
   const unsigned r=i/16,c=i%16,b=prefix[i];physical[r][c]=uint16_t((b<<5)|(b?16:0));defined[r][c]=true;ready[r][c]=vector.cycle;
  }
 }
 void Drain(){drains++;dirty=false;vector.cycle++;}
 void Op(uint32_t op){
  const unsigned code=op>>24;
  if(code==0x70 || code==0x72){
   const unsigned reg=(op>>20)&15,mode=(op>>16)&15,at=op&1023;
   assert(reg<8 && !(at&1) && !(op&0xfc00));if(code==0x72)stores++;
   for(unsigned lane=0;lane<32;lane++){
    const unsigned r=(at&~3U)+lane/8,c=(lane&7)*2+bool(at&2);
    if(code==0x70){
     if(mode==13){assert(r<16);if(!defined[r][c] || ready[r][c]>vector.cycle)throw Hazard{};vector.lanes[reg][lane]=BlackholeRawSfpLoad(physical[r][c],13);}
     else{assert(mode==3 && r>=240 && r<256);vector.lanes[reg][lane]=Get(r,c);}
    }else{assert(mode==3 && r>=128 && r<256);if(stores!=dropStore)Put(r,c,vector.Read(reg,lane));}
   }
   if(code==0x70)vector.ready[reg]=vector.cycle+1;else dirty=true;vector.cycle++;
  }else if(code==0x8c){
   assert(op==0x8c000000);transposes++;
   if(transposes!=dropTranspose){
    unsigned old[8][32];for(unsigned r=0;r<8;r++)for(unsigned l=0;l<32;l++)old[r][l]=vector.Read(r,l);
    for(unsigned b:{0U,4U})for(unsigned r=0;r<4;r++)for(unsigned g=0;g<4;g++)for(unsigned c=0;c<8;c++)vector.lanes[b+r][g*8+c]=old[b+g][r*8+c];
    for(auto& v:vector.ready)v=vector.cycle+1;
   }vector.cycle++;
  }else vector.Op(op);
  assert(vector.okay);
 }
};

static unsigned Differences(const CompactMetadataMachine& m,const std::array<uint8_t,6144>& expanded){
 unsigned wrong=0;
 for(unsigned group=0;group<8;group++)for(unsigned half=0;half<2;half++)for(unsigned kind=0;kind<2;kind++)for(unsigned pad=0;pad<4;pad++)for(unsigned c=0;c<16;c++){
  unsigned expected=0;const unsigned at=4096+(group*4+half*2+kind)*64+c*4;
  if(!pad)for(unsigned b=0;b<4;b++)expected|=unsigned(expanded[at+b])<<(b*8);
  wrong+=m.Get(Compact::Row(group,half)+kind*4+pad,c)!=expected;
 }return wrong;
}

int main(){
 (void)&Round;unsigned seed=0x1583e971,refusals=0;auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 const uint16_t edge[]={0,0x8000,1,0x8001,0x3ff,0x83ff,0x400,0x8400,0x7bff,0xfbff};
 std::array<uint8_t,4608> original{},compact{};std::array<uint8_t,6144> expanded{};
 for(unsigned trial=0;trial<512;trial++){
  for(auto& b:original)b=uint8_t(random());
  for(unsigned row=0;row<32;row++)for(unsigned kind=0;kind<2;kind++){
   const unsigned h=trial<10?edge[(trial+row+kind)%10]:(random()%0x7c00)|(random()&0x8000),at=row*144+kind*2;
   original[at]=uint8_t(h);original[at+1]=uint8_t(h>>8);
  }
  if(trial<2)for(unsigned row=0;row<32;row++)for(unsigned b=4;b<16;b++)original[row*144+b]=trial?255:0;
  assert(PioneerQ38Q4FpuCompactData::Prepare(original.data(),original.size(),compact.data(),compact.size()));
  assert(PioneerQ38Q4FpuBlockData::Prepare(original.data(),original.size(),expanded.data(),expanded.size()));
  assert(std::equal(compact.begin(),compact.begin()+4096,expanded.begin()));
  for(unsigned row=0;row<32;row++)for(unsigned byte=0;byte<16;byte++)assert(compact[4096+(row/16)*256+byte*16+row%16]==original[row*144+byte]);
  CompactMetadataMachine m;uint16_t before[512][16];memcpy(before,m.physical,sizeof(before));
  m.Raw(compact.data()+4096);Compact::Decode<0>(m);m.Raw(compact.data()+4352);Compact::Decode<1>(m);
  assert(!m.dirty && !Differences(m,expanded) && m.stores==72 && m.transposes==12 && m.drains==2);
  for(unsigned r=16;r<256;r++)for(unsigned c=0;c<16;c++)assert(m.physical[r][c]==before[r][c]);
 }
 for(unsigned row=0;row<32;row++)for(unsigned kind=0;kind<2;kind++)for(unsigned mantissa:{0U,1U}){
  auto bad=original;bad[row*144+kind*2]=uint8_t(mantissa);bad[row*144+kind*2+1]=0x7c;
  auto unchanged=compact;assert(!PioneerQ38Q4FpuCompactData::Prepare(bad.data(),bad.size(),compact.data(),compact.size()) && compact==unchanged);refusals++;
 }
 for(unsigned which=0;which<6;which++){
  auto unchanged=compact;
  assert(!PioneerQ38Q4FpuCompactData::Prepare(which==0?nullptr:original.data(),which==1?4607:4608,which==2?nullptr:which==3?original.data():which==4?original.data()+1:compact.data(),which==5?4607:4608));
  assert(compact==unchanged);refusals++;
 }
 unsigned faults=0;
 for(unsigned fault=0;fault<6;fault++){
  CompactMetadataMachine m;if(fault<4)m.dropStore=fault==0?1:fault==1?2:fault==2?71:72;else m.dropTranspose=fault==4?1:12;
  m.Raw(compact.data()+4096);Compact::Decode<0>(m);m.Raw(compact.data()+4352);Compact::Decode<1>(m);
  assert(Differences(m,expanded));faults++;
 }
 printf("PASS_COMPACT_Q4_FPU_METADATA blocks512_all32Outputs8Groups2Kinds_and1536PaddingWords_each;losslessQuant4096_originalMetadata512_total4608_vs6144;exactOldFpuProductNegation_signedZero_subnormalHalf;preservedOtherDst;atomicRefusals%u corruptionControls%u MOCK_COMPLETED_RAW_PREFIX_NOT_COMPILED_WORKER_NATIVE_QUALITY_OR_SPEED\n",refusals,faults);
}
