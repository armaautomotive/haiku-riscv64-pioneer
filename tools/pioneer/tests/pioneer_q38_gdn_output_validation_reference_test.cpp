/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "../PioneerQ38GdnOutputValidationReference.h"
#include "../PioneerQ38CpuValidation.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
// Synthetic ORIGINAL row-major bytes. Not public-source authentication,
// actual GDN arithmetic,card transport or an inference performance test.
struct OutputReferenceSource {
 struct Tensor {
  std::string name="blk.0.ssm_out.weight";unsigned type=8,rank=2;
  uint64_t inputs=6144,outputs=5120,bytes=UINT64_C(5120)*6528,offset=123456;
 };
 struct Index {
  std::vector<Tensor> tensors={Tensor{}};
  const auto& Tensors()const{return tensors;}
  const Tensor* Find(const char* name)const{for(const auto& t:tensors)if(t.name==name)return &t;return nullptr;}
 } model;
 unsigned reads=0,failRead=0,dropRead=0,mutateRead=0,badRead=0,roundRead=0;bool valid=true;
 bool Ready()const{return valid;}
 const auto& Model()const{return model;}
 static uint16_t Scale(unsigned row,unsigned block){
  const uint16_t values[]={0x3c00,0xbc00,0x0001,0x8001,0,0x8000,0x3555,0xb555};return values[(row+block)%8];
 }
 static int Weight(unsigned row,unsigned block,unsigned lane){return int((row*7+block*11+lane*37)%256)-128;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* output,unsigned capacity){
  assert(!tensor && !(offset%(16*6528)) && bytes==16*6528 && capacity==bytes && offset+bytes<=UINT64_C(5120)*6528);
  if(++reads==failRead)return false;
  auto* data=static_cast<uint8_t*>(output);unsigned first=unsigned(offset/6528);
  for(unsigned row=0;row<16;row++)for(unsigned block=0;block<192;block++){
   auto* b=data+row*6528+block*34;uint16_t half=Scale(first+row,block);b[0]=uint8_t(half);b[1]=uint8_t(half>>8);
   for(unsigned lane=0;lane<32;lane++)b[2+lane]=uint8_t(Weight(first+row,block,lane)&255);
  }
  if(reads==badRead){data[bytes-34]=0;data[bytes-33]=0x7c;}
  if(reads==dropRead)valid=false;
  if(reads==mutateRead)model.tensors[0].offset++;
  if(reads==roundRead)assert(!std::fesetround(FE_DOWNWARD));
  return true;
 }
};
static float HalfHardware(uint16_t bits){_Float16 half;std::memcpy(&half,&bits,2);return float(half);}
int main(){
 using namespace PioneerQ38GdnOutputReferenceDetail;
 setvbuf(stdout,nullptr,_IOLBF,0);assert(std::fegetround()==FE_TONEAREST);
 unsigned halves=0,positive=0,refusals=0;
 for(unsigned raw=0;raw<65536;raw++)if((raw&0x7c00)!=0x7c00){assert(Bits(HalfFloat(uint16_t(raw)))==Bits(HalfHardware(uint16_t(raw))));halves++;}
 for(unsigned raw=0;raw<0x7bff;raw++){
  float lo=HalfHardware(uint16_t(raw)),hi=HalfHardware(uint16_t(raw+1));
  float mid=float((double(lo)+hi)*.5);
  for(float f:{lo,std::nextafter(mid,0.f),mid,std::nextafter(mid,INFINITY)}){
   uint16_t actual;_Float16 expected=static_cast<_Float16>(f);uint16_t bits;std::memcpy(&bits,&expected,2);
   assert(HalfBits(f,actual) && actual==bits);halves++;
  }
 }
 {uint16_t half;assert(HalfBits(65504.f,half) && half==0x7bff);
  assert(HalfBits(std::nextafter(65520.f,0.f),half) && half==0x7bff);
  assert(!HalfBits(65520.f,half) && !HalfBits(INFINITY,half) && !HalfBits(-1.f,half));}
 std::array<uint32_t,6144> input={};std::array<float,6144> x={};PioneerQ38GdnOutputReferenceResult output;
 unsigned domains=0;uint32_t random=0x534f5554;
 for(unsigned trial=0;trial<96;trial++){
  const int exponents[]={-32,-29,-25,-24,-14,-1,0,8,15,16,17};int exponent=exponents[trial%11];
  for(unsigned i=0;i<6144;i++){
   random=random*1664525+1013904223;
   float value=i%32==31?(i&32?-127.f:127.f):float(int((random>>8)%25301)-12650)*.01f;
   x[i]=std::ldexp(value,exponent);input[i]=Bits(x[i]);
  }
  PioneerQ38GdnOutputReferenceResult stage={};PioneerQ38CohortInput expected;
  bool actual=Input(input.data(),stage),qualified=PioneerQ38CohortQuantize(8,6144,x.data(),expected);
  assert(actual==qualified);
  if(actual)assert(!std::memcmp(stage.operand,expected.activation,sizeof(stage.operand))
   && !std::memcmp(stage.scale,expected.scales,sizeof(stage.scale)));
  domains++;
 }
 for(unsigned trial=0;trial<4;trial++){
  for(unsigned i=0;i<6144;i++){
   float value=trial==0?std::copysign(0.f,i&1?-1.f:1.f):trial==1?float(int((i*37+13)%255)-127)*.03125f:
    trial==2?float(int((i*37+13)%255)-127)*0x1p-29f:(i%32==31?127.f:(i&1?-.5f:.5f));
   input[i]=Bits(value);x[i]=value;
  }
  auto before=input;OutputReferenceSource source;
  assert(PioneerQ38GdnOutputReference(source,input.data(),6144,&output) && input==before && source.reads==320);
  PioneerQ38CohortInput quantized;assert(PioneerQ38CohortQuantize(8,6144,x.data(),quantized));
  assert(!std::memcmp(output.operand,quantized.activation,sizeof(output.operand)) && !std::memcmp(output.scale,quantized.scales,sizeof(output.scale)));
  if(trial==2){for(auto scale:output.scale)assert(!scale);assert(output.operand[0]!=0x40004000);}
  if(trial==3)assert((output.operand[0]&0xffff)==0x4001 && output.operand[0]>>16==0xc001); // AWAY,not ties-even
  std::vector<float> actual(5120);for(unsigned row=0;row<5120;row++)actual[row]=Float(output.values[row]);
  assert(PioneerQ38CpuValidate(source,0,x.data(),6144,actual.data(),5120) && source.reads==640);
  for(unsigned row=0;row<5120;row++){
   double sum=0,absolute=0;
   for(unsigned block=0;block<192;block++)for(unsigned lane=0;lane<32;lane++){
    unsigned at=block*32+lane;uint16_t operand=uint16_t(output.operand[at/2]>>((at%2)*16));
    int activation=operand&0x8000?-int(operand&1023):int(operand&1023);
    double term=double(HalfHardware(OutputReferenceSource::Scale(row,block)))*OutputReferenceSource::Weight(row,block,lane)
     *double(Float(output.scale[block]))*activation;
    sum+=term;absolute+=std::fabs(term);
   }
   assert(std::fabs(double(actual[row])-sum)<=1e-6+absolute*2e-6);
  }
  positive++;
 }
 // Every failure leaves the entire public result AND original input intact.
 auto refuse=[&](OutputReferenceSource& source,const uint32_t* in,unsigned words,PioneerQ38GdnOutputReferenceResult* out){
  std::memset(&output,0xa5,sizeof(output));auto saved=output;auto before=input;
  assert(!PioneerQ38GdnOutputReference(source,in,words,out));
  assert(!std::memcmp(&output,&saved,sizeof(output)) && input==before);refusals++;
 };
 for(unsigned kind=0;kind<11;kind++){
  OutputReferenceSource source;const uint32_t* in=input.data();auto* out=&output;unsigned words=6144;
  if(kind==0)in=nullptr;if(kind==1)out=nullptr;if(kind==2)words--;if(kind==3)words++;
  if(kind==4)in=reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(in)+1);
  if(kind==5)out=reinterpret_cast<PioneerQ38GdnOutputReferenceResult*>(reinterpret_cast<uint8_t*>(out)+1);
  if(kind==6)in=reinterpret_cast<const uint32_t*>(UINTPTR_MAX-3);
  if(kind==7)out=reinterpret_cast<PioneerQ38GdnOutputReferenceResult*>(UINTPTR_MAX-3);
  if(kind==8)in=output.values;if(kind==9)out=reinterpret_cast<PioneerQ38GdnOutputReferenceResult*>(input.data());
  if(kind==10)source.valid=false;
  refuse(source,in,words,out);
 }
 for(unsigned field=0;field<6;field++){
  OutputReferenceSource source;auto& t=source.model.tensors[0];
  if(field==0)t.name="blk.1.ssm_out.weight";if(field==1)t.type=12;if(field==2)t.rank=1;
  if(field==3)t.inputs--;if(field==4)t.outputs--;if(field==5)t.bytes--;
  refuse(source,input.data(),6144,&output);assert(!source.reads);
 }
 for(unsigned at:{0U,6143U})for(uint32_t bits:{1U,0x80000001U,0x00800000U,0x7f800000U,0xff800000U,0x7fc00001U,0x7f7fffffU}){
  OutputReferenceSource source;uint32_t saved=input[at];input[at]=bits;
  // A smallest-normal source with other large lanes is valid;put it in
  // an otherwise-zero block to exercise the overflowing reciprocal.
  auto savedInput=input;if(bits==0x00800000){unsigned block=at/32;for(unsigned i=0;i<32;i++)input[block*32+i]=0;input[at]=bits;}
  refuse(source,input.data(),6144,&output);assert(!source.reads);input=savedInput;input[at]=saved;
 }
 for(unsigned at:{1U,160U,320U})for(unsigned kind=0;kind<5;kind++){
  OutputReferenceSource source;
  if(kind==0)source.failRead=at;if(kind==1)source.dropRead=at;if(kind==2)source.badRead=at;
  if(kind==3)source.mutateRead=at;if(kind==4)source.roundRead=at;
  refuse(source,input.data(),6144,&output);assert(source.reads==at);assert(!std::fesetround(FE_TONEAREST));
 }
 for(int mode:{FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}){
  OutputReferenceSource source;assert(!std::fesetround(mode));refuse(source,input.data(),6144,&output);
  assert(!source.reads && !std::fesetround(FE_TONEAREST));
 }
 printf("PASS isolated output CPU observer half-boundaries%u/quantization-domains%u/positives%u/atomic-refusals%u;ALL6144 inputs/3072 operands/192 scales/5120 original-wire rows/FP32 bit-exact second oracle+FP64;NO device/upload/native/fallback/TPS\n",halves,domains,positive,refusals);
}
