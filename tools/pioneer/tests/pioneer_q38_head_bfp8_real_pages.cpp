/* SPDX-License-Identifier: MIT */
// Authenticated original head -> page reader vs separate original-row tile
// reader. Read-only CPU diagnostic; no persistent cache, upload or device.
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#include "../PioneerQ38HeadBfp8FragmentSource.h"
#include "../PioneerQ38NativeWeightTiles.h"
#include "../PioneerModelWeightCache.h"
#include <array>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc,char** argv){
 if(argc!=2 || argv[1][0]!='/')return 2;
 int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* file=fdopen(fd,"rb");if(!file){close(fd);return 1;}
 PioneerModelWeightCache::Digest digest={};std::string error;
 puts("Authenticating 27B GGUF for read-only output-head page samples");fflush(stdout);
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747") {fclose(file);return 1;}
 PioneerQwen38Gguf model;PioneerQwen38PackedPlan plan;
 if(!model.Open(file) || !model.Stable() || !plan.Build(model,8192,true,true)){fclose(file);return 1;}
 unsigned stripes=0,pages=0,rows=0;
 for(const auto& part:plan.Parts()){
  if(part.kind!=PIONEER_Q38_WEIGHT || part.layer!=64 || part.role!=1)continue;
  if(part.firstRow!=rows || part.rows<64 || part.bank>=7){fclose(file);return 1;}
  rows+=part.rows;PioneerQ38HeadBfp8FragmentSource<PioneerQwen38Gguf> reader;
  if(!reader.Bind(model,part) || reader.Bytes()!=uint64_t(part.rows)*5440 || reader.Pages()<43){fclose(file);return 1;}
  for(unsigned page:{0U,42U,reader.Pages()-1}){
   std::array<uint8_t,4096> actual={},expected={};
   const uint64_t offset=uint64_t(part.firstRow/32)*160*1088+uint64_t(page)*4096;
   if(!reader.ReadPage(page,actual.data(),actual.size())
    || !PioneerQ38NativeWeightTiles::ReadRange(model,part.tensor,offset,expected.size(),expected.data(),expected.size())
    || actual!=expected || !model.Stable()){fclose(file);return 1;}
   pages++;
  }
  printf("SAMPLED bank%u rows%u..%u pages3 bytes_per_band174080\n",part.bank,part.firstRow,part.firstRow+part.rows-1);fflush(stdout);stripes++;
 }
 if(stripes!=7 || pages!=21 || rows!=248320 || !model.Stable() || !plan.Ready()){fclose(file);return 1;}
 puts("PASS original Q6 head seven stripes/21 sampled BFP8 pages incl cross-band against separate row reader; CPU only, NO full conversion, GPU or speed");
 return fclose(file)?1:0;
}
