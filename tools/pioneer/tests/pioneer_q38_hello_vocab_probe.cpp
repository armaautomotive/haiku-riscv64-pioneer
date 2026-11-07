/* SPDX-License-Identifier: MIT */
// CPU-only, read-only fixed vocabulary probe. No tokenizer or device inference.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error This is a standalone vocabulary inspection, not Work or kernel code
#endif
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
#include <cstdio>
#include <cstring>
#include <memory>
int main(int argc,char** argv){
 if((argc!=2 && (argc!=3 || strcmp(argv[2],"--candidate-prefixes"))) || argv[1][0]!='/')return 2;
 std::unique_ptr<FILE,decltype(&fclose)> f(fopen(argv[1],"rb"),&fclose);
 if(!f)return 1;
 PioneerQwen38Gguf model;if(!model.Open(f.get()) || !model.Stable())return 1;
 PioneerModelWeightCache::Digest digest={};std::string error;
 puts("VOCAB_PROBE authenticating original; read-only CPU, no device");fflush(stdout);
 if(!PioneerModelWeightCache::HashFile(fileno(f.get()),PioneerQwen38Gguf::FullBytes,digest,error)
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
  || !model.Stable() || fseeko(f.get(),0,SEEK_SET))return 1;
 PioneerGgufReader r(f.get(),PioneerQwen38Gguf::FullBytes);
 if(r.Number(4)!=0x46554747 || r.Number(4)!=3 || r.Number(8)!=851)return 1;
 const char* words[]={"Hello","What","Why","The","def"};
 constexpr unsigned choices=sizeof(words)/sizeof(words[0]);unsigned found[choices]={},id[choices]={};
 const auto count=r.Number(8);bool tokens=false;
 for(uint64_t i=0;i<count;i++){
  char key[513];if(!r.String(key,sizeof(key)))return 1;
  const auto type=uint32_t(r.Number(4));
  if(strcmp(key,"tokenizer.ggml.tokens")){if(!r.Value(type))return 1;continue;}
  if(tokens || type!=9 || r.Number(4)!=8 || r.Number(8)!=248320)return 1;
  tokens=true;
  for(unsigned n=0;n<248320;n++){
   const auto len=r.Number(8);if(!r.good || len>32*1024*1024)return 1;
   if(len>5){if(!r.Skip(len))return 1;continue;}
   char value[5];if(!r.Take(value,len))return 1;
   for(unsigned w=0;w<choices;w++)if(len==strlen(words[w]) && !memcmp(value,words[w],len)){found[w]++;id[w]=n;}
  }
 }
 if(!tokens || found[0]!=1 || id[0]!=9419 || !r.good || !model.Stable())return 1;
 printf("VOCAB_HELLO_TOKEN %u; METADATA_MATCH_ONLY_NOT_TOKENIZER_OR_MODEL_QUALITY\n",id[0]);
 if(argc==3)for(unsigned w=1;w<choices;w++)
  printf("VOCAB_CANDIDATE word=%s matches=%u id=%u; METADATA_ONLY\n",words[w],found[w],id[w]);
 return fflush(stdout)?1:0;
}
