/* SPDX-License-Identifier: MIT */
// Read-only full-model authentication and private projection data export.
// CPU-only validation fixture; never opens a card or writes the cache.
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

static void Need(bool value,const char* message){if(!value){std::fprintf(stderr,"EXPORT_REFUSED %s\n",message);std::exit(1);}}
static void Write(FILE* f,const void* data,size_t bytes){Need(std::fwrite(data,1,bytes,f)==bytes,"private output write");}
int main(int argc,char** argv){
 if(argc!=5 || (std::strcmp(argv[4],"--gate") && std::strcmp(argv[4],"--qkv") && std::strcmp(argv[4],"--out")))return 2;
 const bool gate=!std::strcmp(argv[4],"--gate"),qkv=!std::strcmp(argv[4],"--qkv");
 const unsigned index=gate?3:qkv?5:16,width=gate||qkv?5120:6144,rows=gate?6144:qkv?10240:5120;
 const unsigned bands=rows/32,tiles=width/32;const char* name=gate?"blk.0.attn_gate.weight":qkv?"blk.0.attn_qkv.weight":"blk.0.ssm_out.weight";
 const uint32_t endian=1;Need(*reinterpret_cast<const uint8_t*>(&endian)==1,"little-endian fixture");struct stat dir{};
 Need(!lstat(argv[3],&dir) && S_ISDIR(dir.st_mode) && dir.st_uid==geteuid() && (dir.st_mode&07777)==0700,"private output directory");
 std::unique_ptr<FILE,decltype(&fclose)> file(std::fopen(argv[1],"rb"),&fclose);Need(bool(file),"original open");
 PioneerQwen38Gguf model;Need(model.Open(file.get()),"original layout");const uint64_t epoch=model.Epoch();
 std::string error;PioneerModelWeightCache::Digest digest{};
 std::puts("AUTHENTICATING_ORIGINAL_FOR_FULL_PROJECTION;NO DEVICE");std::fflush(stdout);
 Need(PioneerModelWeightCache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,digest,error)
  && digest==PioneerQ38NativeBfpBandSource::OriginalPin() && model.Stable() && model.Epoch()==epoch,"original identity");
 Need(!lstat(argv[2],&dir) && S_ISDIR(dir.st_mode) && dir.st_uid==geteuid() && (dir.st_mode&07777)==0700,"cache directory");
 PioneerModelWeightCache cache;Need(cache.OpenExistingDirectory(argv[2],error),"existing cache");
 PioneerQ38NativeBfpBandSource source(cache);Need(source.Bind(model,digest,[&]{return model.Stable() && model.Epoch()==epoch;},error),"source bind");
 const auto* tensor=model.Find(name);
 Need(tensor && unsigned(tensor-model.Tensors().data())==index && tensor->inputs==width && tensor->outputs==rows && tensor->type==8,"tensor identity");
 const std::string path=std::string(argv[3])+"/projection.bin";
 int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);Need(fd>=0,"exclusive output");
 FILE* output=fdopen(fd,"wb");Need(output,"output stream");
 const uint32_t header[]={0x31525051,1,index,width,rows,bands,tiles,1088};Write(output,header,sizeof(header));
 std::vector<uint16_t> input(width);for(unsigned i=0;i<input.size();i++)input[i]=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));
 double squaredError=0,squaredReference=0,maxError=0;
 for(unsigned band=0;band<bands;band++){
  PioneerQ38NativeBfpBandData::Packet packet;
  Need(source.Stage(index,band*32,input.data(),input.size(),4,packet,error) && packet.width==width
   && packet.firstRow==band*32 && packet.weights.size()==tiles*1104,"cached band");
  std::vector<uint32_t> words(packet.weights.size()/4);std::memcpy(words.data(),packet.weights.data(),packet.weights.size());
  uint32_t result[32]={};double originalDot[32]={};
  for(unsigned tile=0;tile<tiles;tile++){
   std::array<float,1024> original{};std::array<uint8_t,1088> fresh{};
   Need(PioneerQ38NativeWeightTiles::ReadFloatTile(model,index,uint64_t(band)*tiles+tile,original.data(),original.size())
    && PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,original.data(),original.size(),fresh.data(),fresh.size())
    && !std::memcmp(fresh.data(),packet.weights.data()+tile*1104+16,1088),"fresh original conversion mismatch");
   for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
    const uint32_t bits=uint32_t(input[tile*32+col])<<16;float value;std::memcpy(&value,&bits,sizeof(value));
    originalDot[row]+=double(original[row*32+col])*double(value);
   }
   Write(output,fresh.data(),fresh.size());
   for(unsigned row=0;row<32;row++)for(unsigned half=0;half<2;half++){
    uint16_t a[16],b[16];for(unsigned c=0;c<16;c++){
     a[c]=Weight(words,tile,row,half*16+c);b[c]=input[tile*32+half*16+c];
    }
    for(unsigned phase=0;phase<4;phase++)result[row]=Instruction(a,b,phase,result[row]);
   }
  }
  for(unsigned row=0;row<32;row++){
   float actual;std::memcpy(&actual,&result[row],sizeof(actual));Need(std::isfinite(actual) && std::isfinite(originalDot[row]),"nonfinite comparison");
   const double delta=double(actual)-originalDot[row];squaredError+=delta*delta;squaredReference+=originalDot[row]*originalDot[row];
   maxError=std::max(maxError,std::fabs(delta));
  }
  Write(output,result,sizeof(result));
  if((band+1)%16==0){std::printf("VERIFIED_FULL_PROJECTION tensor%u bands%u/%u\n",index,band+1,bands);std::fflush(stdout);}
 }
 Need(source.Ready() && model.Stable() && model.Epoch()==epoch,"source changed");
 Need(!std::ferror(output) && !std::fflush(output) && !fsync(fd),"output flush");Need(!std::fclose(output),"output close");
 Need(squaredReference>0,"zero reference norm");
 std::printf("FULL_PROJECTION_ERROR tensor%u relative_l2=%.9g max_absolute=%.9g;ONE_SYNTHETIC_INPUT\n",index,std::sqrt(squaredError/squaredReference),maxError);
 std::printf("PASS_PRIVATE_FULL_PROJECTION tensor%u bands%u rows%u;FRESH_CONVERSION_AND_INDEPENDENT_INTEGER_REFERENCE;NO DEVICE OR SPEED\n",index,bands,rows);
}
