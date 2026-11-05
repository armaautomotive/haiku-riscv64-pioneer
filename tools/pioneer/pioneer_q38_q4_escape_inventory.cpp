/* SPDX-License-Identifier: MIT */
// READ-ONLY aggregate original-quant inventory. No prepared weights are
// written or printed,no device/driver/NoC/inference/reference is invoked.
#include "PioneerQ38Q4EscapeTiles.h"
#include "PioneerQwen38PackedPlan.h"
#include "PioneerModelWeightCache.h"
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <memory>
#ifdef NDEBUG
#error Inventory checks must not be compiled out
#endif
using E=PioneerQ38Q4EscapeTiles;
static bool Same(const struct stat& a,const struct stat& b) {
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int main(int argc,char** argv) {
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);
 FILE* file=fdopen(fd,"rb");assert(file);struct stat before{},after{};
 assert(!fstat(fd,&before) && S_ISREG(before.st_mode) && uint64_t(before.st_size)==PioneerQwen38Gguf::FullBytes);
 PioneerModelWeightCache::Digest digest;std::string error;
 printf("ORIGINAL_Q4_INVENTORY_AUTH_START_READ_ONLY_NO_DEVICE\n");
 assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error));
 assert(PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 assert(!fstat(fd,&after) && Same(before,after));
 auto model=std::make_unique<PioneerQwen38Gguf>();assert(model->Open(file) && model->Tensors().size()==851);
 auto plan=std::make_unique<PioneerQwen38PackedPlan>();assert(plan->Build(*model,8192,true,true));
 const auto& parts=plan->Parts(); // stable metadata snapshot; source rechecked at closure
 uint64_t allocated=0,q4Bytes=0,expectedBlocks=0;
 for(const auto& part:parts) {
  allocated+=part.allocated;
  if(part.kind==PIONEER_Q38_WEIGHT && model->Tensors()[part.tensor].type==12) {
   assert(part.bytes%2304==0);q4Bytes+=part.bytes;expectedBlocks+=part.bytes/2304;
  }
 }
 assert(expectedBlocks==4488320);
 uint64_t observed=0,payload=0,alignedPayload=0,escapes=0,histogram[257]={};unsigned tensors=0,sampled=0;
 // Append-only OFFLINE geometry: retain EVERY original primary/state/workspace
 // extent. Each Q4 fragment gets its own 16-aligned index and mask records,
 // rounded up to a 4KiB allocation in that fragment's EXISTING bank. This
 // does not allocate/reserve/authorize those ranges on a device.
 std::vector<uint64_t> partBlocks(parts.size()),partPayload(parts.size());
 for(unsigned index=0;index<model->Tensors().size();index++) {
  const auto& tensor=model->Tensors()[index];if(tensor.type!=12)continue;
  assert(tensor.rank==2 && !(tensor.inputs%256) && !(tensor.outputs%16));
  const unsigned blocks=unsigned(tensor.inputs/256),rowBytes=blocks*144,chunkBytes=16*rowBytes;
  assert(chunkBytes<=PioneerQwen38Gguf::MaxPackedRead);std::vector<uint8_t> rows(chunkBytes);
  for(unsigned first=0;first<tensor.outputs;first+=16) {
   unsigned fragment=unsigned(parts.size());
   for(unsigned p=0;p<parts.size();p++) {
    const auto& part=parts[p];
    if(part.kind==PIONEER_Q38_WEIGHT && part.tensor==index && first>=part.firstRow && first-part.firstRow<part.rows) {
     assert(fragment==parts.size() && !(part.firstRow%16) && !(part.rows%16));fragment=p;
    }
   }
   assert(fragment<parts.size());
   const auto& part=parts[fragment];
   assert(model->ReadPacked(index,uint64_t(first)*rowBytes,chunkBytes,rows.data(),chunkBytes));
   for(unsigned b=0;b<blocks;b++) {
    const uint64_t tile=uint64_t(first-part.firstRow)/16*blocks+b,tiles=part.bytes/2304;
    const bool sample=tile==0 || tile==tiles/2 || tile==tiles-1;
    std::array<uint8_t,512> expectedMask{};
    unsigned nonzero=0;
    for(unsigned k=0;k<256;k++) {
     unsigned mask=0;
     for(unsigned n=0;n<16;n++) {
      unsigned code=rows[(n*blocks+b)*144+16+(k/64)*32+k%32];
      const unsigned q=(code>>(k%64>=32?4:0))&15;mask|=unsigned(q==15)<<n;
     }
     nonzero+=bool(mask);escapes+=unsigned(__builtin_popcount(mask));
      if(sample){expectedMask[2*k]=uint8_t(mask);expectedMask[2*k+1]=uint8_t(mask>>8);}
    }
    const unsigned bytes=E::MaskPayloadBytes(nonzero);assert(bytes<=512);
    histogram[nonzero]++;payload+=bytes;alignedPayload+=(bytes+3)&~3U;observed++;
    partBlocks[fragment]++;partPayload[fragment]+=(bytes+15)&~15U;
    if(sample) {
     // Independent loader relation: reconstruct from GGUF row-major bytes,
     // compare the ACTUAL existing tiled page reader (including crossings),
     // then lossless encode/decode and sparse mask round-trip. No learned
     // bytes leave these bounded temporary ordinary host arrays.
     std::array<uint8_t,2304> expected{},tiled{},decoded{};
     for(unsigned byte=0;byte<144;byte++)for(unsigned n=0;n<16;n++)
      expected[byte*16+n]=rows[(n*blocks+b)*144+byte];
     unsigned copied=0;std::array<uint8_t,4096> page{};
     while(copied<2304) {
      const uint64_t at=tile*2304+copied;const unsigned within=unsigned(at%4096),take=std::min(2304-copied,4096-within);
      assert(plan->ReadWeightPage(*model,fragment,unsigned(at/4096),page.data(),page.size()));
      memcpy(tiled.data()+copied,page.data()+within,take);copied+=take;
     }
     assert(expected==tiled);std::array<uint8_t,2816> encoded{};std::array<uint8_t,512> packedMask{},restoredMask{};E::Mask description;
     assert(E::Encode(tiled.data(),2304,encoded.data(),2816) && !memcmp(encoded.data(),tiled.data(),256));
     assert(E::Decode(encoded.data(),2816,decoded.data(),2304) && decoded==tiled);
     assert(!memcmp(expectedMask.data(),encoded.data()+2304,512) && E::NonzeroRows(expectedMask.data())==nonzero);
     assert(E::PackMask(expectedMask.data(),512,packedMask.data(),512,description) && description.bytes==bytes);
     assert(E::UnpackMask(packedMask.data(),bytes,description,restoredMask.data(),512) && restoredMask==expectedMask);sampled++;
    }
   }
  }
  tensors++;printf("ORIGINAL_Q4_INVENTORY_TENSOR%u_BLOCKS%llu;AGGREGATES_ONLY\n",tensors,(unsigned long long)observed);
 }
 assert(observed==expectedBlocks && tensors==193 && model->Stable() && plan->Ready());
 assert(!fstat(fd,&after) && Same(before,after));
 const uint64_t capacity=7*PioneerQwen38PackedPlan::Capacity,denseExtra=observed*512,indexBytes=observed*4;
 printf("CLOSED_ORIGINAL_Q4_INVENTORY SHA256%s tensors%u blocks%llu values%llu escapes%llu compressedPayload%llu alignedPayload%llu index4Bytes%llu\n",
  PioneerModelWeightCache::Hex(digest).c_str(),tensors,(unsigned long long)observed,(unsigned long long)(observed*4096),
  (unsigned long long)escapes,(unsigned long long)payload,(unsigned long long)alignedPayload,(unsigned long long)indexBytes);
 printf("FULL8192_TWO_STATE_ALL851_GEOMETRY allocated%llu Q4bytes%llu capacity%llu denseExtra%llu denseOverBy%lld compressedPlusIndex%llu aggregateRemaining%lld;"
  "DATA_ONLY_NOT_BANK_ALLOCATION_OR_CAPACITY_OR_SPEED_PROOF\n",(unsigned long long)allocated,(unsigned long long)q4Bytes,(unsigned long long)capacity,
  (unsigned long long)denseExtra,(long long)(allocated+denseExtra)-(long long)capacity,(unsigned long long)(alignedPayload+indexBytes),
  (long long)capacity-(long long)(allocated+alignedPayload+indexBytes));
 for(unsigned n=0;n<=256;n++)if(histogram[n])printf("Q4_ESCAPE_NONZERO_ROWS%u_BLOCKS%llu\n",n,(unsigned long long)histogram[n]);
 uint64_t bankExtra[7]={},appendTotal=0,checkedBlocks=0;bool allFit=true;unsigned fragments=0;
 for(unsigned p=0;p<parts.size();p++)if(partBlocks[p]) {
  const auto& part=parts[p];
  assert(part.kind==PIONEER_Q38_WEIGHT && model->Tensors()[part.tensor].type==12
   && partBlocks[p]==part.bytes/2304 && part.bank<7);
  const uint64_t bytes=((partBlocks[p]*4+15)&~UINT64_C(15))+partPayload[p];
  assert(bytes<(UINT64_C(1)<<30)); // future per-fragment 30-bit offset plus kind
  bankExtra[part.bank]+=(bytes+4095)&~UINT64_C(4095);checkedBlocks+=partBlocks[p];fragments++;
 }
 assert(checkedBlocks==observed);
 for(unsigned bank=0;bank<7;bank++) {
  const uint64_t total=plan->Used(bank)+bankExtra[bank];const bool fit=total<=PioneerQwen38PackedPlan::Capacity;
  appendTotal+=bankExtra[bank];allFit=allFit && fit;
  printf("OFFLINE_APPEND_BANK%u original%llu added%llu total%llu remaining%lld fits%u;NO_NATIVE_ALLOCATION_AUTHORITY\n",bank,
   (unsigned long long)plan->Used(bank),(unsigned long long)bankExtra[bank],(unsigned long long)total,
   (long long)PioneerQwen38PackedPlan::Capacity-(long long)total,unsigned(fit));
 }
 assert(sampled==fragments*3);
 printf("CLOSED_APPEND_GEOMETRY full8192twoStateAll851 fragments%u added%llu allSevenBanksFit%u sampledActualLoaderTiles%u;"
  "NOT_NATIVE_OWNERSHIP_OR_KERNEL_OR_QUALITY_OR_SPEED_PROOF\n",fragments,(unsigned long long)appendTotal,unsigned(allFit),sampled);
 assert(!fclose(file));
}
