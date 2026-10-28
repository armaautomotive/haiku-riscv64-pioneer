/* SPDX-License-Identifier: MIT */
// CPU-only TEST byte transport. Never compile into device/native/Work code.
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_BENCHMARK_HOST 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_whole_original_bytes.h"
#include "../PioneerQwen38PackedPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include <array>
#include <cassert>
#include <map>
#include <memory>
#include <unistd.h>
static const char* wholeOriginalPath=nullptr;
static FILE* WholeOriginalFopen(const char* path,const char* mode){
 assert(wholeOriginalPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));return fopen(wholeOriginalPath,mode);
}
static int WholeOriginalExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen WholeOriginalFopen
#define execv WholeOriginalExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
struct WholeModelOriginalBytes::Original {
 PioneerQ38PublicModelSource source;PioneerQwen38PackedPlan plan;
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::vector<PioneerQ38PackedPart> parts;
 std::map<uint64_t,unsigned> spans;
 std::map<uint64_t,std::array<uint8_t,4096>> cache; // <=128 RAW pages;never model products
 bool attempted=false,prepared=false,failed=false;
 static uint64_t Key(unsigned route,uint32_t address){return uint64_t(route)<<32|address;}
 bool Current()const{return prepared && !failed && source.Ready() && plan.MatchesSource(source.Model());}
 const PioneerQ38PackedPart* Find(unsigned route,uint32_t address,unsigned bytes)const{
  if(!prepared || !bytes || uint64_t(address)+bytes>UINT64_C(0x100000000))return nullptr;
  auto found=spans.upper_bound(Key(route,address));if(found==spans.begin())return nullptr;--found;
  const auto& p=parts.at(found->second);
  return PioneerQ38ProjectionRoute(p.bank)==route && address>=p.proposedAddress && address-p.proposedAddress<p.allocated
   && bytes<=p.allocated-(address-p.proposedAddress)?&p:nullptr;
 }
 bool Page(const PioneerQ38PackedPart& p,unsigned page,std::array<uint8_t,4096>& out){
  uint64_t key=Key(PioneerQ38ProjectionRoute(p.bank),p.proposedAddress+page*4096);
  auto found=cache.find(key);if(found!=cache.end()){out=found->second;return true;}
  unsigned index=unsigned(&p-parts.data());std::array<uint8_t,4096> stage;
  if(index>=parts.size() || p.kind!=PIONEER_Q38_WEIGHT || !source.ReadWeightPage(plan,index,page,stage.data(),4096)){
   failed=true;cache.clear();return false;
  }
  if(cache.size()>=128)cache.erase(cache.begin());cache.emplace(key,stage);out=stage;return true;
 }
};
WholeModelOriginalBytes::WholeModelOriginalBytes():original(new Original){}
WholeModelOriginalBytes::~WholeModelOriginalBytes(){delete original;}
bool WholeModelOriginalBytes::Ready()const{return original && original->Current();}
uint64_t WholeModelOriginalBytes::Epoch()const{return Ready()?original->source.Model().Epoch():0;}
bool WholeModelOriginalBytes::Describe(unsigned index,Q38OriginalTensor& output)const{
 if(!Ready() || index>=original->source.Model().Tensors().size())return false;
 const auto& t=original->source.Model().Tensors()[index];Q38OriginalTensor stage;
 stage.name=t.name;stage.type=t.type;stage.rank=t.rank;stage.inputs=t.inputs;stage.outputs=t.outputs;stage.bytes=t.bytes;stage.offset=t.offset;
 if(!Ready())return false;output=std::move(stage);return true;
}
bool WholeModelOriginalBytes::ReadOriginal(unsigned tensor,uint64_t at,unsigned bytes,void* output,unsigned capacity){
 uintptr_t out=reinterpret_cast<uintptr_t>(output);
 if(!Ready() || !output || !bytes || bytes>1048576 || capacity<bytes || out>UINTPTR_MAX-bytes || tensor>=851)return false;
 const auto& t=original->source.Model().Tensors()[tensor];if(at>t.bytes || bytes>t.bytes-at)return false;
 for(const auto& page:original->cache){uintptr_t p=reinterpret_cast<uintptr_t>(page.second.data());if(out<p+4096 && p<out+bytes)return false;}
 std::vector<uint8_t> stage(bytes);
 if(!original->source.ReadReferencePacked(tensor,at,bytes,stage.data(),bytes) || !Ready())return false;
 memcpy(output,stage.data(),bytes);return true;
}
void WholeModelOriginalBytes::Revoke(){if(original){original->failed=true;original->cache.clear();}}
bool WholeModelOriginalBytes::Open(const char* path){
 if(!original || original->attempted || !path || wholeOriginalPath)return false;
 original->attempted=true;wholeOriginalPath=path;auto& o=*original;
 if(!o.source.Open() || !o.source.Ready() || !o.plan.Build(o.source.Model(),8192,true,true) || !o.canonical->Build())return false;
 const auto& parts=o.plan.Parts();if(parts.size()!=o.canonical->Count() || o.source.Model().Tensors().size()!=851)return false;
 unsigned tensors=0;
 for(unsigned i=0;i<parts.size();i++){
  const auto& p=parts[i];const auto* c=o.canonical->Part(i);if(!c)return false;
  if(p.kind!=c->kind || p.layer!=c->layer || p.role!=c->role || p.bank!=c->bank || p.firstRow!=c->firstRow
   || p.rows!=c->rows || p.rowBytes!=c->rowBytes || p.proposedAddress!=o.canonical->Base+c->offset
   || p.bytes!=c->bytes || p.allocated!=c->allocated)return false;
  if(p.kind==PIONEER_Q38_WEIGHT){
   if(p.tensor>=851 || o.source.Model().Tensors()[p.tensor].type!=c->type)return false;
   if(!p.firstRow)tensors++;
   if(!o.spans.emplace(Original::Key(PioneerQ38ProjectionRoute(p.bank),p.proposedAddress),i).second)return false;
  }
 }
 if(tensors!=851)return false;
 o.parts=parts;o.prepared=true;return Ready();
}
bool WholeModelOriginalBytes::PrepareFork(){return Ready() && original->source.PrepareBenchmarkFork();}
bool WholeModelOriginalBytes::Contains(unsigned route,uint32_t address,unsigned bytes)const{return Ready() && original->Find(route,address,bytes);}
bool WholeModelOriginalBytes::Read(unsigned route,uint32_t address,uint8_t* out,unsigned bytes){
 uintptr_t output=reinterpret_cast<uintptr_t>(out);
 if(!Ready() || !out || !bytes || bytes>16384 || output>UINTPTR_MAX-bytes)return false;
 const auto* p=original->Find(route,address,bytes);if(!p)return false;
 for(const auto& page:original->cache){
  uintptr_t begin=reinterpret_cast<uintptr_t>(page.second.data());if(output<begin+4096 && begin<output+bytes)return false;
 }
 std::array<uint8_t,16384> stage;unsigned at=address-p->proposedAddress,done=0;
 while(done<bytes){
  std::array<uint8_t,4096> page;unsigned relative=at+done;
  if(!original->Page(*p,relative/4096,page))return false;
  unsigned n=std::min(bytes-done,4096-relative%4096);memcpy(stage.data()+done,page.data()+relative%4096,n);done+=n;
 }
 if(!Ready()){original->failed=true;original->cache.clear();return false;}
 memcpy(out,stage.data(),bytes);return true;
}
bool WholeModelOriginalBytes::CheckSamples(unsigned& partCount,unsigned& pageCount,uint64_t& padding,unsigned& mutableRefusals){
 if(!Ready())return false;
 unsigned checked=0,pages=0,refused=0;uint64_t zero=0;auto& o=*original;
 for(const auto& p:o.parts){
  if(p.kind!=PIONEER_Q38_WEIGHT){
   std::array<uint8_t,64> out;out.fill(0x5a);auto before=out;
   unsigned route=PioneerQ38ProjectionRoute(p.bank);
   if(Contains(route,p.proposedAddress,64) || Read(route,p.proposedAddress,out.data(),64) || out!=before)return false;
   refused++;continue;
  }
  const auto& t=o.source.Model().Tensors()[p.tensor];unsigned count=p.allocated/4096;
  std::vector<unsigned> selected={0};if(count>1)selected.push_back(count-1);if(count>2)selected.push_back(count/2);
  for(unsigned page:selected){
   std::array<uint8_t,4096> actual,reference={};
   if(!Read(PioneerQ38ProjectionRoute(p.bank),p.proposedAddress+page*4096,actual.data(),4096))return false;
   if(!t.type){
    uint64_t relative=uint64_t(page)*4096;unsigned n=unsigned(std::min(UINT64_C(4096),uint64_t(p.bytes)-relative));
    if(!o.source.ReadReferencePacked(p.tensor,uint64_t(p.firstRow)*p.rowBytes+relative,n,reference.data(),4096))return false;
    zero+=4096-n;
   }else{
    unsigned blockBytes=t.type==8?34:t.type==12?144:210,tile=t.type==8?576:t.type==12?2304:3392;
    unsigned blocks=unsigned(t.inputs)/(t.type==8?32:256),rowBytes=blocks*blockBytes,saved=~0U;
    std::vector<uint8_t> raw(size_t(rowBytes)*16);
    for(unsigned i=0;i<4096;i++){
     uint64_t offset=uint64_t(page)*4096+i;
     if(offset>=p.bytes){zero++;continue;}
     unsigned tileIndex=unsigned(offset/tile),within=unsigned(offset%tile),group=tileIndex/blocks;
     if(within>=blockBytes*16){zero++;continue;}
     if(saved!=group){
      if(!o.source.ReadReferencePacked(p.tensor,uint64_t(p.firstRow+group*16)*rowBytes,unsigned(raw.size()),raw.data(),unsigned(raw.size())))return false;saved=group;
     }
     reference[i]=raw[(within%16)*rowBytes+(tileIndex%blocks)*blockBytes+within/16];
    }
   }
   if(actual!=reference || !Ready())return false;pages++;
  }
  checked++;
 }
 if(!Ready() || !checked || !pages)return false;
 partCount=checked;pageCount=pages;padding=zero;mutableRefusals=refused;return true;
}
