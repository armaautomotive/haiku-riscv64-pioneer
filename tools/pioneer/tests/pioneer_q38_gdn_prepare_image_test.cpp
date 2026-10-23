/* SPDX-License-Identifier: MIT */
// Execute ACTUAL emitted QGP1 instructions. Host references only OBSERVE;
// raw inputs,not precomputed q/k/gates,are supplied to the image.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#define PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
#include "pioneer_graph_math_image_test.cpp"
#ifndef PIONEER_Q38_GDN_PREPARE_MACHINE_ONLY
#define PIONEER_Q38_GDN_PREPARE_IMAGE 1
#include "../PioneerQ38GdnPrepare.h"
#endif
#include "../PioneerQ38DeltaVectorLayout.h"
#include <array>
namespace GL=PioneerQ38DeltaVectorLayout;
static double SoftplusReference(double x){return x>0?x+log1p(exp(-x)):log1p(exp(x));}
struct PrepareMachine:ScalarMachine {
 std::array<uint32_t,32> header={};std::array<uint32_t,324> input={};
 unsigned localWrites=0,descriptorWrites=0;
 PrepareMachine(const char* image,unsigned half,unsigned trial=0):ScalarMachine(image,128,0){
  memory.clear();header[0]=0x31504751;header[1]=128;header[2]=64;header[3]=half;SetHeader();
  unsigned h=half/2,kh=h%16;
  for(unsigned i=0;i<128;i++){
   float q=float(sin(kh*.117+i*.071)*2.7),k=float(cos(kh*.213+i*.091)*1.9);
   if(trial==1)q=k=0;if(trial==2){q=i&1?-65504.f:65504.f;k=-q;}
   if(trial==3){q=PioneerAttentionFloat(0x00800000);k=-q;}
   if(trial==4){q=PioneerAttentionFloat(i&1?0x80000000:0);k=-q;}
   input[i]=PioneerAttentionBits(q);input[128+i]=PioneerAttentionBits(k);
  }
  for(unsigned i=0;i<64;i++)input[256+i]=PioneerAttentionBits(i?float(sin((h*128+(half&1)*64+i)*.035)):PioneerAttentionFloat(0x80000000));
  input[320]=PioneerAttentionBits(float(h)*.13f-2);input[321]=PioneerAttentionBits(float(h)*.07f-2);
  input[322]=PioneerAttentionBits(-float(h+1)*.25f);input[323]=PioneerAttentionBits(-3.5f);
  if(trial>=5){
   input[320]=PioneerAttentionBits(trial&1?65504.f:-65504.f);input[321]=PioneerAttentionBits(trial&2?65504.f:-65504.f);
   input[322]=PioneerAttentionBits(trial&4?-65504.f:-PioneerAttentionFloat(0x00800000));
   input[323]=PioneerAttentionBits(trial&8?65504.f:-65504.f);
  }
  if(trial>=21){
   const float edge[]={80,81,84,87,88,100};assert(trial<27);
   input[320]=PioneerAttentionBits(edge[trial-21]);input[321]=PioneerAttentionBits(-edge[trial-21]);
   input[322]=PioneerAttentionBits(-1.0f);input[323]=0;
  }
  for(unsigned i=0;i<input.size();i++)memory[0x80000+i*4]=input[i];
  for(unsigned i=0;i<322;i++)memory[GL::Query+i*4]=0xdeadbeef;
  for(unsigned i=0;i<4;i++)memory[GL::Descriptor+i*4]=0xdeadbeef;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned a:{0x7fffcU,0x80510U,0x8dffcU,0x8e508U,0x745fcU,0x74680U,0x73ffcU,0x74010U})memory[a]=0xdecafbad;
 }
 void SetHeader(){for(unsigned i=0;i<32;i++)memory[0x74600+i*4]=header[i];}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,0x74600,128) || Range(a,0x80000,1296)
   || Range(a,GL::Query,1288) || Range(a,GL::Descriptor,16) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  assert(!(a&3));
  assert(Range(a,0x6e000,8192) || Range(a,GL::Query,1288) || Range(a,GL::Descriptor,16) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,GL::Query,1288))localWrites++;if(Range(a,GL::Descriptor,16))descriptorWrites++;
  memory[a]=v; // NO code/input/NoC/CSR/PCI/reset or resident GDDR writes
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned a:{0x7fffcU,0x80510U,0x8dffcU,0x8e508U,0x745fcU,0x74680U,0x73ffcU,0x74010U})assert(memory.at(a)==0xdecafbad);
  unsigned writes=localWrites,desc=descriptorWrites;
  for(unsigned i=0;i<32;i++)StepScalar();assert(localWrites==writes && descriptorWrites==desc);
  for(unsigned i=0;i<input.size();i++)assert(memory.at(0x80000+i*4)==input[i]);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x46504751 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==0x31504751 && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==64;
 }
 void Verify(){
  assert(localWrites==322 && descriptorWrites==5 && memory.at(GL::Descriptor)==GL::Magic
   && memory.at(GL::Descriptor+4)==128 && memory.at(GL::Descriptor+8)==64 && !memory.at(GL::Descriptor+12));
  float qs=0,ks=0;
  for(unsigned i=0;i<128;i++){float q=PioneerAttentionFloat(input[i]),k=PioneerAttentionFloat(input[128+i]);qs+=q*q;ks+=k*k;}
  float qi=(1.0f/sqrtf(qs+1e-6f))*0.0883883461356163f,ki=1.0f/sqrtf(ks+1e-6f);
  for(unsigned i=0;i<128;i++)for(unsigned which=0;which<2;which++){
   double reference=PioneerAttentionFloat(input[which*128+i])*(which?ki:qi);
   double actual=PioneerAttentionFloat(memory.at((which?GL::Key:GL::Query)+i*4));
   assert(isfinite(actual) && fabs(actual-reference)<=3e-6*(1+fabs(reference)));
  }
  for(unsigned i=0;i<64;i++)assert(memory.at(GL::Value+i*4)==input[256+i]);
  float shifted=PioneerAttentionFloat(input[320])+PioneerAttentionFloat(input[323]);
  float sp=float(SoftplusReference(shifted)),product=PioneerAttentionFloat(input[322])*sp;
  double decay=exp(double(product)),logit=PioneerAttentionFloat(input[321]);
  double e=exp(-fabs(logit)),beta=logit>=0?1/(1+e):e/(1+e);
  double actualDecay=PioneerAttentionFloat(memory.at(GL::Decay)),actualBeta=PioneerAttentionFloat(memory.at(GL::Beta));
  assert(isfinite(actualDecay) && isfinite(actualBeta) && fabs(actualDecay-decay)<=3e-6*(1+fabs(decay))
   && fabs(actualBeta-beta)<=3e-6*(1+fabs(beta)));
  // Absolute bounds alone would wrongly accept0 for exp(-80..-87).
  // Require RELATIVE accuracy for every normal gate;only real subnormals
  // may flush. This protects later large-state recurrent multiplication.
  for(unsigned i=0;i<2;i++){
   double expected=i?beta:decay,actual=i?actualBeta:actualDecay;
   if(expected>=double(PioneerAttentionFloat(0x00800000)))assert(actual>0 && fabs(actual-expected)<=3e-6*expected);
   else assert(actual==0);
  }
 }
 void RefuseValue(unsigned i,uint32_t bits){input[i]=bits;memory[0x80000+i*4]=bits;assert(!Run() && !localWrites && memory.at(GL::Descriptor)==0);}
};
#ifndef PIONEER_Q38_GDN_PREPARE_MACHINE_ONLY
static void HostMathAndPointers(){
 double maximum=0;
 for(int i=-131008;i<=131008;i++){
  float x=float(i)*.001f,got=PioneerQ38PrepareSoftplus(x);double expected=SoftplusReference(x);
  double error=fabs(got-expected)/(1+fabs(expected));maximum=std::max(maximum,error);assert(error<1e-6);
 }
 float q[128]={},k[128]={},out[258]={};
 assert(PioneerQ38PrepareHead(q,k,0,0,-1,0,out,out+128,out+256));
 for(unsigned kind=0;kind<10;kind++){
  const float* a=q;float* b=out;float* c=out+128;float* d=out+256;
  if(kind==0)a=nullptr;if(kind==1)b=nullptr;if(kind==2)c=nullptr;if(kind==3)d=nullptr;
  if(kind==4)b=q;if(kind==5)c=out;if(kind==6)d=out+127;
  if(kind==7)b=reinterpret_cast<float*>(uintptr_t(1));
  if(kind==8)a=reinterpret_cast<float*>(UINTPTR_MAX-3);
  if(kind==9)d=reinterpret_cast<float*>(UINTPTR_MAX-3);
  assert(!PioneerQ38PrepareHead(a,k,0,0,-1,0,b,c,d));
 }
 assert(!PioneerQ38PrepareGates(0,0,-1,0,reinterpret_cast<float*>(UINTPTR_MAX-3)));
 printf("PASS host-only softplus262017grid maxScaledError%.9g/11pointer-refusals;NO native/Work/fallback\n",maximum);
}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);HostMathAndPointers();
 unsigned positives=0,negatives=0,maxSteps=0,maxStack=0;
 for(unsigned half=0;half<96;half++){
  PrepareMachine m(argv[1],half);assert(m.Run());m.Verify();positives++;
  maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
 }
 for(unsigned trial=1;trial<27;trial++){
  PrepareMachine m(argv[1],95,trial);assert(m.Run());m.Verify();positives++;
  maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
 }
 for(unsigned i=0;i<32;i++)for(unsigned bit=0;bit<32;bit++){
  PrepareMachine m(argv[1],0);m.header[i]^=1U<<bit;
  if(i==3 && m.header[i]<96)continue;m.SetHeader();assert(!m.Run() && !m.localWrites && !m.descriptorWrites);negatives++;
 }
 for(unsigned at:{0U,127U,128U,255U,256U,319U,320U,321U,322U,323U})
  for(uint32_t bad:{1U,0x807fffffU,0x7f800000U,0xff800000U,0x7fc00000U}){
   PrepareMachine m(argv[1],0);m.RefuseValue(at,bad);negatives++;
  }
 for(unsigned at:{0U,127U,128U,255U,320U,321U,322U,323U}){
  PrepareMachine m(argv[1],0);m.RefuseValue(at,0x7f7fffff);negatives++;
 }
 for(uint32_t bad:{0U,0x80000000U,0x3f800000U}){PrepareMachine m(argv[1],0);m.RefuseValue(322,bad);negatives++;}
 printf("PASS ACTUAL QGP1 RV32IMF local prep positives%u/refusals%u/maxSteps%u/maxStack%u;rawqk128/v64/alpha-beta-originalNEGATIVE_A-dtBias->QK/gates/existingRGDNlayout;noNoC/GDDR/PCI/CSR/reset/nativeownership/originalGGUF/card/speedclaim\n",positives,negatives,maxSteps,maxStack);
}
#endif
