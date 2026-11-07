/* SPDX-License-Identifier: MIT */
// Actual linked QGW7 dispatcher and QDM2 capsule, same simulated physical
// matrix state and L1 across BFP4 -> dense -> BFP4. No hardware/Work claim.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#include "pioneer_q38_hybrid_worker_image_test.cpp"
#include "../PioneerAttentionMath.h"
#include "../PioneerQ38DensePartitionLayout.h"
namespace Dense=PioneerQ38DensePartitionLayout;

struct TransitionImage:HybridImage {
 bool denseMode=false;uint32_t fp[32]={};unsigned matrixBefore=0;
 TransitionImage(const char* worker,const char* leaf):HybridImage(worker,leaf,73,17408){
  native.expectedWeight=0xa5000;native.alternateWeight=0xa6000;
  native.expectedInput=0xa6000;native.alternateInput=0xa7000;native.thirdInput=0xa7400;
 }
 static bool DenseL1(unsigned a){return Range(a,0x74600,0x600) || Range(a,0x80000,0x27600);}
 void WordWrite(unsigned a,unsigned value){assert(!(a&3) && a+4<=native.l1.size());memcpy(native.l1.data()+a,&value,4);}
 unsigned WordRead(unsigned a){assert(!(a&3) && a+4<=native.l1.size());unsigned value;memcpy(&value,native.l1.data()+a,4);return value;}
 uint32_t Load(uint32_t a)override{
  if(denseMode && DenseL1(a))return WordRead(a);
  if(denseMode && a==0xffe000fc)return gpr63;
  if(denseMode && Range(a,0xffef0000,224*4))return native.config[(a-0xffef0000)/4];
  if(denseMode && (Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64))){
   const unsigned row=(a-0xffbd8000)/64,col=((a-0xffbd8000)%64)/4;
   assert(native.valid[row] && !native.mathPending);return NativeBfpMachine::Bits(native.destination[row][col]);
  }
  return HybridImage::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(denseMode && DenseL1(a)){
   assert(PioneerQ38HybridTokenL1::SnapshotContains(boot[3],a));
   WordWrite(a,value);return;
  }
  if(denseMode && a==0xffe000fc){assert(!value);gpr63=0;return;}
  if(denseMode && Range(a,0xffef0000,224*4)){native.Config((a-0xffef0000)/4,value);return;}
  if(denseMode && a==0xffe40000){
   if(value==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
   else if(value==0x4500017e){assert(awaiting && !gpr63);native.Drain();gpr63=1;awaiting=false;}
   else native.NativeBfpMachine::Op(value);
   return;
  }
  HybridImage::Store(a,value);
 }
 void StepCombined(){
  const uint32_t instruction=Load(pc),op=instruction&127;
  if((instruction&3)==3 && op==0x23 && ((instruction>>12)&7)==0){
   fprintf(stderr,"unmodeled byte store %08x at %08x return=%08x destination=%08x size=%u\n",instruction,pc,regs[1],regs[10],regs[12]);assert(false);
  }
  if((instruction&3)!=3 || (op!=7 && op!=0x27 && op!=0x53)){StepWorker();return;}
  assert(++steps<StepLimit());const unsigned d=(instruction>>7)&31,f=(instruction>>12)&7,s=(instruction>>15)&31,t=(instruction>>20)&31,hi=instruction>>25;
  if(op==7){assert(f==2);fp[d]=Load(regs[s]+Sign(instruction>>20,12));}
  else if(op==0x27){assert(f==2);Store(regs[s]+Sign((hi<<5)|d,12),fp[t]);}
  else{
   const float a=PioneerAttentionFloat(fp[s]),b=PioneerAttentionFloat(fp[t]);
   switch(hi){
    case 0:fp[d]=PioneerAttentionBits(a+b);break;case 4:fp[d]=PioneerAttentionBits(a-b);break;
    case 8:fp[d]=PioneerAttentionBits(a*b);break;
    case 0x10:assert(f<=2);fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000));break;
    case 0x50:assert(f<=2);if(d)regs[d]=f==0?a<=b:f==1?a<b:a==b;break;
    case 0x68:assert(t<=1);fp[d]=PioneerAttentionBits(t?float(regs[s]):float(int32_t(regs[s])));break;
    case 0x70:if(d)regs[d]=fp[s];break;case 0x78:fp[d]=regs[s];break;
    default:fprintf(stderr,"unmodeled QGW7 floating instruction %08x at %08x\n",instruction,pc);assert(false);
   }
  }
  regs[0]=0;pc+=4;
 }
 bool FinishCombined(unsigned seq){
  while(pc!=faultPC && !(pc==waitPC && Get(HW::Status+4)==seq))StepCombined();
  return pc!=faultPC;
 }
 void RearmTestAccounting(){
  assert(native.Clean() && transfer.pending.empty());
  // These are fixture counters only, not simulated configuration, source
  // banks, Dst, NoC counters, worker registers, code or L1 contents.
  transfer.issues=transfer.completions=0;transfer.band=0;
  native.clears=0;native.unpacks[0]=native.unpacks[1]=0;native.matrixOps=0;stores=0;
 }
 void StageDense(unsigned phase,unsigned sequence,bool stageJob=true){
  assert(phase>=1 && phase<=3);denseMode=true;
  if(phase==1){
   unsigned d[64]={};const unsigned first[]={Dense::Magic,1,64,3,0,3,0,1,0x10000000,0x20000000,0,1,Dense::Route(0),Dense::Route(1),boot[4],73};
   for(unsigned i=0;i<16;i++)d[i]=first[i];for(unsigned i=0;i<16;i++)d[16+i]=source[i];
   for(unsigned i=0;i<8;i++)d[32+i]=boot[8+i];
   d[40]=2025;d[41]=256;d[42]=0x3d800000;d[43]=32;d[44]=8192;d[45]=16384;
   d[48]=0;d[49]=1;d[50]=3;d[51]=6;d[52]=24;d[53]=384;d[54]=1536;d[55]=Dense::Magic;
   assert(Dense::Shape(d));for(unsigned i=0;i<64;i++)WordWrite(Dense::Header+i*4,d[i]);
   for(unsigned i=0;i<1536;i++)WordWrite(Dense::HalfQuery+i*4,PioneerAttentionBits(float(int(i%7)-3)*.125f));
   for(unsigned i=0;i<128;i++){
    const uint16_t klo=PioneerAttentionToHalf(float(int((i*2)%11)-5)*.125f);
    const uint16_t khi=PioneerAttentionToHalf(float(int((i*2+1)%11)-5)*.125f);
    WordWrite(Dense::NewK+i*4,klo|(unsigned(khi)<<16));
    const uint16_t lo=PioneerAttentionToHalf(float(int((i*2)%17)-8)*.0625f);
    const uint16_t hi=PioneerAttentionToHalf(float(int((i*2+1)%17)-8)*.0625f);
    WordWrite(Dense::NewV+i*4,lo|(unsigned(hi)<<16));
   }
   matrixBefore=native.matrixOps;
  }
  for(unsigned i=0;i<16;i++){
   const unsigned value=(phase>=2 && i<6)?WordRead(Dense::Local+(4+i)*4):
    (phase==3 && i>=8 && i<14)?PioneerAttentionBits(1.f):0;
   WordWrite(Dense::Parameters+i*4,value);
  }
  if(!stageJob)return;
  unsigned j[64]={};j[0]=HW::Magic;j[1]=sequence;j[2]=HW::DenseScores+phase-1;j[3]=73;j[4]=boot[4];j[5]=3;j[6]=0;j[7]=2025;
  for(unsigned i=0;i<8;i++)j[8+i]=boot[8+i];
  assert(HW::JobShape(j,boot.data(),sequence-1,phase-1));
  for(unsigned i=0;i<64;i++)Put(HW::Job+i*4,j[i]);Put(HW::Doorbell,sequence);
 }
};
#ifndef PIONEER_Q38_HYBRID_MATRIX_TRANSITION_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);TransitionImage image(argv[1],argv[2]);image.Boot();
 for(unsigned layer=0;layer<3;layer++){
  if(layer)image.RearmTestAccounting();
  image.Stage(layer,layer+1,4);assert(image.FinishCombined(layer+1));
 }
 assert(image.native.Clean());
 for(unsigned phase=1;phase<=3;phase++){
  image.StageDense(phase,3+phase);assert(image.FinishCombined(3+phase));
  assert(image.WordRead(Dense::Summary)==Dense::Magic && image.WordRead(Dense::Local)==phase);
  if(phase==1)for(unsigned h=0;h<6;h++){
   float expected=0;
   for(unsigned d=0;d<256;d++)expected+=float(int((h*256+d)%7)-3)*.125f*float(int(d%11)-5)*.125f;
   expected*=.0625f;
   assert(PioneerAttentionFloat(image.WordRead(Dense::Scores+h*Dense::Stride*4))==expected);
  }
 }
 assert(image.native.matrixOps-image.matrixBefore==6*(128+4*8*16) && image.native.Clean());
 for(unsigned h=0;h<6;h++)for(unsigned d=0;d<256;d++){
  const float expected=float(int(d%17)-8)*.0625f;
  const float high=PioneerAttentionFloat(image.WordRead(Dense::Partial+(h*512+d*2)*4));
  const float low=PioneerAttentionFloat(image.WordRead(Dense::Partial+(h*512+d*2+1)*4));
  assert(high+low==expected);
 }
 image.denseMode=false;image.RearmTestAccounting();
 image.Stage(3,7,4);assert(image.FinishCombined(7) && image.native.Clean());
 const auto* golden=PioneerNativeBfp4BandGolden(17408);assert(golden);
 for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++)
  assert(image.Get(W::Output+(band*32+row)*4)==golden[row]);
 puts("PASS actual QGW7 BFP4 down layers 0/1/2 -> QDM2 attention phases 1/2/3 -> BFP4 down layer 3; same simulated registers/banks/L1/NoC history, synthetic data/controller; NO HARDWARE OR SPEED");
}
#endif
