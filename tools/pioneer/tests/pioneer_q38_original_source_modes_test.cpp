/* SPDX-License-Identifier: MIT */
// Original-model observer regression ONLY,not a controller/ready producer.
#include "pioneer_q38_decoder_original_source.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 using Factory=std::unique_ptr<DecoderOriginalSource> (*)(const char*);
 unsigned modes=0;
 for(Factory factory:{OpenDecoderOriginalSource,OpenGdnInputOriginalSource,OpenGdnRecurrentOriginalSource}){
  auto source=factory(argv[1]);assert(source && source->Prepare(0) && source->Ready() && source->ReferenceReads()==1672
   && !source->GdnLearnedNorm() && !source->ProjectionReads() && !source->ConvolutionReads() && !source->GdnInputReads()
   && source->SealBeforeNativeContext() && !source->SealBeforeNativeContext());
  uint32_t fake[64]={};uint8_t bytes[512]={};assert(!source->CheckGdnGatedNorm(0,fake) && !source->Read(129,0,bytes,512));
  printf("PASS OLD original observer mode%u/reference1672/sealonce/NO new norm data-oracle or512byte route\n",modes++);
 }
 assert(modes==3);puts("PASS3 original-model prior observer modes unchanged;NO native context/device/Work");
}
