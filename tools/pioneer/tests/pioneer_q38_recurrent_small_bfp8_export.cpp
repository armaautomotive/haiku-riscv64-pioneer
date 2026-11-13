/* SPDX-License-Identifier: MIT */
// Offline preparation/validation fixture, no installed cache or card access.
#define PIONEER_Q38_RECURRENT_SMALL_BFP8_PREPARER 1
#include "../PioneerQ38RecurrentSmallBfp8Selection.h"
#include "../PioneerQ38NativeBfpBandSource.h"
#define main independent_band_checker_main
#include "pioneer_dense_band_independent_check.cpp"
#undef main
#include <array>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc,char** argv){
 if(argc!=3 || argv[1][0]!='/' || argv[2][0]!='/')return 2;
 struct stat dir{};assert(!lstat(argv[2],&dir) && S_ISDIR(dir.st_mode) && dir.st_uid==geteuid() && (dir.st_mode&07777)==0700);
 std::unique_ptr<FILE,decltype(&fclose)> file(std::fopen(argv[1],"rb"),&fclose);assert(file);
 PioneerQwen38Gguf model;assert(model.Open(file.get()));const uint64_t epoch=model.Epoch();
 std::string error;PioneerModelWeightCache::Digest digest{};
 assert(PioneerModelWeightCache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,digest,error)
  && digest==PioneerQ38NativeBfpBandSource::OriginalPin() && model.Stable() && model.Epoch()==epoch);
 const uint32_t endian=1;assert(*reinterpret_cast<const uint8_t*>(&endian)==1);
 const unsigned width=5120,rows=48,tiles=160,bands=2;std::array<uint16_t,5120> input{};
 for(unsigned i=0;i<width;i++)input[i]=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));
 const char* names[]={"blk.0.ssm_alpha.weight","blk.0.ssm_beta.weight"};
 for(unsigned kind=0;kind<2;kind++){
  const auto* tensor=model.Find(names[kind]);assert(tensor && PioneerQ38RecurrentSmallBfp8Selection::Match(*tensor));
  const unsigned index=unsigned(tensor-model.Tensors().data());
  const std::string path=std::string(argv[2])+(kind?"/beta.bin":"/alpha.bin");
  int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);assert(fd>=0);
  FILE* out=fdopen(fd,"wb");assert(out);const uint32_t header[]={0x31525051,1,index,width,64,bands,tiles,1088};
  assert(fwrite(header,4,8,out)==8);double error2=0,norm2=0;
  for(unsigned band=0;band<bands;band++){
   std::array<uint32_t,32> result{};std::array<double,32> original{};
   for(unsigned t=0;t<tiles;t++){
    std::array<float,1024> values{};std::array<uint8_t,1088> packed{};
    assert(PioneerQ38NativeWeightTiles::ReadFloatTile(model,index,uint64_t(band)*tiles+t,values.data(),values.size()));
    assert(PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,values.data(),values.size(),packed.data(),packed.size()));
    assert(fwrite(packed.data(),1,packed.size(),out)==packed.size());
    std::vector<uint32_t> words(1104/4,0);std::memcpy(reinterpret_cast<uint8_t*>(words.data())+16,packed.data(),packed.size());
    for(unsigned row=0;row<32;row++){
     for(unsigned c=0;c<32;c++){uint32_t bits=uint32_t(input[t*32+c])<<16;float x;std::memcpy(&x,&bits,4);original[row]+=double(values[row*32+c])*x;}
     for(unsigned half=0;half<2;half++){
      uint16_t a[16],b[16];for(unsigned c=0;c<16;c++){a[c]=Weight(words,0,row,half*16+c);b[c]=input[t*32+half*16+c];}
      for(unsigned phase=0;phase<4;phase++)result[row]=Instruction(a,b,phase,result[row]);
     }
    }
   }
   for(unsigned row=0;row<32;row++){
    float value;std::memcpy(&value,&result[row],4);assert(std::isfinite(value));
    if(band*32+row>=rows){assert(!result[row] && original[row]==0);continue;}
    const double delta=double(value)-original[row];error2+=delta*delta;norm2+=original[row]*original[row];
   }
   assert(fwrite(result.data(),4,result.size(),out)==result.size());
  }
  assert(norm2>0 && model.Stable() && model.Epoch()==epoch && !fflush(out) && !fsync(fd) && !fclose(out));
  std::printf("SMALL_BFP8 tensor=%s real_rows=48 padded_rows=16 relative_l2=%.9g; ONE_SYNTHETIC_INPUT\n",names[kind],std::sqrt(error2/norm2));
 }
 std::puts("PASS authenticated alpha/beta BFP8 packing and independent integer reference; NO DEVICE OR SPEED");
}
