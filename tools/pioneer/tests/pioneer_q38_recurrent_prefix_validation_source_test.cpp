/* SPDX-License-Identifier: MIT */
// Adapter delegation over an explicit SYNTHETIC private CPU source. NOT
// actual full-SHA/original arithmetic/GPU/native or performance evidence.
#include "../PioneerQ38RecurrentPrefixValidationSource.h"
#include "../PioneerQ38RecurrentPrefixOriginalSource.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
static unsigned destroyed=0;
struct PrefixAdapterSyntheticSource: PioneerQ38RecurrentPrefixOriginalSource {
 bool prepared=false,sealed=false,valid=true,dropCompare=false;unsigned reads=0,compares=0,extents=0;
 ~PrefixAdapterSyntheticSource()override{destroyed++;}
 bool Prepare()override{if(prepared)return false;prepared=true;return true;}
 bool SealBeforeNativeContext()override{if(!prepared || sealed)return false;sealed=true;return true;}
 bool Ready()const override{return prepared && sealed && valid;}
 bool Contains(unsigned route,uint32_t address,unsigned bytes)const override{return Ready() && route==786 && address>=0x04000000 && bytes && uint64_t(address)-0x04000000+bytes<=64;}
 bool Read(unsigned route,uint32_t address,uint8_t* output,unsigned bytes)override{
  if(!output || !Contains(route,address,bytes))return false;
  reads++;memset(output,0x5a,bytes);return true;
 }
 bool BeginRawPacket()override{return false;}bool EndRawPacket()override{return false;}
 bool Word(unsigned,uint32_t,uint32_t&)const override{return false;}
 bool CompareStage(unsigned layer,PioneerRecurrentOriginalStage stage,const uint32_t* values,unsigned words,PioneerRecurrentOriginalComparison* out)const override{
  if(!Ready() || layer!=2 || stage!=PioneerRecurrentFinal || !values || words!=5120 || !out)return false;
  auto& self=*const_cast<PrefixAdapterSyntheticSource*>(this);self.compares++;
  *out={};out->checked=5120;out->passed=values[0]==0x12345678 && values[5119]==0x87654321;
  if(dropCompare)self.valid=false;return true;
 }
 uint64_t ReferenceReads()const override{return 11557;}
 uint64_t ReferenceFingerprint(unsigned layer)const override{return Ready() && layer<3?100+layer:0;}
 uint64_t Bytes()const override{return 842821248;}unsigned Pages()const override{return 205785;}unsigned Spans()const override{return 169;}
 bool Extent(unsigned index,PioneerRecurrentOriginalExtent& out)const override{
  if(index || !Ready())return false;
  const_cast<PrefixAdapterSyntheticSource*>(this)->extents++;out={0,786,0x04000000,64,1};return true;
 }
};
static PrefixAdapterSyntheticSource* original=nullptr;
std::unique_ptr<PioneerQ38RecurrentPrefixOriginalSource> OpenPioneerQ38RecurrentPrefixOriginalSource(const char* path){
 if(!path || strcmp(path,"synthetic-adapter-only"))return {};
 auto source=std::make_unique<PrefixAdapterSyntheticSource>();original=source.get();return source;
}
int main(){
 assert(!OpenPioneerQ38RecurrentPrefixValidationSource("bad-path"));
 auto source=OpenPioneerQ38RecurrentPrefixValidationSource("synthetic-adapter-only");assert(source && original && !source->Ready());
 std::array<uint8_t,64> bytes;bytes.fill(0xa5);assert(!source->Read(786,0x04000000,bytes.data(),64));
 assert(source->Prepare() && !source->Ready() && source->SealBeforeNativeContext() && source->Ready());
 assert(!source->Prepare() && !source->SealBeforeNativeContext() && source->ReferenceReads()==11557 && source->Spans()==169
  && source->Pages()==205785 && source->Bytes()==842821248 && !source->ReferenceFingerprint(3));
 for(unsigned layer=0;layer<3;layer++)assert(source->ReferenceFingerprint(layer)==100+layer);
 assert(source->Contains(786,0x04000000,64) && !source->Contains(978,0x04000000,64));
 assert(source->Read(786,0x04000000,bytes.data(),64));for(auto b:bytes)assert(b==0x5a);
 auto reads=original->reads;
 assert(!source->Read(786,0x04000000,reinterpret_cast<uint8_t*>(source.get()),4)
  && !source->Read(786,0x04000000,reinterpret_cast<uint8_t*>(UINTPTR_MAX-3),64) && original->reads==reads && source->Ready());
 PioneerRecurrentPrefixValidationExtent extent;
 assert(source->Extent(0,extent) && extent.route==786 && extent.bytes==64 && extent.pages==1);
 auto before=extent;assert(!source->Extent(1,extent) && !memcmp(&before,&extent,sizeof(extent)));
 auto extents=original->extents;assert(!source->Extent(0,*reinterpret_cast<PioneerRecurrentPrefixValidationExtent*>(source.get())) && original->extents==extents);
 std::array<uint32_t,5120> values={};values[0]=0x12345678;values[5119]=0x87654321;
 PioneerRecurrentOriginalComparison report;assert(source->CompareFinal(values.data(),5120,&report) && report.checked==5120 && report.passed);
 auto comparisons=original->compares;
 assert(!source->CompareFinal(values.data(),5119,&report) && !source->CompareFinal(nullptr,5120,&report)
  && !source->CompareFinal(values.data(),5120,reinterpret_cast<PioneerRecurrentOriginalComparison*>(source.get()))
  && !source->CompareFinal(values.data(),5120,reinterpret_cast<PioneerRecurrentOriginalComparison*>(values.data())) && original->compares==comparisons);
 auto previous=report;original->dropCompare=true;assert(!source->CompareFinal(values.data(),5120,&report) && !memcmp(&previous,&report,sizeof(report)));
 assert(!source->Ready() && !source->Contains(786,0x04000000,64) && !source->Bytes() && !source->Pages() && !source->Spans() && !source->ReferenceFingerprint(0));
 source.reset();assert(destroyed==1);
 puts("PASS UNFLAGGED prefix validation source adapter/private stage2-FINAL comparison/169span identity forwarding/prepare+seal/raw bytes/3fingerprints/self+range+result aliases/atomic failed comparison+CPU-only destruction;SYNTHETIC source,NOT full-SHA/original arithmetic/native/physical qualification");
}
