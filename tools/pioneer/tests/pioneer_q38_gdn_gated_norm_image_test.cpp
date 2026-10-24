/* SPDX-License-Identifier: MIT */
// Execute actual QGN1 instructions. Independent libm observes only;not a
// native owner,NoC producer or CPU inference fallback.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#ifndef PIONEER_Q38_GDN_GATED_NORM_MACHINE_ONLY
#define PIONEER_Q38_GDN_GATED_NORM_IMAGE 1
#define PIONEER_Q38_GDN_PREPARE_IMAGE 1
#include "../PioneerQ38GdnGatedNorm.h"
#endif
#include <array>
static double GatedSiluReference(double z){double e=exp(-fabs(z));return z*(z>=0?1/(1+e):e/(1+e));}
struct GatedNormMachine:ScalarMachine {
 std::array<uint32_t,32> header={};std::array<uint32_t,384> input={};
 unsigned writes=0,sealWrites=0;
 GatedNormMachine(const char* image,unsigned half,unsigned trial=0):ScalarMachine(image,128,0){
  memory.clear();header[0]=0x314e4751;header[1]=128;header[2]=64;header[3]=half;SetHeader();
  for(unsigned i=0;i<128;i++){
   float raw=float(sin(i*.137+(half/2)*.213)*3.7),norm=float(.7+cos(i*.071)*.2),z=float(sin(i*.091+(half/2)*.17)*12);
   if(trial==1)raw=0;
   if(trial==2){raw=i&1?-65504.f:65504.f;norm=raw;z=raw;}
   if(trial==3){raw=PioneerAttentionFloat(0x00800000);norm=i&1?-1.f:1.f;z=i&1?-4.f:4.f;}
   if(trial==4){raw=i&1?PioneerAttentionFloat(0x80000000):0;z=-0.f;}
   if(trial>=5){
    raw=i?0:1;norm=65504;const float edge[]={-80,-81,-84,-87,-88,-90,-100,-104,-106,-128,-65504,65504};
    assert(trial<17);z=edge[trial-5];
   }
   input[i]=PioneerAttentionBits(raw);input[128+i]=PioneerAttentionBits(norm);input[256+i]=PioneerAttentionBits(z);
  }
  SetInputs();for(unsigned i=0;i<64;i++)memory[0x8ea00+i*4]=0xdeadbeef;
  for(unsigned i=0;i<4;i++)memory[0x74000+i*4]=0xdeadbeef;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned at:{0x7fffcU,0x80600U,0x8e9fcU,0x8eb00U,0x745fcU,0x74680U,0x73ffcU,0x74010U})memory[at]=0xdecafbad;
 }
 void SetHeader(){for(unsigned i=0;i<32;i++)memory[0x74600+i*4]=header[i];}
 void SetInputs(){for(unsigned i=0;i<input.size();i++)memory[0x80000+i*4]=input[i];}
 uint32_t Load(uint32_t at)override{
  assert(!(at&3));if(Range(at,0x64000,unsigned(code.size())))return l1[Index(at)];
  assert(Range(at,0x6e000,8192) || Range(at,0x74600,128) || Range(at,0x80000,1536)
   || Range(at,0x74000,16) || Range(at,0x8ea00,256) || Range(at,0x17ffc0,12));
  assert(memory.count(at));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3));assert(Range(at,0x6e000,8192) || Range(at,0x74000,16) || Range(at,0x8ea00,256) || Range(at,0x17ffc0,12));
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,unsigned(at));
  if(Range(at,0x74000,16))sealWrites++;if(Range(at,0x8ea00,256))writes++;
  memory[at]=value; // no code/input/NoC/GDDR/CSR/PCI/reset writes
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned at:{0x7fffcU,0x80600U,0x8e9fcU,0x8eb00U,0x745fcU,0x74680U,0x73ffcU,0x74010U})assert(memory.at(at)==0xdecafbad);
  unsigned before=writes,seals=sealWrites;for(unsigned i=0;i<32;i++)StepScalar();assert(writes==before && sealWrites==seals);
  for(unsigned i=0;i<input.size();i++)assert(memory.at(0x80000+i*4)==input[i]);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x464e4751 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==0x314e4751 && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==64;
 }
 void Verify(){
  assert(writes==64 && sealWrites==5 && memory.at(0x74000)==0x314e4751
   && memory.at(0x74004)==128 && memory.at(0x74008)==64 && memory.at(0x7400c)==header[3]);
  float sum=0;for(unsigned i=0;i<128;i++){float raw=PioneerAttentionFloat(input[i]);sum+=raw*raw;}
  float inverse=1.f/sqrtf(sum*.0078125f+1e-6f);
  for(unsigned i=0;i<64;i++){
   unsigned at=(header[3]&1)*64+i;float raw=PioneerAttentionFloat(input[at]),norm=PioneerAttentionFloat(input[128+at]),z=PioneerAttentionFloat(input[256+at]);
   float weighted=(raw*inverse)*norm,canonical=weighted*float(GatedSiluReference(z));
   double reference=double(weighted)*GatedSiluReference(z),actual=PioneerAttentionFloat(memory.at(0x8ea00+i*4));
   assert(isfinite(actual) && fabs(actual-canonical)<=3e-6*(1+fabs(double(canonical))));
   if(fabs(reference)>=double(PioneerAttentionFloat(0x00800000))){
    if(actual==0 || fabs(actual-reference)>3e-6*fabs(reference)){
     fprintf(stderr,"QGN bad half%u at%u raw%.9g norm%.9g z%.9g weighted%.9g actual%.12g expected%.12g\n",header[3],at,raw,norm,z,weighted,actual,reference);assert(false);
    }
   }else assert(actual==0); // only true final underflow FTZ in these bounded fixtures
  }
 }
 void RefuseValue(unsigned at,uint32_t bits){input[at]=bits;SetInputs();assert(!Run() && !writes && sealWrites==1 && !memory.at(0x74000));}
};
#ifndef PIONEER_Q38_GDN_GATED_NORM_MACHINE_ONLY
static void HostBounds(){
 double maxRelative=0;unsigned points=0;
 for(unsigned e=0;e<22;e++)for(int step=-512;step<=512;step++){
  int exponent=e==21?20:-126+int(e)*7;
  float weighted=ldexpf(1.f,exponent),z=float(step)*.25f;
  for(unsigned sign=0;sign<2;sign++){
   double expected=double(sign?-weighted:weighted)*GatedSiluReference(z),got=PioneerQ38GdnWeightedSilu(sign?-weighted:weighted,z);
   assert(isfinite(got));
   if(fabs(expected)>=double(PioneerAttentionFloat(0x00800000))){
    double relative=fabs(got-expected)/fabs(expected);maxRelative=std::max(relative,maxRelative);assert(got!=0 && relative<3e-6);
   }else assert(got==0);
   points++;
  }
 }
 for(float weighted:{0.f,-0.f})for(float z:{0.f,-0.f,1.f,-1.f,65504.f,-65504.f}){
  float expected=float(double(weighted)*GatedSiluReference(z));
  assert(PioneerAttentionBits(PioneerQ38GdnWeightedSilu(weighted,z))==PioneerAttentionBits(expected));
 }
 for(uint32_t bits:{1U,0x807fffffU,0x49800001U,0x7f800000U,0x7fc00000U})
  assert(!isfinite(PioneerQ38GdnWeightedSilu(PioneerAttentionFloat(bits),1)));
 float raw[128]={},norm[128],z[128],output[64];for(unsigned i=0;i<128;i++)norm[i]=z[i]=1;
 for(unsigned kind=0;kind<11;kind++){
  const float *a=raw,*b=norm,*c=z;float* out=output;unsigned half=0;
  if(kind==0)a=nullptr;if(kind==1)b=nullptr;if(kind==2)c=nullptr;if(kind==3)out=nullptr;
  if(kind==4)out=raw;if(kind==5)out=norm;if(kind==6)out=z;
  if(kind==7)half=2;if(kind==8)a=reinterpret_cast<const float*>(UINTPTR_MAX-3);
  if(kind==9)out=reinterpret_cast<float*>(uintptr_t(1));if(kind==10)c=reinterpret_cast<const float*>(uintptr_t(1));
  assert(!PioneerQ38GdnGatedNormHalf(a,b,c,half,out));
 }
 printf("PASS host weightedSiLU%ugrid/strict NORMAL final relative max%.9g/12signedzeros/5invalidweighted/11pointer-domain refusals;NO native/Work/fallback\n",points,maxRelative);
}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);HostBounds();unsigned positives=0,negative=0,maxSteps=0,maxStack=0;
 for(unsigned half=0;half<96;half++){
  GatedNormMachine machine(argv[1],half);assert(machine.Run());machine.Verify();positives++;
  maxSteps=std::max(maxSteps,machine.steps);maxStack=std::max(maxStack,0x70000-machine.minStack);
 }
 for(unsigned trial=1;trial<17;trial++)for(unsigned half:{0U,95U}){
  GatedNormMachine machine(argv[1],half,trial);assert(machine.Run());machine.Verify();positives++;
  maxSteps=std::max(maxSteps,machine.steps);maxStack=std::max(maxStack,0x70000-machine.minStack);
 }
 for(unsigned i=0;i<32;i++)for(unsigned bit=0;bit<32;bit++){
  GatedNormMachine machine(argv[1],0);machine.header[i]^=1U<<bit;
  if(i==3 && machine.header[i]<96)continue;machine.SetHeader();assert(!machine.Run() && !machine.writes && !machine.sealWrites);negative++;
 }
 for(unsigned at:{0U,63U,64U,127U,128U,191U,192U,255U,256U,319U,320U,383U})
  for(uint32_t bad:{1U,0x807fffffU,0x7f800000U,0xff800000U,0x7fc00000U,0x47800000U}){
   GatedNormMachine machine(argv[1],0);machine.RefuseValue(at,bad);negative++;
  }
 printf("PASS ACTUAL QGN1 RV32IMF128wide RMS/learned norm/SiLU BOTH64halves positives%u/refusals%u/maxSteps%u/maxStack%u;independent libm/canonical bounds+strict NORMAL fused-final edges;NO NoC/native-owner/original-GGUF/card/speed claim\n",positives,negative,maxSteps,maxStack);
}
#endif
