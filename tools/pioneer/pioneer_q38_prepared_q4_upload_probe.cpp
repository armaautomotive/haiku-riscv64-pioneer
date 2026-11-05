/* SPDX-License-Identifier: MIT */
// Existing real cache -> complete upload/expected-read DATA proof ONLY.
// Never opens a driver,executes inference,reads chats or emits learned data.
#include "PioneerQ38PreparedQ4UploadPlan.h"
#include "PioneerQ38PreparedQ4RootTables.h"
#include "PioneerQwen38FleetPlan.h"
#include <fcntl.h>
#include <unistd.h>
#include <set>
#include <cassert>
#if defined(NDEBUG) || !defined(PIONEER_NATIVE_Q4_PREPARATION_SHA256)
#error Source-fingerprinted original cache recipe and checks required
#endif
using Cache=PioneerModelWeightCache;
static bool Same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int main(int argc,char** argv){
 assert(argc==4 && argv[1][0]=='/' && argv[2][0]=='/' && !strcmp(argv[3],"--existing-cache"));setvbuf(stdout,nullptr,_IOLBF,0);
 const int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);FILE* file=fdopen(fd,"rb");assert(file);
 struct stat before{},after{};assert(!fstat(fd,&before) && S_ISREG(before.st_mode));std::string error;Cache::Digest sha;
 puts("PREPARED_Q4_UPLOAD_AUTH_SAME_OPEN_ORIGINAL_READ_ONLY_NO_DEVICE");
 assert(Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error)
  && Cache::Hex(sha)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 auto model=std::make_unique<PioneerQwen38Gguf>();assert(model->Open(file) && model->Tensors().size()==851);
 auto original=std::make_unique<PioneerQwen38PackedPlan>();assert(original->Build(*model,8192,true,true));
 auto ready=[&]{return model->Stable() && !fstat(fd,&after) && Same(before,after);};
 const auto recipe=PioneerQ38NativeQ4WeightCache::Recipe(PIONEER_NATIVE_Q4_PREPARATION_SHA256);
 const auto conversion=Cache::Hash(recipe.data(),recipe.size());
 auto weights=std::make_unique<PioneerQ38NativeQ4WeightCache>();
 assert(weights->Bind(argv[2],*model,*original,sha,conversion,ready,error));PioneerQ38NativeQ4WeightCache::Statistics stats;
 while(!weights->AllPrepared()){
  // Existing verified hits only;refuse a cold cache rather than launching
  // a conversion. This is still pre-device preparation,not Work execution.
  if(!weights->PrepareNext(stats,error,[](uint64_t,uint64_t,bool verifying){return verifying;})){
   fprintf(stderr,"EXISTING_PREPARED_CACHE_REFUSED %s\n",error.c_str());return 1;
  }
  assert(!stats.convertedTiles && stats.manifestHits==stats.fragments && stats.bundleHits==stats.fragments);
  if(!(stats.fragments%128))printf("PREPARED_Q4_UPLOAD_EXISTING fragments%llu converted0\n",(unsigned long long)stats.fragments);
 }
 PioneerQ38NativeQ4CachedPages pages;assert(pages.Bind(argv[2],*weights,ready,error));
 auto plan=std::make_unique<PioneerQ38PreparedQ4UploadPlan<>>();
 if(!plan->Build(*weights,pages,error)){fprintf(stderr,"PREPARED_Q4_UPLOAD_PLAN_FAILED %s\n",error.c_str());return 1;}
 const auto& layout=weights->OriginalPlan();std::set<unsigned> selected;unsigned embedding=0,originals=0,primary=0,corrections=0;
 for(const auto& span:plan->Spans()){
  const auto& p=layout.Parts()[span.fragment];
  if(span.kind==plan->Correction)corrections++;
  else if(span.kind==plan->PreparedPrimary){selected.insert(p.tensor);primary++;}
  else{originals++;if(p.layer==64 && p.role==0)embedding++;}
 }
 assert(stats.fragments==1351 && selected.size()==192 && primary==1344 && corrections==1344 && embedding==7 && originals==2207);
 unsigned jobs=0;uint64_t extraReads=0,extraFlits=0;const auto fillsBefore=pages.WindowFills();
 for(unsigned tensor:selected){
  PioneerQwen38FleetPlan fleet;assert(fleet.Build(*model,layout,tensor,119,16) && fleet.Jobs().size()==119);
  for(const auto& part:fleet.Jobs()){
   PioneerQ38PreparedQ4UploadPlan<>::Job job;
   if(!plan->Describe(part.fragment,part.firstRow,part.groups,part.worker+1,17,job,error)){
    fprintf(stderr,"PREPARED_Q4_UPLOAD_JOB_FAILED %s\n",error.c_str());return 1;
   }
   assert(job.descriptor[6]==layout.Base+part.proposedOffset && job.descriptor[7]==PioneerQ38ProjectionRoute(part.bank));
   extraReads+=job.extraReads;extraFlits+=job.extraFlits;jobs++;
  }
 }
 assert(jobs==22848 && pages.WindowFills()==fillsBefore && plan->Ready() && ready());
 auto tables=std::make_unique<PioneerQ38PreparedQ4RootTables<>>();assert(tables->Build(*plan,*model,error));
 using Root=PioneerQ38PreparedQ4RootLayout;std::array<unsigned,Root::Bytes/4> table{};unsigned bindings=0;
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=2;role<=4;role++){
  assert(tables->Read(layer,role,0,reinterpret_cast<uint8_t*>(table.data()),Root::Bytes) && Root::Header(table.data(),layer,role));
  for(unsigned w=0;w<119;w++){
   const auto* d=table.data()+16+w*16;const auto* ledger=table.data()+Root::LedgerOffset/4+w*4;unsigned job[16]={},output[16]={},reads=0,flits=0;
   job[1]=12;job[2]=2304;job[5]=d[4];job[6]=d[6];job[7]=d[5];job[8]=d[7];job[11]=w+1;job[12]=17;job[14]=1;
   assert(Root::Bind(table.data(),layer,role,d,ledger,job,ledger[2],ledger[3],output,reads,flits)
    && output[2]==17 && reads==ledger[0] && flits==ledger[1]);bindings++;
  }
 }
 assert(bindings==22848 && pages.WindowFills()==fillsBefore);
 auto raw=[&](unsigned f,unsigned page,uint8_t* out,unsigned capacity){return ready() && layout.ReadWeightPage(*model,f,page,out,capacity) && ready();};
 std::array<uint8_t,4096> output{};unsigned sampled=0;
 for(unsigned s=0;s<plan->Spans().size();s++){
  const auto& span=plan->Spans()[s];
  for(unsigned page:{0U,span.allocated/4096-1}){
   if(!plan->ReadPage(s,page,output.data(),output.size(),raw,error)){
    fprintf(stderr,"PREPARED_Q4_UPLOAD_PAGE_FAILED %s\n",error.c_str());return 1;
   }sampled++;
  }
 }
 for(unsigned bank=0;bank<7;bank++)printf("PREPARED_Q4_UPLOAD_BANK%u total%llu spare%llu;PROPOSAL_NOT_NATIVE_OWNERSHIP\n",bank,
  (unsigned long long)plan->Used(bank),(unsigned long long)(layout.Capacity-plan->Used(bank)));
 assert(plan->Ready() && ready() && !fstat(fd,&after) && Same(before,after));
 printf("CLOSED_REAL_PREPARED_Q4_UPLOAD_PLAN originalSpans%u preparedPrimaries%u correctionSpans%u originalEmbeddingStripes%u "
  "jobs%u independentlyPlannedExtraReads%llu extraFlits%llu sampledPages%u bytes%llu warmFragments%llu converted0 "
  "ROOTBindings%u jobPlanningDiskRefills0;ALL851_full8192_twoState_NO_NATIVE_ROOT_CARD_INFERENCE_OR_TPS\n",originals,primary,corrections,embedding,jobs,
  (unsigned long long)extraReads,(unsigned long long)extraFlits,sampled,(unsigned long long)plan->Bytes(),(unsigned long long)stats.fragments,bindings);
 weights.reset();model.reset();assert(!fclose(file));
}
