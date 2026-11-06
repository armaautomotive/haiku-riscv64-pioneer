/* SPDX-License-Identifier: MIT */
// Private diagnostic source preparation,NO device/cache/Work/model context.
#include "PioneerNativeCompactQ4StreamSource.h"
#include "PioneerPreparedBf16Q4OriginalRows.h"
#include "PioneerQ38PreparedBf16Q4BlockData.h"
#include <vector>
int main(int argc,char** argv){
 if(argc!=3)return 2;
 PioneerNativeCompactQ4StreamSource source;std::string error;
 if(!source.Prepare(argv[1],error) || !source.Ready())return 1;
 std::array<std::vector<uint32_t>,9> payloads;
 for(unsigned s=0;s<9;s++)for(unsigned b=0;b<PioneerNativeCompactQ4StreamData::Width(s)/256;b++){
  std::array<uint8_t,4608> rows{};std::array<uint16_t,256> input{};std::array<uint8_t,5632> encoded{};
  if(!source.Ready() || !PioneerPreparedBf16Q4OriginalRows::Block(s,b,rows,input)
   || !PioneerQ38PreparedBf16Q4BlockData::Prepare(rows.data(),rows.size(),encoded.data(),encoded.size()))return 1;
  for(unsigned i=0;i<encoded.size();i+=4)payloads[s].push_back(uint32_t(encoded[i])|uint32_t(encoded[i+1])<<8|uint32_t(encoded[i+2])<<16|uint32_t(encoded[i+3])<<24);
 }
 const int directory=open(argv[2],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(directory<0)return 1;
 struct stat info{};if(fstat(directory,&info) || !S_ISDIR(info.st_mode) || info.st_uid!=geteuid() || (info.st_mode&0777)!=0700){close(directory);return 1;}
 const int fd=openat(directory,"PioneerNativePreparedBf16Q4Data.h",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);close(directory);if(fd<0)return 1;
 FILE* file=fdopen(fd,"wb");if(!file){close(fd);return 1;}
 fputs("/* PRIVATE derived weights;DO NOT COMMIT. */\n#ifndef PIONEER_NATIVE_PREPARED_BF16_Q4_DATA_H\n#define PIONEER_NATIVE_PREPARED_BF16_Q4_DATA_H\n#include <stdint.h>\n#include \"PioneerNativeCompactQ4StreamData.h\"\nstruct PioneerNativePreparedBf16Q4Data {\n static constexpr unsigned Version=1,Rows=32,Samples=9,Role=115,PreparedBytes=5632;\n static unsigned Width(unsigned s){return s<6?5120:s<9?17408:0;}\n static unsigned Words(unsigned s){return Width(s)/256*2688;}\n static uint32_t Word(unsigned s,unsigned i){\n if(s>=9 || i>=Words(s))return 0;\n const unsigned raw=Width(s)/256*1152,extra=Width(s)/256*1408;\n if(i<raw)return PioneerNativeCompactQ4StreamData::Word(s,i);\n if(i>=raw+extra)return PioneerNativeCompactQ4StreamData::Word(s,i-extra);\n i-=raw;\n",file);
 for(unsigned s=0;s<9;s++){
  fprintf(file," static const uint32_t p%u[%zu]={\n",s,payloads[s].size());
  for(unsigned i=0;i<payloads[s].size();i++)fprintf(file,"0x%08x%s",payloads[s][i],i+1==payloads[s].size()?"\n":i%8==7?",\n":",");
  fputs(" };\n",file);
 }
 fputs(" switch(s){\n",file);for(unsigned s=0;s<9;s++)fprintf(file," case %u:return p%u[i];\n",s,s);
 fputs(" default:return 0;}\n }\n};\n#endif\n",file);
 const bool okay=!ferror(file) && !fflush(file) && !fsync(fd) && source.Ready();const int closed=fclose(file);
 if(!okay || closed)return 1;
 puts("PASS_PREPARED115_PRIVATE_EXPORT sameOpenOriginalSHA_all9OriginalBindings_losslessNativePrimaryDenseMaskOnly_NO_DEVICE_OR_CPU_INFERENCE");return 0;
}
