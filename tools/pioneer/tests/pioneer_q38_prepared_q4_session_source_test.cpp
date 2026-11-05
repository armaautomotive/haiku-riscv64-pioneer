/* SPDX-License-Identifier: MIT */
// Actual production preparation/upload/publication helper on an authenticated
// SAME open public model and existing real cache. No accelerator is opened.
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38PreparedQ4SessionSource.h"
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#ifdef NDEBUG
#error Checks required
#endif
using C=PioneerModelWeightCache;
struct Original {
 int fd=-1;FILE* file=nullptr;PioneerQwen38Gguf model;struct stat identity{};C::Digest sha{};
 bool authenticated=false;
 ~Original(){if(file)fclose(file);}
 bool Ready()const{
  struct stat current{};
  return authenticated && model.Stable() && !fstat(fd,&current) && current.st_dev==identity.st_dev && current.st_ino==identity.st_ino
   && current.st_size==identity.st_size && current.st_mtime==identity.st_mtime && current.st_ctime==identity.st_ctime
#ifdef __APPLE__
   && current.st_mtimespec.tv_nsec==identity.st_mtimespec.tv_nsec && current.st_ctimespec.tv_nsec==identity.st_ctimespec.tv_nsec
#else
   && current.st_mtim.tv_nsec==identity.st_mtim.tv_nsec && current.st_ctim.tv_nsec==identity.st_ctim.tv_nsec
#endif
   ;
 }
 const auto& Model()const{return model;}
 bool BindPreparedCache(PioneerQ38NativeQ4WeightCache& cache,const PioneerQwen38PackedPlan& plan,const char* directory,std::string& error){
  const auto recipe=PioneerQ38NativeQ4WeightCache::Recipe(PIONEER_NATIVE_Q4_PREPARATION_SHA256);
  return Ready() && cache.Bind(directory,model,plan,sha,C::Hash(recipe.data(),recipe.size()),[this]{return Ready();},error);
 }
 bool ReadWeightPage(const PioneerQwen38PackedPlan& plan,unsigned f,unsigned page,uint8_t* out,unsigned capacity){
  return Ready() && plan.ReadWeightPage(model,f,page,out,capacity) && Ready();
 }
};
int main(int argc,char** argv){
 assert(argc==3 && argv[1][0]=='/' && argv[2][0]=='/');setvbuf(stdout,nullptr,_IOLBF,0);Original source;std::string error;
 source.fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(source.fd>=0);source.file=fdopen(source.fd,"rb");
 assert(source.file && !fstat(source.fd,&source.identity) && S_ISREG(source.identity.st_mode));
 puts("PREPARED_SESSION_SOURCE authenticating SAME open public original;NO DEVICE");
 assert(C::HashFile(source.fd,PioneerQwen38Gguf::FullBytes,source.sha,error)
  && C::Hex(source.sha)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747" && source.model.Open(source.file));
 source.authenticated=true;assert(source.Ready());PioneerQwen38PackedPlan layout;assert(layout.Build(source.model,8192,true,true));
 auto prepared=std::make_unique<PioneerQ38PreparedQ4SessionSource<Original>>();
 if(!prepared->Build(source,layout,argv[2])){fprintf(stderr,"PREPARED_SESSION_SOURCE_FAILED %s\n",prepared->error.c_str());return 1;}
 assert(prepared->Ready() && prepared->Spans().size()==4895 && prepared->Bytes()==UINT64_C(20470759424));
 const auto& stats=prepared->PreparationStatistics();
 assert(stats.fragments==1351 && stats.manifestHits==1351 && stats.bundleHits==1351 && !stats.convertedTiles);
 auto packet=std::make_unique<PioneerQ38PreparedSource::Packet>();unsigned phases=0;std::array<uint8_t,4096> page{};
 for(unsigned step=0;step<194;step++){
  namespace P=PioneerQ38PreparedSource;const unsigned op=step==0?P::Begin:step==193?P::Seal:P::Phase;
  assert(prepared->Publication(op,step==0?0:step==193?192:step-1,*packet) && P::RequestShape(*packet));
  if(op==P::Phase){assert(PioneerQ38PreparedQ4RootLayout::Header(packet->request.data,phases/3,phases%3+2));phases++;}
 }
 unsigned sampled=0;
 for(unsigned s=0;s<prepared->Spans().size();s++){
  const auto& span=prepared->Spans()[s];
  for(unsigned at:{0U,span.allocated/4096-1}){assert(prepared->ReadPage(s,at,page.data(),page.size()));sampled++;}
 }
 assert(phases==192 && source.Ready() && prepared->Ready());
 printf("PASS_REAL_PREPARED_SESSION_SOURCE production_helper4895_spans bytes%llu publications194 phases%u sampledPages%u "
  "SAME_ORIGINAL_FD warm1351 converted0 no_device_or_CPU_inference_OR_TPS\n",(unsigned long long)prepared->Bytes(),phases,sampled);
}
