/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "../PioneerQ38MixedLiveValidationReference.h"
#include "pioneer_q38_mixed_live_reference_source.h"
// TEST comparisons only. Independent reference above includes neither.
#include "../PioneerQ38ProjectionInput.h"
#include "../TenstorrentGgufQuant.h"
#include "../PioneerQ38DensePrepare.h"
#include <cstdio>
int main(){
 using namespace PioneerQ38MixedLiveReferenceDetail;setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned halfCases=0,quantCases=0,refusals=0,matrices=0;uint32_t random=0x51424c52;
 (void)&PioneerQ38LearnedRms;(void)&PioneerQ38DensePrepare::Run;
 for(unsigned trial=0;trial<8;trial++){
  std::array<uint32_t,256> input,weight,output;std::array<float,256> x,w,actual;std::array<float,64> rope={};
  for(unsigned i=0;i<32;i++)rope[i*2]=1;
  for(unsigned i=0;i<256;i++){
   x[i]=trial<4?Float((trial&1 || i&1)?0x80000000:0):float(int(i*37%257)-128)*.03125f;
   w[i]=(trial&2 && i&1)?-1.f:1.f;input[i]=Bits(x[i]);weight[i]=Bits(w[i]);
  }
  assert(Rms(input.data(),weight.data(),256,output.data()));RopeZero(output.data());
  assert(PioneerQ38DensePrepare::Head(x.data(),w.data(),rope.data(),actual.data()));
  for(unsigned i=0;i<256;i++)assert(output[i]==Bits(actual[i]));
 }
 for(unsigned bits=0;bits<65536;bits++){
  if((bits&0x7c00)==0x7c00)continue;float x=HalfFloat(bits);_Float16 h;uint16_t b=uint16_t(bits);std::memcpy(&h,&b,2);
  assert(Bits(x)==Bits(float(h)) && HalfRne(x)==bits);halfCases++;
 }
 for(unsigned i=0;i<100000;i++){
  random=random*1664525+1013904223;uint32_t b=(random&0x807fffff)|((random%250)<<23);float x=Float(b);
  _Float16 h=_Float16(x);uint16_t bits;std::memcpy(&bits,&h,2);assert(HalfRne(x)==bits);halfCases++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned trial=0;trial<16;trial++){
  std::array<uint32_t,256> input;std::array<float,256> raw;
  for(unsigned i=0;i<256;i++){random=random*1664525+1013904223;raw[i]=float(int(random%25401)-12700)*.00031f;}
  if(!trial)raw.fill(-0.f);
  if(trial==1 || trial==2)for(unsigned b=0;b<8;b++){
   raw[b*32]=trial==1?127.f:-127.f;raw[b*32+1]=.5f;raw[b*32+2]=-.5f;raw[b*32+3]=1.5f;raw[b*32+4]=-1.5f;
  }
  for(unsigned i=0;i<256;i++)input[i]=Bits(raw[i]);std::vector<int> q;std::vector<float> scale;
  assert(Input(type,input.data(),256,q,scale));PioneerQ38ProjectionInput actual;
  assert(PioneerQ38ProjectionQuantize(type,raw.data(),256,&actual));
  for(unsigned i=0;i<256;i++){unsigned v=actual.operand[i/2]>>((i%2)*16)&65535;assert(q[i]==int(v&1023)*(v&0x8000?-1:1));}
  for(unsigned i=0;i<scale.size();i++)assert(Bits(scale[i])==actual.scale[i]);quantCases++;
 }
 for(unsigned slot:{4U,8U,11U}){
  MixedReferenceSource source;View<MixedReferenceSource> view(source);assert(view.Bind());const auto& t=source.model.tensors[slot];
  unsigned width=t.type==8?32:256,blocks=t.inputs/width;std::vector<uint32_t> input(t.inputs),output(t.outputs,0xdeadbeef);
  for(unsigned i=0;i<t.inputs;i++)input[i]=Bits(float(int(i*29%25301)-12650)*.000031f);
  std::vector<int> q;std::vector<float> scale;assert(Input(t.type,input.data(),t.inputs,q,scale));
  assert(view.Matrix(slot,input.data(),output.data()) && source.reads==t.outputs/16);
  for(unsigned row:{0U,15U,16U,t.outputs/2,t.outputs-1}){
   float exact=0;double wide=0,absolute=0;
   for(unsigned b=0;b<blocks;b++)for(unsigned panel=0;panel<width/16;panel++){
    int total=0,dot=0;float coefficient=t.type==8?0x1p-11f:t.type==14?0x1p-20f*MixedReferenceSource::Scale6(row,b,panel)
      :0x1p-20f*MixedReferenceSource::Scale(row,b,panel/2);
    float minimum=t.type==12?0x1p-21f*float(-int(MixedReferenceSource::Minimum(row,b,panel/2))):0.f;
    for(unsigned i=0;i<16;i++){
     unsigned lane=panel*16+i;int w=t.type==8?MixedReferenceSource::Q8(row,b,lane):t.type==14?int(MixedReferenceSource::Q6(row,b,lane))-32:int(MixedReferenceSource::Q4(row,b,lane));
     int x=q[b*width+lane];total+=x;dot+=w*x;double term=(double(coefficient)*w+minimum)*(double(scale[b])*x);wide+=term;absolute+=std::fabs(term);
    }
    float c=coefficient*scale[b],m=minimum*scale[b],product=c*float(dot),correction=m*float(total);exact+=product+correction;
   }
   assert(output[row]==Bits(exact) && std::fabs(double(Float(output[row]))-wide)<=1e-7+absolute*3e-6);
  }matrices++;
 }
 // Every descriptor, including a later tensor not yet read, is sealed
 // before any bytes. Corruptions cannot retarget a later reference stage.
 for(unsigned slot=0;slot<12;slot++)for(unsigned field=0;field<7;field++){
  MixedReferenceSource source;View<MixedReferenceSource> view(source);assert(view.Bind());auto& t=source.model.tensors[slot];
  if(field==0)t.name+="x";if(field==1)t.type++;if(field==2)t.rank++;if(field==3)t.inputs++;if(field==4)t.outputs++;if(field==5)t.bytes++;if(field==6)t.offset++;
  uint8_t bytes[4];assert(!view.Read(1,0,4,bytes) && !source.reads);refusals++;
 }
 for(unsigned after:{1U,64U})for(unsigned kind=0;kind<7;kind++){
  MixedReferenceSource source;View<MixedReferenceSource> view(source);assert(view.Bind());
  if(kind==0)source.failRead=after;if(kind==1)source.mutateRead=after;if(kind==2)source.dropRead=after;
  if(kind==3)source.epochRead=after;if(kind==4)source.reallocateRead=after;if(kind==5)source.roundRead=after;if(kind==6)source.badRead=after;
  std::vector<uint32_t> input(5120,Bits(.1f)),output(1024,0xdeadbeef);
  assert(!view.Matrix(4,input.data(),output.data()) && source.reads==after);for(auto v:output)assert(v==0xdeadbeef);
  assert(!std::fesetround(FE_TONEAREST));refusals++;
 }
 auto output=std::make_unique<PioneerQ38MixedLiveReferenceProducts>();
 {
  MixedReferenceSource source;View<MixedReferenceSource> view(source);assert(view.Bind());std::vector<uint32_t> x(6144,Bits(.1f)),y(5120,0xdeadbeef);
  for(uintptr_t bad:{uintptr_t(0),uintptr_t(1),UINTPTR_MAX-3}){
   assert(!view.Matrix(8,reinterpret_cast<const uint32_t*>(bad),y.data()) && !view.Matrix(8,x.data(),reinterpret_cast<uint32_t*>(bad))
    && !PioneerQ38MixedLiveOriginalReference(source,0,reinterpret_cast<PioneerQ38MixedLiveReferenceProducts*>(bad)) && !source.reads);refusals+=3;
  }
  assert(!view.Matrix(8,x.data(),x.data()) && !view.Matrix(8,x.data(),x.data()+1) && !source.reads);refusals+=2;
  for(unsigned slot:{0U,1U,2U,6U,7U,12U,UINT32_MAX}){assert(!view.Matrix(slot,x.data(),y.data()) && !source.reads);refusals++;}
  assert(!PioneerQ38MixedLiveOriginalReference(source,248320,output.get()) && !source.reads);refusals++;
  std::array<uint32_t,256> weights;weights.fill(Bits(1.f));
  assert(!Rms(x.data(),weights.data(),256,x.data()) && !Rms(x.data(),weights.data(),256,weights.data()));refusals+=2;
 }
 for(unsigned kind=0;kind<8;kind++){
  MixedReferenceSource source;std::memset(output.get(),0xa5,sizeof(*output));
  if(kind==0)source.valid=false;if(kind==1)source.model.epoch=0;if(kind==2)source.model.tensors.pop_back();
  if(kind==3)source.model.tensors[12]=source.model.tensors[0];if(kind==4)source.failRead=1;
  if(kind==5)source.mutateRead=1;if(kind==6)source.badRead=1;if(kind==7)source.roundRead=1;
  assert(!PioneerQ38MixedLiveOriginalReference(source,0,output.get()));
  const auto* b=reinterpret_cast<const uint8_t*>(output.get());for(unsigned i=0;i<sizeof(*output);i++)assert(b[i]==0xa5);
  assert(!std::fesetround(FE_TONEAREST));refusals++;
 }
 for(bool zero:{false,true}){
  MixedReferenceSource source;source.zero=zero;assert(PioneerQ38MixedLiveOriginalReference(source,0,output.get()) && source.reads==3717);
  uint64_t hash=14695981039346656037ULL;const auto* p=reinterpret_cast<const uint8_t*>(output.get());
  for(unsigned i=0;i<sizeof(*output);i++){hash^=p[i];hash*=1099511628211ULL;}
  bool nonzero=false;for(unsigned i=0;i<5120;i++){
   assert(Domain(output->finalHidden[i]));if(zero)assert(output->finalHidden[i]==output->embedding[i]);nonzero|=output->projection[i]!=0;
  }assert(zero || nonzero);printf("PASS independent dense ORIGINAL-row CPU pipeline zero%u/3717reads/12sealedtensors/7matrices/Q8_0+Q6_K+Q4_K/embedding+RMS+position0attention+FFN+final fingerprint%016llx;NO device/reference feedback\n",zero,(unsigned long long)hash);
  std::vector<uint32_t> actual(output->finalHidden,output->finalHidden+5120);PioneerQ38MixedLiveReferenceComparison report;
  assert(PioneerQ38CompareMixedLiveFinal(actual.data(),5120,*output,&report) && report.passed && report.checked==5120 && !report.differences && !report.relativeL2);
  actual[5119]^=1;assert(PioneerQ38CompareMixedLiveFinal(actual.data(),5120,*output,&report) && !report.passed && report.differences==1);
  actual[0]=0x7fc00000;assert(PioneerQ38CompareMixedLiveFinal(actual.data(),5120,*output,&report) && !report.passed && report.nonfinite==1 && !std::isfinite(report.relativeL2));
  report.checked=0xdeadbeef;
  assert(!PioneerQ38CompareMixedLiveFinal(nullptr,5120,*output,&report) && !PioneerQ38CompareMixedLiveFinal(actual.data(),5119,*output,&report)
   && !PioneerQ38CompareMixedLiveFinal(actual.data(),5120,*output,nullptr)
   && !PioneerQ38CompareMixedLiveFinal(actual.data(),5120,*output,reinterpret_cast<PioneerQ38MixedLiveReferenceComparison*>(actual.data()))
   && !PioneerQ38CompareMixedLiveFinal(actual.data(),5120,*output,reinterpret_cast<PioneerQ38MixedLiveReferenceComparison*>(output.get())) && report.checked==0xdeadbeef);refusals+=5;
  const uint32_t* expectedStages[]={output->embedding,output->normalized,output->qg,output->key,output->value,output->query,output->gate,
   output->halfKey,output->halfValue,output->attention,output->projection,output->residual,output->postNormalized,output->ffnGate,
   output->ffnUp,output->activated,output->down,output->finalHidden};
  const unsigned stageWords[]={5120,5120,12288,1024,1024,6144,6144,1024,1024,6144,5120,5120,5120,17408,17408,17408,5120,5120};
  static_assert(sizeof(expectedStages)/sizeof(*expectedStages)==PioneerQ38MixedStageCount,"all private stages");
  for(unsigned i=0;i<PioneerQ38MixedStageCount;i++){
   auto stage=PioneerQ38MixedLiveReferenceStage(i);unsigned words=stageWords[i];std::vector<uint32_t> values(expectedStages[i],expectedStages[i]+words);
   assert(PioneerQ38CompareMixedLiveStage(values.data(),words,*output,stage,&report) && report.passed && report.checked==words);
   values[words-1]^=1;
   assert(PioneerQ38CompareMixedLiveStage(values.data(),words,*output,stage,&report) && !report.passed && report.differences==1);
   values[0]=0x7fc00000;
   assert(PioneerQ38CompareMixedLiveStage(values.data(),words,*output,stage,&report) && !report.passed && report.nonfinite==1
    && !std::isfinite(report.relativeL2));
   report.checked=0xdeadbeef;
   assert(!PioneerQ38CompareMixedLiveStage(values.data(),words-1,*output,stage,&report)
    && !PioneerQ38CompareMixedLiveStage(nullptr,words,*output,stage,&report)
    && !PioneerQ38CompareMixedLiveStage(reinterpret_cast<const uint32_t*>(1),words,*output,stage,&report)
    && !PioneerQ38CompareMixedLiveStage(reinterpret_cast<const uint32_t*>(UINTPTR_MAX-3),words,*output,stage,&report)
    && !PioneerQ38CompareMixedLiveStage(values.data(),words,*output,stage,nullptr)
    && !PioneerQ38CompareMixedLiveStage(values.data(),words,*output,stage,reinterpret_cast<PioneerQ38MixedLiveReferenceComparison*>(values.data()))
    && !PioneerQ38CompareMixedLiveStage(values.data(),words,*output,stage,reinterpret_cast<PioneerQ38MixedLiveReferenceComparison*>(output.get()))
    && report.checked==0xdeadbeef);refusals+=7;
   assert(!std::fesetround(FE_DOWNWARD));
   assert(!PioneerQ38CompareMixedLiveStage(values.data(),words,*output,stage,&report) && report.checked==0xdeadbeef);refusals++;
   assert(!std::fesetround(FE_TONEAREST));
  }
  for(unsigned invalid:{unsigned(PioneerQ38MixedStageCount),UINT32_MAX}){
   report.checked=0xdeadbeef;assert(!PioneerQ38CompareMixedLiveStage(actual.data(),5120,*output,PioneerQ38MixedLiveReferenceStage(invalid),&report)
    && report.checked==0xdeadbeef);refusals++;
  }
 }
 printf("PASS independent dense CPU reference half%u/quant%u/typedmatrices%u/refusals%u/atomic publication and global metadata/epoch/rounding seal;NOT original-file or native/card/Work/full64/TPS qualification\n",halfCases,quantCases,matrices,refusals);
}
