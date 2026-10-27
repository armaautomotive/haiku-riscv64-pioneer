/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38RecurrentBlockOriginalSource.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START independent ORIGINAL recurrent CPU source/authentication/cache/reference BEFORE native context;NO device entry");
 auto source=OpenPioneerQ38RecurrentBlockOriginalSource(argv[1]);assert(source && !source->Ready() && !source->SealBeforeNativeContext());
 assert(source->Prepare() && !source->Ready() && !source->Prepare() && source->SealBeforeNativeContext() && source->Ready()
  && !source->SealBeforeNativeContext() && source->Spans()==57 && source->Pages()==68603 && source->Bytes()==280971136);
 std::array<uint32_t,5120> values;values.fill(0xdeadbeef);PioneerRecurrentOriginalComparison report;report.checked=0x12345678;
 assert(!source->CompareStage(PioneerRecurrentFinal,values.data(),5119,&report) && report.checked==0x12345678);
 assert(!source->CompareStage(PioneerRecurrentStageCount,values.data(),5120,&report) && report.checked==0x12345678);
 assert(!source->CompareStage(PioneerRecurrentFinal,nullptr,5120,&report) && !source->CompareStage(PioneerRecurrentFinal,values.data(),5120,nullptr));
 assert(!source->CompareStage(PioneerRecurrentFinal,values.data(),5120,reinterpret_cast<PioneerRecurrentOriginalComparison*>(values.data())) && values[0]==0xdeadbeef);
 assert(source->CompareStage(PioneerRecurrentFinal,values.data(),5120,&report) && report.checked==5120 && report.differences && !report.passed);
 values[0]=0x7fc00000;assert(source->CompareStage(PioneerRecurrentFinal,values.data(),5120,&report) && report.nonfinite==1 && !report.passed);
 // Independent fixed-layout coordinate:token0 is the very first global
 // stripe,bank0/base04000000. No kernel/mutable-owner header is included.
 constexpr unsigned embeddingRoute=18U|(12U<<6),embeddingAddress=0x04000000;
 std::array<uint8_t,4096> bytes;bytes.fill(0xa5);
 assert(!source->Read(~0U,0,bytes.data(),64) && !source->Read(0,0,nullptr,64) && !source->Read(0,0,bytes.data(),0));
 for(auto v:bytes)assert(v==0xa5);
 assert(source->Contains(embeddingRoute,embeddingAddress,46080)
  && source->Read(embeddingRoute,embeddingAddress,bytes.data(),4096));
 assert(!source->EndRawPacket() && source->BeginRawPacket() && !source->BeginRawPacket());uint32_t value=0;
 assert(source->Word(embeddingRoute,embeddingAddress,value) && source->EndRawPacket() && !source->EndRawPacket());
 printf("PASS separate ORIGINAL recurrent CPU source/SAME authenticated c600 open file/57spans68603pages280971136bytes/private referenceReads%llu/fingerprint%016llx/prepare->seal BEFOREcontext/raw bytes only+postreturn strict comparison/invalid+overlap+nonfinite refusals;NO native,device,Work or TPS\n",
  (unsigned long long)source->ReferenceReads(),(unsigned long long)source->ReferenceFingerprint());
}
