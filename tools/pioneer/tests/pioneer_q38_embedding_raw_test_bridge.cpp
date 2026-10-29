/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Host-only raw bridge implementation must NEVER compile as native/Work
#endif
#define PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE 1
#include "pioneer_q38_embedding_raw_test_bridge.h"
#include "pioneer_q38_whole_original_bytes.h"
struct Q38EmbeddingRawTestBridge::Source {WholeModelOriginalBytes original;unsigned embedding=851;};
Q38EmbeddingRawTestBridge::Q38EmbeddingRawTestBridge():source(new Source){}
Q38EmbeddingRawTestBridge::~Q38EmbeddingRawTestBridge(){delete source;}
bool Q38EmbeddingRawTestBridge::Open(const char* path){
 if(!source || !source->original.Open(path))return false;
 for(unsigned i=0;i<851;i++){Q38OriginalTensor d;if(!source->original.Describe(i,d))return false;
  if(d.name=="token_embd.weight"){
   if(source->embedding!=851 || d.type!=12 || d.inputs!=5120 || d.outputs!=248320)return false;
   source->embedding=i;
  }
 }
 return source->embedding<851 && Ready();
}
bool Q38EmbeddingRawTestBridge::Ready()const{return source && source->embedding<851 && source->original.Ready();}
void Q38EmbeddingRawTestBridge::Revoke(){if(source)source->original.Revoke();}
bool Q38EmbeddingRawTestBridge::Read(unsigned route,uint32_t address,uint8_t* bytes,unsigned count){
 return Ready() && source->original.Read(route,address,bytes,count);
}
bool Q38EmbeddingRawTestBridge::OriginalBlock(unsigned token,unsigned block,uint8_t* bytes,unsigned count){
 return Ready() && token<248320 && block<20 && count==144
  && source->original.ReadOriginal(source->embedding,uint64_t(token)*2880+block*144,144,bytes,count);
}
