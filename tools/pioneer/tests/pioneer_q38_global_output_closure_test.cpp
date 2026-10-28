/* SPDX-License-Identifier: MIT */
// Actual SAME worker histories produce the DATA receipts tested below.
// NO fabricated completed owner, processor-state/PC transplantation or ROOT
// launch. 64 down jobs alone do NOT prove all model operators or973 outputs.
#define PIONEER_Q38_GLOBAL_WORKER_ARITHMETIC_MACHINE_ONLY
#include "pioneer_q38_global_worker_arithmetic_test.cpp"
#include "../PioneerQ38GlobalOutputClosure.h"
#include "../PioneerQ38GlobalHeadPlan.h"
#include "../PioneerQ38GlobalChainPackage.h"
#include "../PioneerQ38MixedRecurrentProjectionPlan.h"
namespace Closure=PioneerQ38GlobalOutputClosure;
struct ClosedOutput {
 std::array<uint32_t,128> header={};std::array<uint32_t,64> root={},boot={},frame={},job={},partial={},proof={};
 std::array<uint32_t,16> status={};std::array<uint32_t,8> physical={};unsigned nonce=0;
 bool Valid()const{return Closure::Validate(header.data(),root.data(),boot.data(),frame.data(),nonce,job.data(),partial.data(),proof.data(),status.data(),physical.data());}
};
static unsigned CheckReceipt(const PioneerQ38CanonicalModelPlan& canonical,GlobalArithmeticPeer& peer,const uint32_t* sources){
 ClosedOutput x;auto package=std::make_unique<PioneerQ38GlobalChainPackage>();
 unsigned workspace=canonical.Base+canonical.Workspace(0)->offset;
 assert(package->Build(canonical,0,0,{1,2,3,4},sources,0,workspace+0x600000));
 for(unsigned i=0;i<64;i++){x.root[i]=package->Boot()[i];x.boot[i]=peer.boot[i];x.job[i]=peer.memory.at(M::Job+i*4);x.partial[i]=peer.memory.at(G::Partial+i*4);x.proof[i]=peer.memory.at(0x83f00+i*4);}
 for(unsigned i=0;i<16;i++)x.status[i]=peer.memory.at(M::Status+i*4);
 for(unsigned i=0;i<8;i++)x.physical[i]=peer.Count(i);x.nonce=peer.sequence;
 PioneerQ38GlobalHeadPlan head;assert(head.Build(canonical,0,x.root.data()));
 for(unsigned i=0;i<128;i++)x.header[i]=head.Data()[i];
 unsigned refusals=0,worker=peer.logicalWorker;
 for(unsigned role:{10U,11U,5U}){
  auto plan=std::make_unique<PioneerQ38MixedRecurrentProjectionPlan>();unsigned offset=role==10?0:role==11?1:2;
  assert(plan->Build(canonical,0,role,1,0,0,2025+offset,65,{1,2,3,4},sources));
  for(unsigned i=0;i<64;i++)x.frame[i]=plan->Frame()[i];
  assert(x.Valid()==(worker>3 || role==10));
 }
 auto plan=std::make_unique<PioneerQ38MixedRecurrentProjectionPlan>();
 assert(plan->Build(canonical,0,10,1,0,0,2025,65,{1,2,3,4},sources));
 for(unsigned i=0;i<64;i++)x.frame[i]=plan->Frame()[i];assert(x.Valid());
 auto mutate=[&](auto& words,unsigned first,unsigned count){
  for(unsigned i=first;i<first+count;i++)for(unsigned bit=0;bit<32;bit++){
   words[i]^=1U<<bit;assert(!x.Valid());words[i]^=1U<<bit;refusals++;
  }
 };
 mutate(x.root,0,64);mutate(x.boot,0,64);mutate(x.job,0,64);mutate(x.proof,0,64);
 mutate(x.status,0,16);mutate(x.physical,0,8);mutate(x.partial,0,8);mutate(x.partial,10,54);
 mutate(x.header,16,32);
 const unsigned wave=worker<=21?8:7,ordinal=wave*119+worker-1;
 mutate(x.header,64+(ordinal%7)*8,8);
 for(unsigned i:{6U,7U,16U,17U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U,28U,29U,30U,31U,
  32U,33U,34U,35U,36U,37U,38U,39U,40U,42U,43U,48U,49U,50U,51U,52U,53U,54U,55U,56U,57U})mutate(x.frame,i,1);
 for(unsigned n:{0U,x.nonce-1,x.nonce+1,0x7fffffffU}){unsigned saved=x.nonce;x.nonce=n;assert(!x.Valid());x.nonce=saved;refusals++;}
 for(unsigned bits:{0x7f800000U,0xff800000U,0x7fc00000U}){unsigned saved=x.partial[9];x.partial[9]=bits;assert(!x.Valid());x.partial[9]=saved;refusals++;}
 for(unsigned row:{x.job[28]-1,x.job[28]+x.job[23]*16}){unsigned saved=x.partial[8];x.partial[8]=row;assert(!x.Valid());x.partial[8]=saved;refusals++;}
 // Deliberately show the limit: a finite in-range value alone is DATA, not
 // a proof of numerical maximum, global coverage, committed token or owner.
 {unsigned row=x.partial[8],bits=x.partial[9];x.partial[8]=x.job[28];x.partial[9]=0;assert(x.Valid());x.partial[8]=row;x.partial[9]=bits;}
 assert(x.Valid());
 printf("PASS closed output DATA from ACTUAL worker%u/final wave%u/previous position0 -> recurrent position1 alpha+inactive beta/Z rules/%u one-bit/bounds/nonce/nonfinite refusals;NO ROOT/model-token/commit/native grant\n",worker,wave,refusals);
 return refusals;
}
int main(int argc,char** argv){
 assert(argc==3 || argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::unique_ptr<GlobalOutputOriginalSource> original;
 if(argc==4){original=std::make_unique<GlobalOutputOriginalSource>();assert(original->Open(argv[3]) && original->Ready());}
 unsigned refusals=0,jobs=0;
 for(unsigned worker:{1U,21U,22U,119U}){
  auto m=std::make_unique<GlobalArithmeticPeer>(argv[1],argv[2],worker);m->BootGlobal();
  std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540000+i+1;
  for(unsigned i=0;i<16;i++)sources[worker*16+i]=m->source[i];
  auto output=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(output->Build(*canonical,0,{1,2,3,4},sources.data()));
  for(unsigned layer=0;layer<64;layer++){auto j=m->Command(false);j[1]=m->sequence+1;j[3]=worker;j[4]=m->selfRoute;j[5]=layer;m->Matrix(j);}
  for(unsigned wave=0;wave<(worker<=21?9U:8U);wave++){
   std::array<uint32_t,64> j;assert(output->Job(wave,worker,m->sequence,j.data()));m->Matrix(j,original.get());jobs++;
  }
  refusals+=CheckReceipt(*canonical,*m,sources.data());m->StopGlobal(0);
 }
 if(original)assert(original->Ready());
 printf("PASS actual4 independent QGW1 histories/256 genuine down jobs/%u real output jobs+independent matrix arithmetic/closed receipt DATA%u refusals/real STOP+inert2048;%s;NOT all64 model/973coverage/ROOT admission execution/state commit/native/physical/Work/TPS\n",jobs,refusals,original?"original raw output weights":"synthetic raw weights");
}
