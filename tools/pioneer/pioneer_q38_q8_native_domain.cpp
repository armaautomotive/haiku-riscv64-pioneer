/* SPDX-License-Identifier: MIT */
// Read-only feasibility check for lossless Q8_0 to native signed magnitude.
// No conversion, weight export, cache creation, inference or device access.
#include "PioneerModelWeightCache.h"
#include "PioneerQwen38Gguf.h"
#include <array>
#include <fcntl.h>
#include <memory>
#include <unistd.h>

namespace {
constexpr unsigned Width=32, Block=34, Chunk=PioneerQwen38Gguf::MaxPackedRead/Block*Block;
constexpr const char* Pin="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747";

struct Domain {
 uint64_t blocks=0,badScales=0,minus128=0,exceptionBlocks=0;
 bool Scan(const uint8_t* data,size_t bytes){
  if(!data || !bytes || bytes%Block)return false;
  for(size_t at=0;at<bytes;at+=Block){
   // Only bytes 2..33 are quantized values. HALF metadata can contain 0x80.
   badScales+=(data[at+1]&0x7c)==0x7c;
   const auto* first=data+at+2;
   const unsigned n=std::count(first,first+Width,uint8_t(0x80));
   minus128+=n;exceptionBlocks+=n!=0;blocks++;
  }
  return true;
 }
};

int Unit(){
 std::array<uint8_t,Block*3> data={};
 for(unsigned b=0;b<3;b++)data[b*Block]=0x80; // not a quant
 data[1]=0x80; // signed, finite scale; not a quant either
 data[Block+1]=0xfc; // non-finite scale
 data[Block+2]=data[Block+33]=0x80; // signed -128 in actual lanes
 data[2*Block+11]=0x80;data[2*Block+32]=0xff; // -1 is representable
 Domain d;
 if(d.Scan(nullptr,34) || d.Scan(data.data(),0) || d.Scan(data.data(),data.size()-1) || d.blocks
  || !d.Scan(data.data(),Block) || d.minus128 || d.badScales
  || !d.Scan(data.data()+Block,Block*2) || d.blocks!=3 || d.minus128!=3 || d.badScales!=1 || d.exceptionBlocks!=2)return 1;
 puts("PASS Q8_0 metadata/lane distinction, two distinct exceptions and truncated-range refusals");
 return 0;
}

int Fail(const char* message){fprintf(stderr,"Q8_NATIVE_DOMAIN_REFUSED %s\n",message);return 1;}
bool Same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_ctime!=b.st_ctime || a.st_mtime!=b.st_mtime
  || a.st_mode!=b.st_mode || a.st_uid!=b.st_uid || a.st_nlink!=b.st_nlink || a.st_size!=b.st_size)return false;
#if defined(__APPLE__)
 return a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec && a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec;
#else
 return a.st_ctim.tv_nsec==b.st_ctim.tv_nsec && a.st_mtim.tv_nsec==b.st_mtim.tv_nsec;
#endif
}
}

int main(int argc,char** argv){
 if(argc==2 && !strcmp(argv[1],"--self-test"))return Unit();
 if(argc!=2 || argv[1][0]!='/')return 2;
 const int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
 if(fd<0)return Fail("read-only open");
 struct stat before{};
 if(fstat(fd,&before) || !S_ISREG(before.st_mode) || uint64_t(before.st_size)!=PioneerQwen38Gguf::FullBytes){
  close(fd);return Fail("original file identity");
 }
 FILE* stream=fdopen(fd,"rb");
 if(!stream){close(fd);return Fail("open model stream");}
 const std::unique_ptr<FILE,decltype(&fclose)> file(stream,&fclose);
 PioneerQwen38Gguf model;
 if(!model.Open(stream))return Fail("fixed 851-tensor layout");
 using Hash=PioneerModelWeightCache;
 Hash::Digest sha={};std::string error;
 if(!Hash::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error) || Hash::Hex(sha)!=Pin || !model.Stable())return Fail("exact source SHA-256");
 std::vector<uint8_t> buffer(Chunk);
 unsigned count=0,exceptions=0,invalid=0;
 uint64_t bytes=0,blocks=0,minus128=0,exceptionBlocks=0,badScales=0;
 for(unsigned i=0;i<model.Tensors().size();i++){
  const auto& tensor=model.Tensors()[i];if(tensor.type!=8)continue;
  if(tensor.rank!=2 || tensor.inputs%Width || tensor.bytes%Block || tensor.bytes/Block!=tensor.inputs/Width*tensor.outputs)
   return Fail("Q8_0 extent");
  Domain found;
  for(uint64_t at=0;at<tensor.bytes;){
   const unsigned take=unsigned(std::min(uint64_t(Chunk),tensor.bytes-at));
   if(!model.ReadPacked(i,at,take,buffer.data(),Chunk) || !found.Scan(buffer.data(),take))return Fail("packed extent read");
   at+=take;
  }
  if(found.blocks!=tensor.bytes/Block)return Fail("incomplete tensor coverage");
  count++;exceptions+=found.minus128!=0;invalid+=found.badScales!=0;
  bytes+=tensor.bytes;blocks+=found.blocks;minus128+=found.minus128;exceptionBlocks+=found.exceptionBlocks;badScales+=found.badScales;
 }
 struct stat after{};
 if(count!=288 || bytes!=7155384320ULL || blocks!=bytes/Block || !model.Stable() || fstat(fd,&after) || !Same(before,after))
  return Fail("whole source coverage and stability");
 printf("Q8_NATIVE_DOMAIN original_sha256=%s tensors=%u packed_bytes=%llu blocks=%llu quants=%llu\n",
  Pin,count,(unsigned long long)bytes,(unsigned long long)blocks,(unsigned long long)(blocks*Width));
 printf("Q8_NATIVE_DOMAIN minus128_values=%llu exception_blocks=%llu exception_tensors=%u "
  "nonfinite_scales=%llu nonfinite_tensors=%u signed_magnitude_domain=%s;DATA_ONLY_NO_INFERENCE_OR_TIMING\n",
  (unsigned long long)minus128,(unsigned long long)exceptionBlocks,exceptions,(unsigned long long)badScales,invalid,
  minus128 || badScales?"REFUSED":"EXACT_DOMAIN_PRESENT");
}
