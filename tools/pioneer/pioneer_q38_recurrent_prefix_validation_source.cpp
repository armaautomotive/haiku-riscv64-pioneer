/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE)
#error Actual original-source adapter must compile in a separate UNFLAGGED translation unit
#endif
#include "PioneerQ38RecurrentPrefixValidationSource.h"
#include "PioneerQ38RecurrentPrefixOriginalSource.h"
namespace {
class PrefixValidationOriginalAdapter final:public PioneerQ38RecurrentPrefixValidationSource {
 std::unique_ptr<PioneerQ38RecurrentPrefixOriginalSource> source;
 bool Output(const void* output,size_t bytes)const{
  const uintptr_t a=reinterpret_cast<uintptr_t>(output),b=reinterpret_cast<uintptr_t>(this);
  return output && bytes && a<=UINTPTR_MAX-bytes && !(a<b+sizeof(*this) && b<a+bytes);
 }
public:
 explicit PrefixValidationOriginalAdapter(std::unique_ptr<PioneerQ38RecurrentPrefixOriginalSource> p):source(std::move(p)){}
 bool Prepare()override{return source && source->Prepare();}
 bool SealBeforeNativeContext()override{return source && source->SealBeforeNativeContext();}
 bool Ready()const override{return source && source->Ready();}
 bool Contains(unsigned route,uint32_t address,unsigned bytes)const override{return Ready() && source->Contains(route,address,bytes);}
 bool Read(unsigned route,uint32_t address,uint8_t* output,unsigned bytes)override{
  return Output(output,bytes) && Ready() && source->Read(route,address,output,bytes) && Ready();
 }
 bool CompareFinal(const uint32_t* values,unsigned words,PioneerRecurrentOriginalComparison* result)const override{
  if(!values || words!=5120 || !Output(result,sizeof(*result)) || !Ready())return false;
  const uintptr_t a=reinterpret_cast<uintptr_t>(values),b=reinterpret_cast<uintptr_t>(result);
  if(a>UINTPTR_MAX-words*4 || (a<b+sizeof(*result) && b<a+words*4))return false;
  PioneerRecurrentOriginalComparison report;
  if(!source->CompareStage(2,PioneerRecurrentFinal,values,words,&report) || !Ready())return false;
  *result=report;return true;
 }
 uint64_t ReferenceReads()const override{return source?source->ReferenceReads():0;}
 uint64_t ReferenceFingerprint(unsigned layer)const override{return Ready()?source->ReferenceFingerprint(layer):0;}
 uint64_t Bytes()const override{return Ready()?source->Bytes():0;}
 unsigned Pages()const override{return Ready()?source->Pages():0;}
 unsigned Spans()const override{return Ready()?source->Spans():0;}
 bool Extent(unsigned index,PioneerRecurrentPrefixValidationExtent& output)const override{
  if(!Output(&output,sizeof(output)) || !Ready())return false;
  PioneerRecurrentOriginalExtent e;if(!source->Extent(index,e) || !Ready())return false;
  output={e.bank,e.route,e.address,e.bytes,e.pages};return true;
 }
};
}
std::unique_ptr<PioneerQ38RecurrentPrefixValidationSource> OpenPioneerQ38RecurrentPrefixValidationSource(const char* path){
 auto source=OpenPioneerQ38RecurrentPrefixOriginalSource(path);if(!source)return {};
 return std::make_unique<PrefixValidationOriginalAdapter>(std::move(source));
}
