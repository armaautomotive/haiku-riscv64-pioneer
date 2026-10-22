/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_FFN_ACTIVATION_MACHINE_ONLY
#include "pioneer_q38_ffn_activation_image_test.cpp"
#include "../PioneerQ38FfnNativeInput.h"
#include "../PioneerQwen38CohortHost.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchProtocol.h"
using Native=PioneerQ38FfnNativeInput;

// Independent wire oracle:host quantizer + explicit sign-magnitude encoding,
// not implementation's packing/validity helpers. ALL576 bytes are compared.
static void NativeReference(const uint32_t* q,Native& out)
{
 memset(&out,0,sizeof(out));out.scale[0]=q[0];
 for(unsigned i=0;i<256;i++) {
  unsigned byte=(q[1+i/4]>>((i%4)*8))&255;int value=int(byte)-int((byte&128)*2);
  uint32_t operand=0x4000|(value<0?0x8000|unsigned(-value):unsigned(value));
  out.operand[i/2]|=operand<<((i%2)*16);
 }
}
struct NativeActivationMachine:FfnActivationMachine {
 unsigned operandWrites=0,nativeScaleWrites=0;
 NativeActivationMachine(const char* path,unsigned trial,unsigned block):FfnActivationMachine(path,trial,block) {
  memory[0x74000]=0x4e384651;memory[0x74008]=576;
  for(unsigned i=0;i<256;i++) {
   // Keep these eight valid vectors INSIDE the unchanged native scale
   // domain. Explicit below/above-domain cases are tested independently.
   uint32_t bits=memory[0x90400+i*4];
   if((bits&0x7fffffff)>0x47800000)memory[0x90400+i*4]=(bits&0x80000000)|0x47800000;
   if(trial==4)memory[0x90400+i*4]=PioneerAttentionBits((i&1?-1:1)*0x1p20f);
  }
  if(trial>=5){memory[0x90000]=0x3f800000;memory[0x90400]=0x3f800000;}
  for(unsigned i=0;i<144;i++)memory[0x90c00+i*4]=0xdeadbeef;
  memory[0x90e40]=0xdecafbad;
 }
 uint32_t Load(uint32_t a) override {
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x74000,16) || Range(a,0x6e000,8192) || Range(a,0x90000,3648) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value) override {
  assert(!(a&3));assert(Range(a,0x6e000,8192) || Range(a,0x90800,1600) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,0x90800,1024))activatedWrites++;
  if(Range(a,0x90c00,512))operandWrites++;
  if(Range(a,0x90e00,64))nativeScaleWrites++;
  memory[a]=value;
 }
 bool RunNative() {
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(uint32_t guard:{0x73ffcU,0x74010U,0x8fffcU,0x90e40U})assert(memory.at(guard)==0xdecafbad);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x464e4651 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc0)==0x6400c && memory.at(0x17ffc8)==0x314e4651);
  return memory.at(0x17ffc4)==1;
 }
 void VerifyNative(float* full,Native& native) {
  float gate[256],up[256],host[256];Native expected,hostNative;uint32_t q[73];
  for(unsigned i=0;i<256;i++) {
   gate[i]=PioneerAttentionFloat(memory.at(0x90000+i*4));up[i]=PioneerAttentionFloat(memory.at(0x90400+i*4));
   full[i]=PioneerAttentionFloat(memory.at(0x90800+i*4));
   float reference=(gate[i]/(1.0f+expf(-gate[i])))*up[i];
   assert(isfinite(full[i]) && fabsf(full[i]-reference)<=3e-6f*fabsf(reference));
  }
  Q38ActivationReference(full,q);NativeReference(q,expected);
  assert(PioneerQ38FfnNativeActivation(gate,up,256,host,&hostNative));
  for(unsigned i=0;i<256;i++)assert(PioneerAttentionBits(host[i])==PioneerAttentionBits(full[i]));
  for(unsigned i=0;i<128;i++)native.operand[i]=memory.at(0x90c00+i*4);
  for(unsigned i=0;i<16;i++)native.scale[i]=memory.at(0x90e00+i*4);
  assert(!memcmp(&native,&expected,sizeof(native)) && !memcmp(&native,&hostNative,sizeof(native)));
  assert(activatedWrites==256 && operandWrites==128 && nativeScaleWrites==16);
 }
 void Refused() {
  assert(!RunNative() && !activatedWrites && !operandWrites && !nativeScaleWrites);
  for(unsigned i=0;i<256;i++)assert(memory.at(0x90800+i*4)==0xdeadbeef);
  for(unsigned i=0;i<144;i++)assert(memory.at(0x90c00+i*4)==0xdeadbeef);
 }
};

#ifndef PIONEER_Q38_FFN_NATIVE_ACTIVATION_MACHINE_ONLY
static void HostPacking(unsigned& positives,unsigned& negatives)
{
 float values[256];for(unsigned i=0;i<256;i++)values[i]=float(int(i)-128)*.25f;
 PioneerQ38ActivationK q;Q38ActivationReference(values,q.words);
 Native actual,expected;auto saved=q;
 auto accept=[&](){NativeReference(q.words,expected);assert(PioneerQ38FfnPackNative(&q,&actual));assert(!memcmp(&actual,&expected,sizeof(actual)));positives++;};
 for(uint32_t scale:{0x2f800000U,0xaf800000U,0x3f800000U,0xbf800000U,0x47800000U,0xc7800000U}){q=saved;q.words[0]=scale;accept();}
 q={};accept();
 auto refuse=[&](const PioneerQ38ActivationK* input,Native* output) {
  memset(&actual,0xa5,sizeof(actual));auto before=actual;
  assert(!PioneerQ38FfnPackNative(input,output));assert(!memcmp(&actual,&before,sizeof(actual)));negatives++;
 };
 for(uint32_t scale:{0x80000000U,1U,0x80000001U,0x2f7fffffU,0xaf7fffffU,0x47800001U,0xc7800001U,0x7f800000U,0xff800000U,0x7fc00001U}) {
  q=saved;q.words[0]=scale;refuse(&q,&actual);
 }
 for(unsigned lane=0;lane<256;lane++) {
  q=saved;q.words[1+lane/4]=(q.words[1+lane/4]&~(255U<<((lane%4)*8)))|(128U<<((lane%4)*8));refuse(&q,&actual);
 }
 for(unsigned group=0;group<16;group++){q=saved;q.words[65+group/2]^=1U<<((group%2)*16);refuse(&q,&actual);}
 q=saved;q.words[0]=0;refuse(&q,&actual); // nonzero values cannot have zero scale
 for(unsigned input=0;input<2;input++)for(unsigned bad=0;bad<3;bad++) {
  uintptr_t address=bad==0?0:bad==1?reinterpret_cast<uintptr_t>(&q)+1:~uintptr_t(3);
  refuse(input==0?reinterpret_cast<PioneerQ38ActivationK*>(address):&q,input==1?reinterpret_cast<Native*>(address):&actual);
 }
 for(unsigned offset:{0U,4U,288U}) {
  q=saved;auto before=q;refuse(&q,reinterpret_cast<Native*>(reinterpret_cast<uintptr_t>(&q)+offset));assert(!memcmp(&q,&before,sizeof(q)));
 }
}

static unsigned HostRanges()
{
 float gate[256],up[256],activated[256];Native native;unsigned refused=0;
 for(unsigned i=0;i<256;i++){gate[i]=1;up[i]=2;activated[i]=-13;}memset(&native,0xa5,sizeof(native));auto saved=native;
 auto check=[&](const float* g,const float* u,unsigned n,float* a,Native* out) {
  assert(!PioneerQ38FfnNativeActivation(g,u,n,a,out));for(float value:activated)assert(value==-13);
  assert(!memcmp(&native,&saved,sizeof(native)));refused++;
 };
 for(unsigned n:{0U,1U,255U,257U,17408U,UINT32_MAX})check(gate,up,n,activated,&native);
 for(unsigned field=0;field<4;field++)for(unsigned bad=0;bad<3;bad++) {
  uintptr_t address=bad==0?0:bad==1?reinterpret_cast<uintptr_t>(gate)+1:~uintptr_t(3);
  check(field==0?reinterpret_cast<float*>(address):gate,field==1?reinterpret_cast<float*>(address):up,256,
   field==2?reinterpret_cast<float*>(address):activated,field==3?reinterpret_cast<Native*>(address):&native);
 }
 for(unsigned first=0;first<4;first++)for(unsigned second=first+1;second<4;second++)for(unsigned offset:{0U,4U}) {
  uintptr_t p[]={reinterpret_cast<uintptr_t>(gate),reinterpret_cast<uintptr_t>(up),reinterpret_cast<uintptr_t>(activated),reinterpret_cast<uintptr_t>(&native)};
  p[second]=p[first]+offset;check(reinterpret_cast<float*>(p[0]),reinterpret_cast<float*>(p[1]),256,reinterpret_cast<float*>(p[2]),reinterpret_cast<Native*>(p[3]));
 }
 return refused;
}
int main(int argc,char** argv)
{
 assert(argc==2);unsigned positive=0,negative=0,packPositive=0,packNegative=0,maxSteps=0,minStack=0x70000;
 HostPacking(packPositive,packNegative);unsigned ranges=HostRanges();
 for(unsigned trial=0;trial<8;trial++) {
  std::vector<float> activated(17408);PioneerQ38CohortInput assembled;assembled.type=12;assembled.inputs=17408;
  for(unsigned block=0;block<68;block++) {
   NativeActivationMachine m(argv[1],trial,block);auto before=m.memory;assert(m.RunNative());Native out;
   m.VerifyNative(activated.data()+block*256,out);positive++;
   for(unsigned i=0;i<128;i++)assembled.activation[block*128+i]=out.operand[i];assembled.scales[block]=out.scale[0];
   maxSteps=std::max(maxSteps,m.steps);minStack=std::min(minStack,m.minStack);
   for(const auto& p:before)if((p.first<0x90800 || p.first>=0x90e40) && !ScalarMachine::Range(p.first,0x17ffc0,12))assert(m.memory.at(p.first)==p.second);
  }
  PioneerQ38CohortInput current;assert(PioneerQ38CohortQuantize(12,17408,activated.data(),current));
  assert(!memcmp(assembled.activation,current.activation,sizeof(current.activation)) && !memcmp(assembled.scales,current.scales,sizeof(current.scales)));
  auto request=std::make_unique<tt_q38_dispatch_probe>();request->version=1;request->type=12;request->inputs=17408;request->peers=1;
  request->part[0]={0,1,0};memcpy(request->activation,assembled.activation,sizeof(assembled.activation));memcpy(request->scales,assembled.scales,sizeof(assembled.scales));
  assert(PioneerQ38DispatchValid(*request));
  printf("OFFLINE actual RV32 native FFN activation trial%u ALL17408 lanes/68padded native blocks/exact unchanged host-wire+DispatchValid PASS\n",trial);fflush(stdout);
 }
 for(unsigned field=0;field<4;field++)for(unsigned bit=0;bit<32;bit++) {
  NativeActivationMachine m(argv[1],1,0);m.memory[0x74000+field*4]^=1U<<bit;m.Refused();negative++;
 }
 for(unsigned source=0;source<2;source++)for(unsigned lane=0;lane<256;lane++) {
  uint32_t upper=source?0x49800001:0x42000001;
  for(uint32_t invalid:{1U,0x80000001U,0x00800000U,0x2b7fffffU,0x7f800000U,0xff800000U,0x7fc00001U,0x7f7fffffU,upper,upper|0x80000000}) {
   NativeActivationMachine m(argv[1],1,0);m.memory[0x90000+source*1024+lane*4]=invalid;m.Refused();negative++;
  }
 }
 for(bool tooLarge:{false,true})for(bool sign:{false,true}) {
  NativeActivationMachine m(argv[1],1,0);
  for(unsigned i=0;i<256;i++){m.memory[0x90000+i*4]=PioneerAttentionBits(tooLarge?32.0f:-32.0f);m.memory[0x90400+i*4]=PioneerAttentionBits((sign?-1:1)*(tooLarge?0x1p20f:0x1p-40f));}
  m.Refused();negative++;
 }
 printf("PASS OFFLINE ACTUAL native FFN producer blocks%u descriptor/domain/scale refusals%u pack positives%u pack refusals%u host ranges%u maxsteps%u minstack%08x;ALL576B/independent signed native operands+scale/zero padded scale line/immutable inputs/atomic refusal/exact full17408 host-wire compatibility;NO_NATIVE_FULL_FFN_OR_SPEED_AUTHORITY\n",positive,negative,packPositive,packNegative,ranges,maxSteps,minStack);
}
#endif
