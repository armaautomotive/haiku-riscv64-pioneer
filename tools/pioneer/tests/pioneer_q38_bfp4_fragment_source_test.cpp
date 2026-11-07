/* SPDX-License-Identifier: MIT */
// Offline conversion only; no disk cache, accelerator, model or Work mode.
#define PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC 1
#define PIONEER_NATIVE_WEIGHT_TILES_FIXTURE_ONLY
#include "pioneer_q38_native_weight_tiles_test.cpp"
#include "../PioneerQ38Bfp4FragmentSource.h"
#include "../PioneerQ38Bfp4FragmentCache.h"
#include <fcntl.h>
#include <unistd.h>
struct RowSource:Source {
 RowSource():Source(12,5120,64){
  PioneerQ38Tensor expected;assert(PioneerQ38Expected(0,2,expected));tensors[0].name=expected.name;
  tensors[0].outputs=expected.outputs;tensors[0].bytes=expected.bytes;
 }
 bool ReadRow(unsigned index,unsigned row,float* output,unsigned capacity){
  if(index || row>=64 || capacity<5120)return false;
  std::array<uint8_t,2880> packed={};if(!ReadPacked(index,uint64_t(row)*2880,2880,packed.data(),packed.size()))return false;
  for(unsigned block=0;block<20;block++)if(!PioneerDecodeQ4K(packed.data()+block*144,output+block*256))return false;
  return true;
 }
};
static PioneerQ38PackedPart Fragment(){return {PIONEER_Q38_WEIGHT,0,2,0,0,0,64,2880,0x04000000,184320,184320};}
int main(int argc,char** argv){
 assert(argc==2);
 RowSource source,reference;PioneerQ38Bfp4FragmentSource<RowSource> converter;auto part=Fragment();
 assert(converter.Bind(source,part) && converter.Ready() && converter.Pages()==45 && !converter.Bind(source,part));
 std::array<uint8_t,4096> actual={},expected={};
 for(unsigned page:{0U,1U,22U,23U,44U}){
  assert(converter.ReadPage(page,actual.data(),actual.size()));
  assert(PioneerQ38NativeWeightTiles::ReadRange(reference,0,uint64_t(page)*4096,4096,expected.data(),expected.size()));
  assert(actual==expected);
  if(page==0)assert(source.reads==32);if(page==1)assert(source.reads==32);if(page>=22)assert(source.reads==64);
 }
 const auto before=actual;const unsigned reads=source.reads;
 assert(!converter.ReadPage(45,actual.data(),actual.size()) && !converter.ReadPage(0,nullptr,4096)
  && !converter.ReadPage(0,actual.data(),4095) && actual==before && source.reads==reads);
 for(unsigned bad=0;bad<13;bad++){
  RowSource s;auto p=part;
  if(bad==0)p.kind=PIONEER_Q38_WORKSPACE;if(bad==1)p.layer=64;if(bad==2)p.role=1;if(bad==3)p.role=5;
  if(bad==4)p.firstRow=1;if(bad==5)p.rows=32;if(bad==6)p.rowBytes++;if(bad==7)p.bytes--;
  if(bad==8)p.allocated+=4096;if(bad==9)p.bank=7;if(bad==10)p.proposedAddress=0;if(bad==11)s.tensors[0].name="wrong";
  if(bad==12)s.tensors[0].outputs++;
  PioneerQ38Bfp4FragmentSource<RowSource> invalid;assert(!invalid.Bind(s,p) && !invalid.Ready() && !s.reads);
 }
 for(unsigned bad=0;bad<3;bad++){
  RowSource s;PioneerQ38Bfp4FragmentSource<RowSource> broken;assert(broken.Bind(s,part));
  if(bad==0)s.fail=5;if(bad==1)s.change=5;if(bad==2)s.tensors[0].name="changed";
  actual.fill(0xa5);const auto saved=actual;assert(!broken.ReadPage(0,actual.data(),actual.size()) && actual==saved && !broken.Ready());
  s.ready=true;s.fail=s.change=0;s.tensors[0].name="blk.0.ffn_gate.weight";s.epoch=1;
  const unsigned stopped=s.reads;assert(!broken.ReadPage(0,actual.data(),actual.size()) && s.reads==stopped);
 }
 {
  RowSource s;const auto sourceHash=PioneerModelWeightCache::Hash(s.raw.data(),s.raw.size());
  const auto fingerprint=PioneerModelWeightCache::Hash("implementation-a",16);
  const std::string directory=std::string(argv[1])+"/cache";std::string error;bool hit=true;
  PioneerQ38Bfp4FragmentCache<RowSource> cold;assert(cold.Prepare(s,part,sourceHash,fingerprint,directory.c_str(),hit,error) && !hit && s.reads==64);
  assert(cold.ReadPage(22,actual.data(),actual.size(),error));
  assert(PioneerQ38NativeWeightTiles::ReadRange(reference,0,uint64_t(22)*4096,4096,expected.data(),expected.size()) && actual==expected);
  const unsigned savedReads=s.reads;PioneerQ38Bfp4FragmentCache<RowSource> warm;
  assert(warm.Prepare(s,part,sourceHash,fingerprint,directory.c_str(),hit,error) && hit && s.reads==savedReads);
  assert(warm.ReadPage(22,actual.data(),actual.size(),error) && actual==expected && s.reads==savedReads);
  PioneerQ38Bfp4FragmentCache<RowSource> existing;
  assert(existing.OpenPrepared(s,part,sourceHash,fingerprint,directory.c_str(),error) && existing.Ready()
   && existing.ReadPage(22,actual.data(),actual.size(),error) && actual==expected && s.reads==savedReads);
  const auto missingRecipe=PioneerModelWeightCache::Hash("missing-recipe",14);
  PioneerQ38Bfp4FragmentCache<RowSource> missing;
  assert(!missing.OpenPrepared(s,part,sourceHash,missingRecipe,directory.c_str(),error) && !missing.Ready() && s.reads==savedReads);
  const std::string noDir=std::string(argv[1])+"/no-directory";PioneerQ38Bfp4FragmentCache<RowSource> absent;struct stat info={};
  assert(!absent.OpenPrepared(s,part,sourceHash,fingerprint,noDir.c_str(),error) && lstat(noDir.c_str(),&info) && errno==ENOENT);
  const auto other=PioneerModelWeightCache::Hash("implementation-b",16);PioneerQ38Bfp4FragmentCache<RowSource> different;
  assert(different.Prepare(s,part,sourceHash,other,directory.c_str(),hit,error) && !hit && s.reads==savedReads+64);
  PioneerQ38Bfp4FragmentCache<RowSource> corrupted;assert(corrupted.Prepare(s,part,sourceHash,fingerprint,directory.c_str(),hit,error) && hit);
  const auto saved=actual;s.epoch++;assert(!warm.ReadPage(0,actual.data(),actual.size(),error) && actual==saved && !warm.Ready());
  s.epoch--;assert(!warm.ReadPage(0,actual.data(),actual.size(),error) && actual==saved);
  PioneerModelWeightCache::Key key;key.sourceSHA256=sourceHash;
  const std::string recipe="pioneer-q38-ffn-bfp4b-v1/32x32-face-major/64-exponents+512-mantissas/Q4K-decode-then-BFP4B-RNE/no-cpu-products/source-code-sha256="+PioneerModelWeightCache::Hex(fingerprint);
  key.conversionSHA256=PioneerModelWeightCache::Hash(recipe.data(),recipe.size());key.entry=uint64_t(2)<<40|64;key.bytes=part.allocated;
  const auto file=directory+"/"+PioneerModelWeightCache::EntryName(key);assert(!chmod(file.c_str(),0600));
  int fd=open(file.c_str(),O_RDWR);assert(fd>=0);uint8_t value=0;const off_t payload=256+3*32;
  assert(pread(fd,&value,1,payload)==1);value^=1;assert(pwrite(fd,&value,1,payload)==1 && !close(fd));
  assert(!corrupted.ReadPage(0,actual.data(),actual.size(),error) && actual==saved && !corrupted.Ready());
  PioneerQ38Bfp4FragmentCache<RowSource> refuseCorrupt;const unsigned readsBefore=s.reads;
  assert(!refuseCorrupt.OpenPrepared(s,part,sourceHash,fingerprint,directory.c_str(),error)
   && !refuseCorrupt.Ready() && s.reads==readsBefore);
 }
 puts("PASS BFP4 FFN fragment conversion: 5 exact pages incl cross-band, cold/warm disk cache and recipe isolation, 13 geometry and 3 permanent-failure refusals; NO DEVICE OR WORK");
}
