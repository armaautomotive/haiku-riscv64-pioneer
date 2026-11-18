/* SPDX-License-Identifier: MIT */
// Exact USER producer body with a read-only offline authentication adapter.
// Never opens the P100A, writes the GGUF or publishes data to a device.
#include "../PioneerQ38NativeQ8FullSource.h"
#include "../PioneerModelWeightCache.h"
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <array>
#include <cassert>
#include <fcntl.h>
#include <memory>
#include <unistd.h>

namespace Q=PioneerQ38NativeQ8FullSource;
using Codec=PioneerQ38NativeQ8Data;
using Page=std::array<uint8_t,4096>;

struct AuthenticatedFile {
 std::unique_ptr<FILE,decltype(&fclose)> file{nullptr,&fclose};
 PioneerQwen38Gguf model;
 struct stat identity={};
 unsigned calls=0,refuseAt=0;
 bool authenticated=false;
 explicit AuthenticatedFile(const char* path){
  const int fd=open(path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
  assert(fd>=0 && !fstat(fd,&identity) && S_ISREG(identity.st_mode));
  file.reset(fdopen(fd,"rb"));
  PioneerModelWeightCache::Digest hash={};std::string error;
  assert(file && PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,hash,error));
  assert(PioneerModelWeightCache::Hex(hash)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
  authenticated=true;assert(model.Open(file.get()) && Ready());
 }
 bool Ready(){
  if(++calls==refuseAt)return false;
  struct stat now={};
  if(!authenticated || !model.Stable() || fstat(fileno(file.get()),&now) || now.st_dev!=identity.st_dev
   || now.st_ino!=identity.st_ino || now.st_size!=identity.st_size
   || now.st_mtime!=identity.st_mtime || now.st_ctime!=identity.st_ctime)return false;
#if defined(__APPLE__)
  return now.st_mtimespec.tv_nsec==identity.st_mtimespec.tv_nsec && now.st_ctimespec.tv_nsec==identity.st_ctimespec.tv_nsec;
#else
  return now.st_mtim.tv_nsec==identity.st_mtim.tv_nsec && now.st_ctim.tv_nsec==identity.st_ctim.tv_nsec;
#endif
 }
#include "pioneer_q38_native_q8_full_source_page.inc"
};

int main(int argc,char** argv){
 assert(argc==2 && argv[1][0]=='/');setvbuf(stdout,nullptr,_IOLBF,0);
 AuthenticatedFile source(argv[1]);PioneerQwen38PackedPlan producer,oracle;
 assert(producer.Build(source.model,8192,true,true) && oracle.Build(source.model,8192,true,true));
 auto driver=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(driver->Build() && driver->Count()==4678 && producer.Parts().size()==driver->Count());
 unsigned rawParts=0,q8Parts=0,firstNative=UINT32_MAX,checked=0;
 uint64_t quants=0,changed=0,uploaded=0;
 std::set<unsigned> tensors;
 for(unsigned f=0;f<producer.Parts().size();f++){
  const auto& p=producer.Parts()[f];
  const auto& d=*driver->Part(f);
  assert(p.kind==d.kind && p.layer==d.layer && p.role==d.role && p.bank==d.bank && p.firstRow==d.firstRow
   && p.rows==d.rows && p.rowBytes==d.rowBytes && p.proposedAddress==driver->Base+d.offset
   && p.bytes==d.bytes && p.allocated==d.allocated);
  if(p.kind!=PIONEER_Q38_WEIGHT)continue;
  const auto& t=source.model.Tensors().at(p.tensor);
  assert(t.type==d.type);
  const bool native=t.type==8;
  assert(Q::Layout(p,t)==(native?Q::Format::SignedMagnitude:Q::Format::Raw));
  if(native){
   ++q8Parts;tensors.insert(p.tensor);firstNative=std::min(f,firstNative);
   quants+=p.bytes/576*uint64_t(512);
  }else ++rawParts;
  uploaded+=p.allocated;
  unsigned pages=p.allocated/4096;
  // Every fragment's first, middle and final pages. All pages of the
  // first Q8 fragment also cover all nine page/tile phase alignments.
  for(unsigned at=0;at<pages;at++){
   if(f!=firstNative && at && at!=pages/2 && at+1!=pages)continue;
   Page raw={},out={};
   assert(oracle.ReadWeightPage(source.model,f,at,raw.data(),raw.size()));
   assert(source.ReadWeightPage(producer,f,at,out.data(),out.size()));
   for(unsigned i=0;i<raw.size();i++){
    const uint64_t offset=uint64_t(at)*4096+i;unsigned j=offset%576;
    const unsigned x=raw[i];
    const unsigned expected=native && offset<p.bytes && j>=32 && j<544 && x>=128?128+256-x:x;
    assert(expected<256 && out[i]==expected);
    changed+=out[i]!=raw[i];
    if(offset>=p.bytes)assert(!out[i]);
   }
   ++checked;
  }
 }
 assert(rawParts+q8Parts==3551 && q8Parts==1440 && tensors.size()==288
  && quants==UINT64_C(6734479360) && uploaded==UINT64_C(19399024640) && changed>0);
 const auto part=producer.Parts().at(firstNative);const auto& t=source.model.Tensors().at(part.tensor);
 auto bad=part;bad.role=0;assert(Q::Layout(bad,t)==Q::Format::Invalid);
 bad=part;bad.firstRow++;assert(Q::Layout(bad,t)==Q::Format::Invalid);
 bad=part;bad.kind=PIONEER_Q38_KV_STATE;assert(Q::Layout(bad,t)==Q::Format::Invalid);
 for(unsigned gate:{1U,2U,3U}){
  Page output;output.fill(0xcd);source.refuseAt=source.calls+gate;
  assert(!source.ReadWeightPage(producer,firstNative,0,output.data(),output.size()));
  for(auto v:output)assert(v==0xcd);
 }
 Page raw={},output;output.fill(0xab);
 raw[Codec::Header]=128;
 assert(!Q::Prepare(part,0,raw.data(),output.data(),output.size()));
 for(auto v:output)assert(v==0xab);
 assert(source.Ready());
 printf("PASS authenticated original model: %u Q8 fragments/%zu tensors, %u unchanged-format fragments, %u exact sampled pages, "
        "%llu changed signed bytes observed, %llu total upload bytes; no device\n",
        q8Parts,tensors.size(),rawParts,checked,(unsigned long long)changed,(unsigned long long)uploaded);
}
