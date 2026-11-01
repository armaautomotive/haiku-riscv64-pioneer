/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include "../PioneerQ38PushedWorkerLayout.h"
#include "../PioneerQ38FlatMatrixGrammar.h"
namespace Old=PioneerQ38PushedWorkerLayout;
namespace Flat=PioneerQ38FlatMatrixGrammar;
using Words=std::array<uint32_t,64>;
static_assert(unsigned(Old::Magic)==unsigned(Flat::Magic)
 && unsigned(Old::JobWitness)==unsigned(Flat::JobWitness)
 && unsigned(Old::ReceiptMagic)==unsigned(Flat::ReceiptMagic)
 && unsigned(Old::Mailbox)==unsigned(Flat::Mailbox)
 && unsigned(Old::PushBuffer)==unsigned(Flat::PushBuffer),"same DATA ABI,not a new producer grant");
static uint64_t checks=0,accepted=0,refused=0;
static uint32_t randomState=0x7b35a19d;
static uint32_t Next(){randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;return randomState;}
static void Check(const Words& boot,const Words& job,unsigned previous=0,unsigned phase=0){
 const auto savedBoot=boot,savedJob=job;
 assert(Flat::BootShape(boot.data())==Old::BootShape(boot.data()));
 const bool expected=Old::JobShape(job.data(),boot.data(),previous,phase);
 const bool actual=Flat::JobShape(job.data(),boot.data(),previous,phase);
 assert(Flat::OrdinaryMatrixJobShape(job.data(),boot.data(),previous,phase)==(job[2]==1 && expected));
 if(actual!=expected){
  fprintf(stderr,"MISMATCH check%llu/worker%u/previous%08x/phase%08x/expected%u/actual%u\n",
   (unsigned long long)checks,boot[3],previous,phase,expected,actual);
  for(unsigned i=0;i<64;i++)fprintf(stderr,"%u boot%08x job%08x\n",i,boot[i],job[i]);
  assert(false);
 }
 assert(boot==savedBoot && job==savedJob);checks++;if(actual)accepted++;else refused++;
}
static std::pair<Words,Words> Seed(unsigned worker,unsigned sequence=1,unsigned layer=0){
 Words boot={},job={};
 boot[0]=Old::Magic;boot[1]=1;boot[2]=64;boot[3]=worker;boot[4]=Old::Peer(worker);
 boot[5]=boot[16]=0x10001;boot[6]=8192;boot[7]=64;
 for(unsigned i=8;i<16;i++)boot[i]=0x12345678U+i;
 for(unsigned i=17;i<32;i++)boot[i]=0x17ac9000U+i;
 for(unsigned i=40;i<=54;i++){boot[i]=Old::BootWord(i);assert(boot[i]!=0);}
 job[0]=Old::Magic;job[1]=sequence;job[2]=1;job[3]=worker;job[4]=boot[4];
 job[5]=layer;job[6]=57;job[7]=0x1234;
 for(unsigned i=8;i<16;i++)job[i]=boot[i];
 for(unsigned i=16;i<26;i++)job[i]=0x6abb9000U+i; // coarse grammar permits arbitrary capsule DATA
 assert(Old::BootShape(boot.data()) && Old::JobShape(job.data(),boot.data(),sequence-1,0));
 return {boot,job};
}
static std::pair<Words,Words> Operation(unsigned worker,unsigned op){
 auto pair=Seed(worker);auto& b=pair.first;auto& j=pair.second;j[2]=op;
 if(op==2 || op==3 || op>=9)for(unsigned i=16;i<64;i++)j[i]=0;
 if(op==3){j[16]=Old::ActivationMagic;j[17]=256;j[18]=576;j[19]=j[7];j[20]=12;j[27]=123;}
 if(op>=9 && op<=11)j[5]=3;
 if(op==12){
  j[5]=64;j[16]=0x31483851;j[17]=14;j[18]=3392;j[21]=5120;j[22]=0x04000000;
  j[23]=1;j[24]=786;j[25]=j[26]=1;j[28]=16*(worker-1);j[29]=248320;j[30]=1;j[31]=Old::JobWitness;
 }
 assert(b[3]==worker);return pair;
}
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned worker=1;worker<=119;worker++){
  assert(Flat::Peer(worker)==Old::Peer(worker));
  for(unsigned kind=0;kind<2;kind++){
   auto [boot,job]=Seed(worker);job[26]=kind;Check(boot,job);
   // Every bit of EVERY boot/job word,not only fields expected to refuse.
   // Genuine allowed source/capsule variations remain accepted by BOTH.
   for(unsigned field=0;field<128;field++)for(unsigned bit=0;bit<32;bit++){
    auto b=boot,j=job;(field<64?b[field]:j[field-64])^=uint32_t(1)<<bit;Check(b,j);
   }
  }
 }
 puts("PASS flat ordinary-matrix ALL119 workers/both flags/ALL128x32 one-bit variations;old DATA predicate unchanged");
 for(unsigned worker=1;worker<=119;worker++)for(unsigned op=2;op<=12;op++){
  auto [boot,job]=Operation(worker,op);const unsigned phase=op>=9 && op<=11?op-9:0;
  Check(boot,job,0,phase);
  if(worker==1 || worker==21 || worker==68 || worker==96 || worker==119)
   for(unsigned field=0;field<128;field++)for(unsigned bit=0;bit<32;bit++){
    auto b=boot,j=job;(field<64?b[field]:j[field-64])^=uint32_t(1)<<bit;Check(b,j,0,phase);
   }
  if(op==2){job[5]=64;Check(boot,job);}
  if(op==12){for(unsigned wave=0;wave<10;wave++){job[27]=wave;Check(boot,job);}}
 }
 const unsigned edges[]={0,1,2,3,63,64,67,68,95,96,118,119,120,255,256,8191,8192,
  248319,248320,0x7ffffffd,0x7ffffffe,0x7fffffff,0x80000000,0xfffffff8,UINT32_MAX};
 auto [baseBoot,baseJob]=Seed(1);
 for(unsigned field=0;field<128;field++)for(unsigned value:edges){
  auto b=baseBoot,j=baseJob;(field<64?b[field]:j[field-64])=value;Check(b,j);
 }
 for(unsigned previous:edges)for(unsigned phase:edges){
  auto j=baseJob;j[1]=previous+1;Check(baseBoot,j,previous,phase);
 }
 for(unsigned op=2;op<=12;op++)for(unsigned previous:edges)for(unsigned phase:edges){
  auto [b,j]=Operation(1,op);j[1]=previous+1;Check(b,j,previous,phase);
 }
 assert(Flat::OrdinaryMatrixJobShape(baseJob.data(),baseBoot.data(),0,0xfffffff8U));
 for(unsigned operation=0;operation<32;operation++){
  auto j=baseJob;j[2]=operation;Check(baseBoot,j);
 }
 // Multiple simultaneous edits,including preserving correlated nonce/peer/
 // source fields rather than testing only trivially invalid random headers.
 for(unsigned trial=0;trial<200000;trial++){
  auto [b,j]=Seed(1+Next()%119,1+Next()%0x7ffffffeU,Next()%64);
  const unsigned previous=j[1]-1;
  if(trial&1){b[5]=b[16]=1+Next()%0xfffffffeU;for(unsigned i=8;i<16;i++)j[i]=b[i]=Next();}
  for(unsigned count=1+Next()%8;count;count--){unsigned field=Next()%128;(field<64?b[field]:j[field-64])^=Next();}
  Check(b,j,previous,(trial%5)?0:Next());
 }
 assert(!Flat::BootShape(nullptr));
 assert(!Flat::JobShape(nullptr,baseBoot.data(),0,0));
 assert(!Flat::JobShape(baseJob.data(),nullptr,0,0));
 assert(!Flat::OrdinaryMatrixJobShape(nullptr,baseBoot.data(),0,0));
 assert(!Flat::OrdinaryMatrixJobShape(baseJob.data(),nullptr,0,0));
 assert(!Flat::OrdinaryMatrixJobShape(nullptr,nullptr,0,0));
 Check(baseBoot,baseBoot); // complete alias is non-matrix;NO write through either pointer
 printf("PASS flat DATA grammar/%llu differential cases/%llu accepted/%llu refused/null+alias+unsigned-boundaries/read-only inputs;"
  "NO ROOT selection,native ownership,device execution,model result or timing grant\n",
  (unsigned long long)checks,(unsigned long long)accepted,(unsigned long long)refused);
 return 0;
}
