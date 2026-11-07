/* SPDX-License-Identifier: MIT */
// Read-only real-model/cache diagnostic. No device, upload or inference.
#include "../PioneerQ38NativeBfpBandSource.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
int main(int argc,char** argv){
 assert(argc==3 && argv[1][0]=='/' && argv[2][0]=='/');
 std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[1],"rb"),&fclose);assert(file);
 PioneerQwen38Gguf model;assert(model.Open(file.get()));const uint64_t epoch=model.Epoch();
 PioneerModelWeightCache::Digest hash={};std::string error;
 puts("BFP8_STRIPE authenticating original model; CPU_ONLY");fflush(stdout);
 assert(PioneerModelWeightCache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,hash,error)
  && hash==PioneerQ38NativeBfpBandSource::OriginalPin() && model.Stable() && model.Epoch()==epoch);
 struct stat directory={};assert(!lstat(argv[2],&directory) && S_ISDIR(directory.st_mode)
  && directory.st_uid==geteuid() && (directory.st_mode&07777)==0700);
 PioneerModelWeightCache cache;assert(cache.Open(argv[2],error));PioneerQ38NativeBfpBandSource source(cache);
 assert(source.Bind(model,hash,[&]{return model.Stable() && model.Epoch()==epoch;},error));
 const auto* t=model.Find("blk.0.ffn_gate.weight");assert(t && t->type==12 && t->inputs==5120 && t->outputs==17408);
 const unsigned tensor=unsigned(t-model.Tensors().data()),bandBytes=160*1088,pages=(bandBytes+4095)/4096;unsigned checked=0;
 for(unsigned first:{0U,8704U,17376U})for(unsigned page:{0U,pages/2,pages-1}){
  std::array<uint8_t,4096> cached{},fresh{};
  assert(source.ReadStripePage(tensor,first,32,page,cached.data(),cached.size(),error));
  const uint64_t at=uint64_t(page)*4096;const unsigned take=unsigned(std::min(uint64_t(4096),uint64_t(bandBytes)-at));
  const uint64_t nativeOffset=uint64_t(first/32)*bandBytes+at;
  assert(PioneerQ38NativeBfp8WeightTiles::ReadRange(model,tensor,nativeOffset,take,fresh.data(),fresh.size()));
  assert(cached==fresh && source.Ready());checked++;
 }
 assert(checked==9 && source.Ready() && model.Stable());
 puts("PASS_BFP8_STRIPE_REAL first_middle_last_bands pages9 fresh_original_exact zero_padding source_cache_read_only; NO_DEVICE_NO_SPEED_CLAIM");
 return fflush(stdout)?1:0;
}
