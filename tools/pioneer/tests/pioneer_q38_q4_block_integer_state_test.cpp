/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_OFFBOARD 1
#include "../PioneerQ38Q4BlockIntegerState.h"
#define PIONEER_Q38_VECTOR_Q4_BLOCK_MACHINE_ONLY
#include "pioneer_q38_vector_q4_block_test.cpp"
#include <set>
#ifdef NDEBUG
#error Assertions required
#endif
using State=PioneerQ38Q4BlockIntegerState;
struct IntegerStateMachine:BlockMetadataMachine {
 bool zeroPending=false,skipZero=false,keepDot=false;
 unsigned zeroUnpacks=0,casts=0,mads=0;
 explicit IntegerStateMachine(unsigned raw):BlockMetadataMachine(raw){
  for(unsigned r=112;r<128;r++)for(unsigned c=0;c<16;c++)defined[Adj(r)][c]=defined[Adj(r)+8][c]=false;
  for(unsigned r=108;r<112;r++)for(unsigned c=0;c<16;c++)StoreFloat(r,c,0);
  vector.cycle++;
 }
 void Config(unsigned index,unsigned value){if(zeroPending)throw Hazard{};BlockMetadataMachine::Config(index,value);}
 void Drain(){
  if(zeroPending && drains+1!=dropDrain){
   if(!skipZero)for(unsigned r=112;r<128;r++)for(unsigned c=0;c<16;c++)StoreFloat(r,c,0);
   zeroPending=false;
  }
  BlockMetadataMachine::Drain();
 }
 void Op(unsigned op){
  if(zeroPending)throw Hazard{};
  if(op==0x42000091){
   assert(!pending && !dirty && !thread5 && config[1]==0x20200000 && config[50]==0x100
    && config[64]==0x100010 && config[65]==16 && config[72]==0 && config[73]==0x13
    && config[76]==0x9fff && config[84]==0x00400740);
   zeroPending=true;zeroUnpacks++;events++;return;
  }
  const unsigned code=op>>24;
  if((code==0x70 && ((op>>16)&15)==3) || code==0x72){
   if(pending || !enabled || thread5!=4)throw Hazard{};events++;
   const unsigned reg=(op>>20)&15,mode=(op>>16)&15,at=op&1023;
   assert(reg<8 && mode==3 && !(at&1) && !(op&0xfc00));
   if(code==0x72)stores++;
   for(unsigned lane=0;lane<32;lane++){
    const unsigned row=(at&~3U)+lane/8,col=(lane&7)*2+bool(at&2);
    if(code==0x70){
     assert((row>=32 && row<36) || (row>=108 && row<124) || (row>=128 && row<192));
     vector.lanes[reg][lane]=FloatDst(row,col);
    }else{
     assert((row>=108 && row<124) || (row>=128 && row<192));
     if(!defined[Adj(row)][col] || !defined[Adj(row)+8][col])throw Hazard{};
     if(stores!=dropStore)StoreFloat(row,col,vector.Read(reg,lane));
    }
   }
   if(code==0x70)vector.ready[reg]=vector.cycle+1;else dirty=true;
   vector.cycle++;return;
  }
  if(code==0x84){
   assert(op==0x84001440 || op==0x84023550 || op==0x84001660 || op==0x84023770);
   if(pending || !enabled || thread5!=4)throw Hazard{};
   unsigned a=(op>>16)&15,b=(op>>12)&15,c=(op>>8)&15,d=(op>>4)&15,out[32];
   for(unsigned lane=0;lane<32;lane++)out[lane]=PioneerBlackholeFmaBits(vector.Read(a,lane),vector.Read(b,lane),vector.Read(c,lane));
   for(unsigned lane=0;lane<32;lane++)vector.lanes[d][lane]=out[lane];
   vector.ready[d]=vector.cycle+2;vector.cycle++;events++;mads++;return;
  }
  if(op==0x90000330){
   if(pending || !enabled || thread5!=4)throw Hazard{};
   unsigned out[32];
   for(unsigned lane=0;lane<32;lane++){
    const unsigned sm=vector.Read(3,lane),magnitude=sm&0x7fffffff;
    assert(magnitude<=14336 && (magnitude || !sm));
    out[lane]=PioneerFmaBits(float(int(magnitude)*(sm>>31?-1:1)));
   }
   for(unsigned lane=0;lane<32;lane++)vector.lanes[3][lane]=out[lane];
   vector.ready[3]=vector.cycle+1;vector.cycle++;events++;casts++;return;
  }
  if(keepDot && stores==38 && (op==0x71420000 || op==0x71620000)){events++;vector.cycle++;return;}
  BlockMetadataMachine::Op(op);
 }
 void Closed()const{if(zeroPending)throw Hazard{};BlockMetadataMachine::Closed();}
 void Dot(unsigned panel,const std::array<int,256>& input){
  Closed();
  // Only a synthetic completed-FPU endpoint. This does NOT qualify the
  // actual UNPACK/MVMUL/source handoffs or replace them in a native worker.
  for(unsigned row=0;row<16;row++){
   int dot=0;
   for(unsigned k=0;k<16;k++){
    const unsigned index=panel*16+k,at=(16+(index/64)*32+index%32)*16+row;
    const int q=(tile[at]>>((index/32%2)*4))&15;dot+=(q-7)*input[index];
   }
   for(unsigned r=32;r<40;r++)StoreFloat(r,row,PioneerFmaBits(float(dot)));
  }
  vector.cycle++;
 }
};
static unsigned Factor(const uint8_t* t,unsigned g,unsigned col,bool minimum){
 if(g<4)return t[((minimum?8:4)+g)*16+col]&63;
 const unsigned v=t[(8+g)*16+col];
 return minimum?(v>>4)|((t[(4+g)*16+col]>>6)<<4):(v&15)|((t[g*16+col]>>6)<<4);
}
static unsigned HalfBits(const uint8_t* t,unsigned col,unsigned kind){
 return t[kind*32+col]|(unsigned(t[kind*32+16+col])<<8);
}
static unsigned MetadataCheck(const IntegerStateMachine& m){
 unsigned errors=0;
 for(unsigned col=0;col<16;col++){
  for(unsigned row=0;row<4;row++)for(unsigned kind=0;kind<2;kind++){
   const auto bits=PioneerFmaBits(PioneerHalfFloat(uint16_t(HalfBits(m.tile.data(),col,kind))));
   errors+=m.FloatDst((kind?State::Minimum:State::Base)+row,col)!=bits;
  }
  for(unsigned g=0;g<8;g++)for(unsigned row=0;row<4;row++)for(unsigned kind=0;kind<2;kind++)
   errors+=m.FloatDst(128+g*8+kind*4+row,col)!=(row?0:PioneerFmaBits(float(Factor(m.tile.data(),g,col,kind))));
 }
 return errors;
}
static unsigned Run(IntegerStateMachine& m,State& state,unsigned blocks,unsigned seed){
 assert(state.Init(m));m.Closed();
 for(unsigned r=112;r<128;r++)for(unsigned c=0;c<16;c++)assert(!m.FloatDst(r,c));
 unsigned errors=0;
 auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 const unsigned halves[]={0,0x8000,1,0x8001,0x3ff,0x83ff,0x400,0x8400,0x3c00,0xbc00,0x7bff,0xfbff};
 for(unsigned block=0;block<blocks;block++){
  for(auto& b:m.tile)b=uint8_t(random());
  for(unsigned c=0;c<16;c++)for(unsigned kind=0;kind<2;kind++){
   const unsigned h=seed&1?halves[(c+block+kind*3)%12]:(random()%0x7c00)|(random()&0x8000);
   m.tile[kind*32+c]=uint8_t(h);m.tile[kind*32+16+c]=uint8_t(h>>8);
  }
  std::array<uint32_t,576> words;memcpy(words.data(),m.tile.data(),2304);assert(V::FiniteBlock(words.data(),2304));
  uint32_t before[4][16];for(unsigned r=0;r<4;r++)for(unsigned c=0;c<16;c++)before[r][c]=m.FloatDst(108+r,c);
  assert(state.Begin(m,m.base));m.Closed();errors+=MetadataCheck(m);m.Config(1,0x60aa0000);
  std::array<int,256> input;for(auto& x:input)x=int(random()%256)-128;
  if(block%3==1)input.fill(-128);
  if(block%3==2)input.fill(0);
  int64_t total[16]={},minimum[16]={};
  for(unsigned p=0;p<16;p++){
   int sum=0;for(unsigned k=0;k<16;k++)sum+=input[p*16+k];
   for(unsigned c=0;c<16;c++)for(unsigned k=0;k<16;k++){
    const unsigned i=p*16+k,at=(16+(i/64)*32+i%32)*16+c;
    const unsigned q=(m.tile[at]>>((i/32%2)*4))&15;
    total[c]+=int64_t(q)*input[i]*Factor(m.tile.data(),p/2,c,false);
    minimum[c]+=int64_t(input[i])*Factor(m.tile.data(),p/2,c,true);
   }
   m.Dot(p,input);assert(state.Panel(m,p,sum));m.Closed();
   for(unsigned r=0;r<4;r++)for(unsigned c=0;c<16;c++)assert(m.FloatDst(108+r,c)==before[r][c]);
  }
  const unsigned scale=block&1?0xbb812345:0x3f812345;assert(state.Finish(m,scale));m.Closed();
  for(unsigned r=0;r<4;r++)for(unsigned c=0;c<16;c++){
   const float d=PioneerHalfFloat(uint16_t(HalfBits(m.tile.data(),c,0))),dm=PioneerHalfFloat(uint16_t(HalfBits(m.tile.data(),c,1)));
   volatile float a=d*PioneerFmaFloat(scale),b=(-dm)*PioneerFmaFloat(scale);
   volatile float product=a*float(r?0:total[c]),correction=b*float(r?0:minimum[c]),value=product+correction;
   errors+=m.FloatDst(108+r,c)!=PioneerFmaBits(PioneerFmaFloat(before[r][c])+value);
  }
 }
 return errors;
}
#ifndef PIONEER_Q38_Q4_BLOCK_INTEGER_STATE_MACHINE_ONLY
int main(){
 const unsigned bases[]={0x90000,0x98000,0x98900,0x9a000,0x9a900};unsigned jobs=0,refusals=0,faults=0;
 std::set<unsigned> allowed;for(unsigned r:{108U,109U,110U,111U}){allowed.insert(IntegerStateMachine::Adj(r));allowed.insert(IntegerStateMachine::Adj(r)+8);}
 for(unsigned r=112;r<192;r++){allowed.insert(IntegerStateMachine::Adj(r));allowed.insert(IntegerStateMachine::Adj(r)+8);}
 for(unsigned r=32;r<40;r++){allowed.insert(IntegerStateMachine::Adj(r));allowed.insert(IntegerStateMachine::Adj(r)+8);}
 for(unsigned r=0;r<16;r++)allowed.insert(r);
 assert(allowed.size()==200);
 for(unsigned trial=0;trial<256;trial++){
  auto m=std::make_unique<IntegerStateMachine>(bases[trial%5]);State state;
  uint16_t before[512][16];memcpy(before,m->physical,sizeof(before));
  assert(!Run(*m,state,3,0x35813261+trial*631));
  for(unsigned r=0;r<512;r++)if(!allowed.count(r))for(unsigned c=0;c<16;c++)assert(before[r][c]==m->physical[r][c]);
  assert(m->zeroUnpacks==1 && m->unpacks==3 && !state.active && !state.failed && state.next==16);
  assert(m->mads==3*16*4 && m->casts==3*16*4);jobs+=3;
 }
 for(unsigned mode=0;mode<8;mode++){
  IntegerStateMachine m(0x98000);State state;
  if(mode){assert(state.Init(m));m.tile.fill(0);assert(state.Begin(m,m.base));}
  const unsigned events=m.events;bool accepted=false;
  if(mode==0)accepted=state.Begin(m,m.base);
  if(mode==1)accepted=state.Begin(m,m.base);
  if(mode==2)accepted=state.Panel(m,1,0);
  if(mode==3)accepted=state.Panel(m,16,0);
  if(mode==4)accepted=state.Panel(m,0,2049);
  if(mode==5)accepted=state.Panel(m,0,-2049);
  if(mode==6)accepted=state.Finish(m,0x3f800000);
  if(mode==7)accepted=state.Init(m);
  assert(!accepted && state.failed && m.events==events);
  assert(!state.Begin(m,m.base) && !state.Panel(m,0,0) && !state.Finish(m,0x3f800000) && !state.Init(m) && m.events==events);refusals++;
 }
 for(unsigned base:{0U,0x90001U,0x94000U,0x98010U,0x9b200U,UINT32_MAX}){
  IntegerStateMachine m(base);State state;assert(state.Init(m));const unsigned events=m.events;
  assert(!state.Begin(m,base) && state.failed && m.events==events);refusals++;
 }
 for(unsigned scale:{0x2f7fffffU,0x47800001U,0x7f800000U,0x7fc00000U}){assert(!State::Scale(scale));refusals++;}
 for(unsigned scale:{0U,0x80000000U,0x2f800000U,0xaf800000U,0x47800000U})assert(State::Scale(scale));
 const unsigned badStores[]={1,2,19,20,37,38},badDrains[]={2,5,7,24};
 for(unsigned fault=0;fault<14;fault++){
  IntegerStateMachine m(0x98000);State state;
  if(fault==0)m.skipZero=true;
  if(fault>=1 && fault<=6)m.dropStore=badStores[fault-1];
  if(fault==7 || fault==8)m.dropTranspose=fault==7?1:6;
  if(fault>=9 && fault<=12)m.dropDrain=badDrains[fault-9];
  if(fault==13)m.keepDot=true;
  bool caught=false;try{caught=Run(m,state,1,0x39a05823)!=0;}catch(const IntegerStateMachine::Hazard&){caught=true;}
  assert(caught);faults++;
 }
 printf("PASS OFFBOARD emitted block-integer state jobs%u fullPhysicalDstWords%u;rawMetadata256B_basesAndFactors_allPadding_originalQdotReconstruction_splitHalfStorage_bothParities_preservedOldAccumulatorUntilFinish_unrelatedPhysicalRows;all5addresses explicitScratchZeroUnpack;refusals%u faultControls%u;SYNTHETIC_COMPLETED_DOT_ENDPOINT_NOT_COMPILED_WORKER_NATIVE_QUALITY_OR_SPEED\n",jobs,jobs*64,refusals,faults);
 (void)&Check;(void)&Round;
}
#endif
