/* SPDX-License-Identifier: MIT */
// Synthetic derived-head cache only; no actual model, device, inference or Work.
#define PIONEER_HEAD_FIXTURE_ONLY
#include "pioneer_q38_head_block_float_test.cpp"
#include "../PioneerQ38HeadBfp8FragmentCache.h"

int main(int argc,char** argv){
 assert(argc==2 && argv[1][0]=='/');
 HeadSource source,reference;PioneerQ38PackedPart part={PIONEER_Q38_WEIGHT,64,1,0,0,0,64,4240,0x04000000,271360,274432};
 const auto sourceHash=PioneerModelWeightCache::Hash(source.raw.data(),source.raw.size());
 const auto fingerprint=PioneerModelWeightCache::Hash("head-implementation-a",21);
 const std::string directory=std::string(argv[1])+"/head-cache";std::string error;bool hit=true;
 PioneerQ38HeadBfp8FragmentCache<HeadSource> cold;
 assert(cold.Prepare(source,part,sourceHash,fingerprint,directory.c_str(),hit,error) && !hit && source.reads==64);
 PioneerQ38HeadBfp8FragmentSource<HeadSource> direct;assert(direct.Bind(reference,part));
 std::array<uint8_t,4096> actual={},expected={};
 for(unsigned page:{0U,42U,84U}){
  assert(cold.ReadPage(page,actual.data(),actual.size(),error) && direct.ReadPage(page,expected.data(),expected.size()) && actual==expected);
 }
 const unsigned reads=source.reads;
 PioneerQ38HeadBfp8FragmentCache<HeadSource> warm;
 assert(warm.Prepare(source,part,sourceHash,fingerprint,directory.c_str(),hit,error) && hit && source.reads==reads);
 PioneerQ38HeadBfp8FragmentCache<HeadSource> existing;
 assert(existing.OpenPrepared(source,part,sourceHash,fingerprint,directory.c_str(),error) && existing.Ready());
 assert(existing.ReadPage(42,actual.data(),actual.size(),error) && direct.ReadPage(42,expected.data(),expected.size()) && actual==expected && source.reads==reads);
 const auto other=PioneerModelWeightCache::Hash("head-implementation-b",21);PioneerQ38HeadBfp8FragmentCache<HeadSource> missing;
 assert(!missing.OpenPrepared(source,part,sourceHash,other,directory.c_str(),error) && !missing.Ready() && source.reads==reads);
 PioneerQ38HeadBfp8FragmentCache<HeadSource> newRecipe;
 assert(newRecipe.Prepare(source,part,sourceHash,other,directory.c_str(),hit,error) && !hit && source.reads==reads+64);
 PioneerQ38HeadBfp8FragmentCache<HeadSource> changed;assert(changed.OpenPrepared(source,part,sourceHash,fingerprint,directory.c_str(),error));
 const auto saved=actual;source.epoch++;
 assert(!changed.ReadPage(0,actual.data(),actual.size(),error) && actual==saved && !changed.Ready());
 source.epoch--;assert(!changed.ReadPage(0,actual.data(),actual.size(),error) && actual==saved);
 PioneerModelWeightCache::Key key;key.sourceSHA256=sourceHash;
 const std::string recipe="pioneer-q38-output-head-bfp8b-v1/32x32-face-major/64-exponents+1024-mantissas/Q6K-decode-then-BFP8B-RNE/no-cpu-products/source-code-sha256="+PioneerModelWeightCache::Hex(fingerprint);
 key.conversionSHA256=PioneerModelWeightCache::Hash(recipe.data(),recipe.size());key.entry=UINT64_C(64)<<48|UINT64_C(1)<<40|64;key.bytes=348160;
 const auto file=directory+"/"+PioneerModelWeightCache::EntryName(key);assert(!chmod(file.c_str(),0600));
 int fd=open(file.c_str(),O_RDWR);assert(fd>=0);uint8_t value=0;const off_t payload=256+3*32;
 assert(pread(fd,&value,1,payload)==1);value^=1;assert(pwrite(fd,&value,1,payload)==1 && !close(fd));
 PioneerQ38HeadBfp8FragmentCache<HeadSource> corrupt;const unsigned stopped=source.reads;
 assert(!corrupt.OpenPrepared(source,part,sourceHash,fingerprint,directory.c_str(),error) && !corrupt.Ready() && source.reads==stopped);
 puts("PASS derived head BFP8 cache cold/warm/existing, exact pages, source/recipe isolation, changed-source and corrupt-cache refusal; synthetic data, NO model/device/TPS");
}
