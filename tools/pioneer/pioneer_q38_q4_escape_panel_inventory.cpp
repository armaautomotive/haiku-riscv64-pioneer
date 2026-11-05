/* SPDX-License-Identifier: MIT */
// O_RDONLY same-open original GGUF;aggregates only. No cache/device access.
#include "PioneerQ38Q4EscapePanelScan.h"
#include "PioneerQ38Q4EscapeTiles.h"
#include "PioneerQwen38PackedPlan.h"
#include "PioneerModelWeightCache.h"
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Assertions required
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
struct Counts{
 uint64_t tiles=0,empty=0,inspected=0,histogram[17]={};
 void Add(unsigned mask,unsigned steps){tiles++;const unsigned zero=16-unsigned(__builtin_popcount(mask));empty+=zero;inspected+=steps;histogram[zero]++;}
 void Print(const char* label)const{
  printf("ESCAPE_PANEL_DATA %s tiles%llu panels%llu emptyPanels%llu emptyPercent%.9f examinedOriginalCodes%llu NOT_COMPUTE_WEIGHTED_OR_TPS\n",
   label,(unsigned long long)tiles,(unsigned long long)(tiles*16),(unsigned long long)empty,tiles?100.0*double(empty)/double(tiles*16):0.,(unsigned long long)inspected);
  for(unsigned i=0;i<=16;i++)if(histogram[i])printf("ESCAPE_PANEL_HIST %s emptyPanelsPerTile%u tiles%llu\n",label,i,(unsigned long long)histogram[i]);
 }
};
static void Sample(const PioneerQwen38PackedPlan& plan,PioneerQwen38Gguf& model,unsigned fragment,unsigned tile,
 const uint8_t* rows,unsigned blocks,unsigned block,unsigned mask){
 std::array<uint8_t,2304> expected{},actual{};std::array<uint8_t,4096> page{};std::array<uint8_t,2816> encoded{};
 for(unsigned b=0;b<144;b++)for(unsigned n=0;n<16;n++)expected[b*16+n]=rows[(n*blocks+block)*144+b];
 for(unsigned copied=0;copied<2304;){
  const unsigned at=tile*2304+copied,within=at%4096,take=std::min(2304-copied,4096-within);
  assert(plan.ReadWeightPage(model,fragment,at/4096,page.data(),unsigned(page.size())));
  memcpy(actual.data()+copied,page.data()+within,take);copied+=take;
 }
 assert(actual==expected && PioneerQ38Q4EscapeTiles::Encode(actual.data(),unsigned(actual.size()),encoded.data(),unsigned(encoded.size())));
 unsigned native=0;for(unsigned p=0;p<16;p++)for(unsigned b=0;b<32;b++)if(encoded[2304+p*32+b])native|=1U<<p;
 assert(native==mask);
}
int main(int argc,char** argv){
 assert(argc==2 && argv[1][0]=='/');setvbuf(stdout,nullptr,_IOLBF,0);
 const int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);FILE* file=fdopen(fd,"rb");assert(file);
 struct stat before{},after{};assert(!fstat(fd,&before) && S_ISREG(before.st_mode));std::string error;Cache::Digest digest;
 puts("ESCAPE_PANEL_AUTH_SAME_OPEN_ORIGINAL_READ_ONLY_NO_CACHE_OR_DEVICE");
 assert(Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  && Cache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 auto model=std::make_unique<PioneerQwen38Gguf>();assert(model->Open(file) && model->Tensors().size()==851);
 auto plan=std::make_unique<PioneerQwen38PackedPlan>();assert(plan->Build(*model,8192,true,true));
 const auto& parts=plan->Parts();const auto& tensors=model->Tensors();Counts all,matrix,embedding,k5120,k17408;
 unsigned fragments=0,samples=0;std::set<unsigned> seenTensors;
 for(unsigned fragment=0;fragment<parts.size();fragment++){
  const auto& part=parts[fragment];if(part.kind!=PIONEER_Q38_WEIGHT || tensors[part.tensor].type!=12)continue;
  const auto& tensor=tensors[part.tensor];const unsigned blocks=unsigned(tensor.inputs/256),rowBytes=blocks*144,bytes=16*rowBytes;
  assert(tensor.rank==2 && !(tensor.inputs%256) && !(part.firstRow%16) && !(part.rows%16) && part.bytes%2304==0 && bytes<=PioneerQwen38Gguf::MaxPackedRead);
  const bool emb=tensor.name=="token_embd.weight";
  const auto ends=[&](const std::string& tail){return tensor.name.size()>=tail.size() && tensor.name.compare(tensor.name.size()-tail.size(),tail.size(),tail)==0;};
  assert(emb || ends("ffn_gate.weight") || ends("ffn_up.weight") || ends("ffn_down.weight"));
  assert(blocks==20 || blocks==68);std::vector<uint8_t> rows(bytes);const unsigned tiles=part.bytes/2304;
  for(unsigned first=part.firstRow;first<part.firstRow+part.rows;first+=16){
   assert(model->ReadPacked(part.tensor,uint64_t(first)*rowBytes,bytes,rows.data(),bytes));
   for(unsigned b=0;b<blocks;b++){
    unsigned mask=0,steps=0;assert(PioneerQ38Q4EscapePanelScan::Mask(rows.data(),bytes,blocks,b,mask,steps));
    all.Add(mask,steps);(emb?embedding:matrix).Add(mask,steps);if(!emb)(blocks==20?k5120:k17408).Add(mask,steps);
    const unsigned tile=(first-part.firstRow)/16*blocks+b;
    if(tile==0 || tile==tiles/2 || tile==tiles-1){Sample(*plan,*model,fragment,tile,rows.data(),blocks,b,mask);samples++;}
   }
  }
  fragments++;seenTensors.insert(part.tensor);
  printf("ESCAPE_PANEL_FRAGMENT%u completeTiles%llu;AGGREGATES_ONLY\n",fragments,(unsigned long long)all.tiles);
 }
 assert(all.tiles==4488320 && matrix.tiles==4177920 && embedding.tiles==310400 && fragments==1351 && samples==4053 && seenTensors.size()==193);
 assert(model->Stable() && plan->Ready() && !fstat(fd,&after) && Same(before,after));
 all.Print("ALL_Q4");matrix.Print("FFN_MATRIX_Q4");embedding.Print("EMBEDDING_Q4");k5120.Print("FFN_K5120");k17408.Print("FFN_K17408");
 assert(!fclose(file));
 printf("CLOSED_ESCAPE_PANEL_INVENTORY all851_full8192twoState_tensors193_fragments1351_tiles4488320_panels71813120 sampledActualTiledNativeCodec4053 sameOpenOriginalSHA_noCacheAccess_orWeightMutation_orModelInference_orGPU_orTPS\n");
}
