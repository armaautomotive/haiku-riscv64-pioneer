/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38RecurrentPrefixOriginalSource.h"
#include <array>
#include <cassert>
#include <cfenv>
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START ORIGINAL autonomous-prefix CPU source/c600 authentication+RAW layers0..2+ONE embedding+independent chained reference BEFORE context;NO device entry");
#ifndef __HAIKU__
 assert(!OpenPioneerQ38RecurrentPrefixOriginalSource() && !OpenPioneerQ38RecurrentPrefixOriginalSource("relative.gguf"));
#endif
 auto source=OpenPioneerQ38RecurrentPrefixOriginalSource(argv[1]);assert(source && !source->Ready() && !source->SealBeforeNativeContext());
 PioneerRecurrentOriginalExtent extent,unchanged=extent;
 assert(!source->Extent(0,extent) && !std::memcmp(&extent,&unchanged,sizeof(extent)) && !source->ReferenceFingerprint(0));
 assert(source->Prepare() && !source->Ready() && !source->Prepare() && source->SealBeforeNativeContext() && source->Ready()
  && !source->SealBeforeNativeContext() && source->Spans()==169 && source->Pages()==205785 && source->Bytes()==842821248
  && source->ReferenceReads()==11557 && source->ReferenceFingerprint(0)==UINT64_C(0xbb911e57059b179f) && !source->ReferenceFingerprint(3));
 assert(source->ReferenceFingerprint(1) && source->ReferenceFingerprint(2) && source->ReferenceFingerprint(0)!=source->ReferenceFingerprint(1)
  && source->ReferenceFingerprint(1)!=source->ReferenceFingerprint(2));
 uint64_t bytes=0;unsigned pages=0;std::array<unsigned,7> bankSpans={};
 for(unsigned i=0;i<169;i++){
  assert(source->Extent(i,extent) && extent.bank<7 && extent.bytes && extent.pages==(extent.bytes+4095)/4096
   && source->Contains(extent.route,extent.address,extent.bytes));bytes+=extent.bytes;pages+=extent.pages;bankSpans[extent.bank]++;
 }
 for(unsigned count:bankSpans)assert(count);
 unchanged=extent;assert(bytes==842821248 && pages==205785 && !source->Extent(169,extent) && !std::memcmp(&extent,&unchanged,sizeof(extent)));
 assert(!source->Extent(0,*reinterpret_cast<PioneerRecurrentOriginalExtent*>(source.get())) && source->Ready());
 std::array<uint32_t,5120> values;values.fill(0xdeadbeef);PioneerRecurrentOriginalComparison report;report.checked=0x12345678;
 assert(!source->CompareStage(3,PioneerRecurrentFinal,values.data(),5120,&report) && report.checked==0x12345678);
 for(unsigned layer=0;layer<3;layer++){
  assert(!source->CompareStage(layer,PioneerRecurrentFinal,values.data(),5119,&report) && report.checked==0x12345678);
  assert(!source->CompareStage(layer,PioneerRecurrentStageCount,values.data(),5120,&report) && report.checked==0x12345678);
  assert(!source->CompareStage(layer,PioneerRecurrentFinal,nullptr,5120,&report) && !source->CompareStage(layer,PioneerRecurrentFinal,values.data(),5120,nullptr));
  assert(!source->CompareStage(layer,PioneerRecurrentFinal,values.data(),5120,reinterpret_cast<PioneerRecurrentOriginalComparison*>(values.data())) && values[0]==0xdeadbeef);
  assert(!source->CompareStage(layer,PioneerRecurrentFinal,values.data(),5120,reinterpret_cast<PioneerRecurrentOriginalComparison*>(source.get())));
  assert(source->CompareStage(layer,PioneerRecurrentFinal,values.data(),5120,&report) && report.checked==5120 && report.differences && !report.passed);
  values[0]=0x7fc00000;assert(source->CompareStage(layer,PioneerRecurrentFinal,values.data(),5120,&report) && report.nonfinite==1 && !report.passed);
  values[0]=0xdeadbeef;report.checked=0x12345678;
 }
 std::array<uint8_t,4096> raw;raw.fill(0xa5);
 assert(!source->Read(~0U,0,raw.data(),64) && !source->Read(0,0,nullptr,64) && !source->Read(0,0,raw.data(),0));
 for(auto byte:raw)assert(byte==0xa5);
 constexpr unsigned embeddingRoute=18U|(12U<<6),embeddingAddress=0x04000000;
 assert(source->Contains(embeddingRoute,embeddingAddress,46080) && source->Read(embeddingRoute,embeddingAddress,raw.data(),4096));
 assert(!source->Read(embeddingRoute,embeddingAddress,reinterpret_cast<uint8_t*>(source.get()),64) && source->Ready());
 assert(!source->EndRawPacket() && source->BeginRawPacket() && !source->BeginRawPacket() && !source->Extent(0,extent)
  && !source->CompareStage(0,PioneerRecurrentFinal,values.data(),5120,&report));
 uint32_t word=0;assert(source->Word(embeddingRoute,embeddingAddress,word) && source->EndRawPacket() && !source->EndRawPacket());
 word=0xa5a5a5a5;assert(!source->Word(embeddingRoute,embeddingAddress+1,word) && word==0xa5a5a5a5);
 std::fesetround(FE_DOWNWARD);assert(!source->Ready() && !source->BeginRawPacket() && !source->ReferenceFingerprint(0));
 std::fesetround(FE_TONEAREST);assert(source->Ready());
 printf("PASS ORIGINAL prefix SAME authenticated open file/169spans205785pages842821248RAWbytes/11557 independent reads/ONE embedding/chained actual layers0->1->2/private fingerprints%016llx,%016llx,%016llx/frozen layer0 exact/invalid+overlap+packet+rounding refusals;NO native,device,Work or TPS\n",
  (unsigned long long)source->ReferenceFingerprint(0),(unsigned long long)source->ReferenceFingerprint(1),(unsigned long long)source->ReferenceFingerprint(2));
}
