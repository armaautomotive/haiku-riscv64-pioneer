/* SPDX-License-Identifier: MIT */
// DISTINCT QGW1 whole-model/global-output front end. Fresh cohort required.
// Reuses returning arithmetic ONLY; this is not an old live-cohort update.
#ifdef PIONEER_Q38_PRECISE_WORKER_IMAGE
#include "PioneerQ38PreciseWorkerImageRole.h"
#else
#include "PioneerQ38GlobalWorkerImageRole.h"
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen38Panel.h"
#include "PioneerQ38GlobalWorkerLayout.h"
#ifdef PIONEER_Q38_PRECISE_WORKER_IMAGE
#include "PioneerQ38PreciseWorkerLayout.h"
extern "C" void q38_precise_persistent_activation(const uint32_t*,const uint32_t*,unsigned,unsigned);
#endif
#include "PioneerQ38DensePartitionLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPersistentShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvBatchShape.h"
extern "C" unsigned block_peer_wait(unsigned);
extern "C" void block_peer_stopped() __attribute__((noreturn));
extern "C" void block_fault_park() __attribute__((noreturn));
extern "C" unsigned q38_compute_once();
extern "C" void q38_mixed_seal();
extern "C" void q38_ffn_persistent_matrix_begin(const uint32_t*,unsigned);
extern "C" void q38_ffn_persistent_activation(const uint32_t*,const uint32_t*,unsigned,unsigned);
extern "C" void q38_resident_persistent_conv(const uint32_t*,const uint32_t*,unsigned,unsigned);
extern "C" void q38_resident_persistent_gdn(const uint32_t*,const uint32_t*,unsigned,unsigned);
extern "C" void q38_resident_persistent_gdn_output(const uint32_t*,const uint32_t*,unsigned,unsigned);
extern "C" void q38_mixed_dense_phase(const uint32_t*,unsigned,unsigned,float*,float*);
namespace GlobalWorker {
#ifdef PIONEER_Q38_PRECISE_WORKER_IMAGE
namespace M=PioneerQ38PreciseWorkerLayout;
#else
namespace M=PioneerQ38GlobalWorkerLayout;
#endif
namespace D=PioneerQ38DensePartitionLayout;
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Matrix(const uint32_t* j,const uint32_t* source,unsigned worker,unsigned self){
 if(j[0]!=0x31483851 || j[3] || j[4] || j[9]!=1 || !PioneerQ38PanelTileBytes(j[1])
  || j[2]!=PioneerQ38PanelTileBytes(j[1]) || (j[5]!=5120 && j[5]!=6144 && j[5]!=17408)
  || !j[7] || j[7]>16)block_fault_park();
 q38_ffn_persistent_matrix_begin(source,self);
 uint32_t before[8];for(unsigned i=0;i<8;i++)before[i]=*Words(0xffb20000+Counter(i));
 unsigned count=q38_compute_once(),blocks=j[5]>>(j[1]==8?5:8);
 unsigned reads=j[7]*blocks+(j[5]*2+16383)/16384+1;
 unsigned flits=j[7]*blocks*j[2]/64+(j[5]*2+((blocks*4+63)&~63U))/64;
 if(count!=j[7]*j[5]/16 || !Quiet() || *Words(0xffb20014)!=self)block_fault_park();
 auto* p=Words(0x83f00);
 for(unsigned i=0;i<8;i++){
  unsigned after=*Words(0xffb20000+Counter(i)),delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
  if(uint32_t(after-before[i])!=delta)block_fault_park();
  p[i]=before[i];p[8+i]=after;
 }
 p[16]=count;p[17]=j[12];p[18]=worker;p[19]=TT_Q38_FFN_PERSISTENT_MARKER;
 for(unsigned i=0;i<10;i++)p[20+i]=Words(0xffb20000)[i];
 p[30]=source[0];p[31]=self;for(unsigned i=0;i<16;i++)p[32+i]=source[i];
 for(unsigned i=48;i<64;i++)p[i]=0;
 Fence();
}
static void GlobalPartial(const uint32_t* j,const uint32_t* boot){
 // Read ALL freshly generated rows, never a host-supplied maximum. Only
 // finite logits; canonical +/-0 ties choose the smallest vocabulary row.
 unsigned count=j[23]*16,best=0,bits=Words(0x84000)[0];
 if((bits&0x7f800000)==0x7f800000)block_fault_park();
 for(unsigned i=1;i<count;i++){
  unsigned value=Words(0x84000)[i];if((value&0x7f800000)==0x7f800000)block_fault_park();
  if(M::Better(value,j[28]+i,bits,j[28]+best)){bits=value;best=i;}
 }
 q38_mixed_seal();if(!Quiet())block_fault_park();
 auto* p=Words(M::Partial);for(unsigned i=0;i<64;i++)p[i]=0;
 const unsigned h[]={0,j[1],64,j[6],j[3],j[27],j[28],count,j[28]+best,bits,1,j[7],248320,5120,14,1};
 for(unsigned i=0;i<16;i++)p[i]=h[i];
 for(unsigned i=0;i<8;i++){p[16+i]=*Words(0xffb20000+Counter(i));p[50+i]=boot[8+i];}
 for(unsigned i=0;i<10;i++)p[24+i]=Words(0xffb20000)[i];
 for(unsigned i=0;i<16;i++)p[34+i]=boot[16+i];
 Fence();p[0]=M::PartialMagic;Fence();q38_mixed_seal();
}
}
extern "C" __attribute__((noinline)) void q38_mixed_seal(){
 using namespace GlobalWorker;const M::PrivateSeal* s;
 asm volatile("mv %0,s11":"=r"(s));
 // s11 is reserved in EVERY linked capsule. It is never a caller's L1
 // descriptor pointer, a writable global, or recaptured between packets.
 if(reinterpret_cast<unsigned>(s)<0x6e000 || reinterpret_cast<unsigned>(s)>0x6fe00)block_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(M::Boot)[i]!=s->boot[i] || Words(M::Job)[i]!=s->job[i])block_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=s->boot[16+i])block_fault_park();
 if(*Words(M::Doorbell)!=s->job[1])block_fault_park();
}
extern "C" void q38_global_worker_main(){
 using namespace GlobalWorker;
 M::PrivateSeal seal;uint32_t dense[64],capsule[16],counters[8];float local[6],global[6];
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(M::Boot)[i];
 if(!M::BootShape(seal.boot) || *Words(M::Doorbell) || !Quiet())block_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=seal.boot[16+i])block_fault_park();
 const unsigned worker=seal.boot[3],self=seal.boot[4];
 if(*Words(0xffb20014)!=self)block_fault_park();
 for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
 if(counters[2]!=counters[6])block_fault_park();
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 for(unsigned i=0;i<16;i++)Words(M::Status)[i]=0;
 Words(M::Status)[3]=M::Magic;Fence();
 unsigned previous=0,densePhase=0,modelLayer=0,modelPosition=0,globalWave=0;
#ifdef PIONEER_Q38_PRECISE_WORKER_IMAGE
 bool preciseInputReady=false;
#endif
 const unsigned finalWaves=worker<=21?9U:8U;
 for(;;){
  unsigned request=block_peer_wait(previous);
  for(unsigned i=0;i<64;i++)seal.job[i]=Words(M::Job)[i];
  if(request!=seal.job[1] || !M::JobShape(seal.job,seal.boot,previous,densePhase))block_fault_park();
  q38_mixed_seal();if(!Quiet())block_fault_park();
  for(unsigned i=0;i<8;i++)if(*Words(0xffb20000+Counter(i))!=counters[i])block_fault_park();
  unsigned op=seal.job[2];const bool outputJob=op==M::GlobalMatrix;
#ifdef PIONEER_Q38_PRECISE_WORKER_IMAGE
  const bool downJob=op==1 && seal.job[17]==12 && seal.job[21]==17408;
  if((preciseInputReady && !downJob && op!=M::Stop) || (downJob && worker<=68 && !preciseInputReady))block_fault_park();
#endif
  // Necessary private ordering, NOT proof of all model arithmetic or a
  // native token/producer grant. Only actual returned down jobs advance it.
  if(op==M::Stop && seal.job[5]==64){
   if(modelLayer!=64 || globalWave!=finalWaves || seal.job[6]!=modelPosition)block_fault_park();
  }else if(op!=M::Stop){
   if(outputJob){
    if(modelLayer!=64 || globalWave!=seal.job[27] || seal.job[6]!=modelPosition)block_fault_park();
   }else{
    if(modelLayer==64){
     if(globalWave!=finalWaves || modelPosition>=8191 || op!=1 || seal.job[5] || seal.job[6]!=modelPosition+1)block_fault_park();
     modelLayer=0;modelPosition++;globalWave=0;
    }
    if(seal.job[5]!=modelLayer || seal.job[6]!=modelPosition)block_fault_park();
   }
  }
  Words(M::Status)[0]=request;Words(M::Status)[4]=op;Fence();
  if(op==M::Stop){
   q38_mixed_seal();Words(M::Status)[1]=request;Words(M::Status)[4]=2;Fence();block_peer_stopped();
  }
  if(op>=M::DenseScores && !outputJob){
   if(!densePhase)for(unsigned i=0;i<64;i++)dense[i]=Words(D::Header)[i];
   if(!D::Shape(dense) || dense[3]!=seal.job[5] || dense[4]!=seal.job[6] || dense[40]!=seal.job[7]
    || dense[15]!=worker || dense[14]!=self)block_fault_park();
   for(unsigned i=0;i<16;i++)if(dense[16+i]!=seal.boot[16+i])block_fault_park();
   for(unsigned i=0;i<8;i++)if(dense[32+i]!=seal.boot[8+i])block_fault_park();
   unsigned phase=op-M::DenseScores+1;
   const uint32_t j[]={D::Magic,phase,request,dense[40],worker,dense[3],dense[4],dense[5],dense[48],dense[49],0,0,0,0,0,0};
   for(unsigned i=0;i<16;i++)Words(D::Job)[i]=j[i];
   Words(D::Doorbell)[0]=request;Fence();
   q38_mixed_dense_phase(dense,phase,request,local,global);
   densePhase=phase==3?0:phase;
  }else{
   for(unsigned i=0;i<10;i++)capsule[i]=seal.job[16+i];
   capsule[10]=TT_Q38_FFN_PERSISTENT_MARKER;capsule[11]=worker;capsule[12]=request;
   capsule[13]=outputJob?1U:op;capsule[14]=seal.job[26];capsule[15]=0;
   if(op>=3 && !outputJob){
    const auto* d=Words(op==7?unsigned(TT_Q38_CONV_BATCH_TABLE):op==8?0x74a00U:0x74200U);
    if(d[op==7 || op==8?5:42]!=seal.job[5] || d[40]!=seal.job[6] || d[6]!=seal.job[7]
     || capsule[3]!=seal.job[7])block_fault_park();
    for(unsigned i=0;i<8;i++)if(d[32+i]!=seal.boot[8+i])block_fault_park();
#ifdef PIONEER_Q38_PRECISE_WORKER_IMAGE
    if(op==3 && d[41]!=seal.job[27])block_fault_park();
#endif
   }
   for(unsigned i=0;i<16;i++)Words(0x74000)[i]=capsule[i];
   for(unsigned i=0;i<8;i++)Words(0x74040)[i]=i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?worker:i==2?self:i==3?seal.boot[16]:0;
   *Words(0x74100)=request;Fence();
   if(op==1 || outputJob)Matrix(capsule,seal.boot+16,worker,self);
#ifdef PIONEER_Q38_PRECISE_WORKER_IMAGE
   else if(op==3)q38_precise_persistent_activation(capsule,seal.boot+16,worker,self);
#endif
   else if(op==6)q38_resident_persistent_gdn_output(capsule,seal.boot+16,worker,self);
   else if(op==7)q38_resident_persistent_conv(capsule,seal.boot+16,worker,self);
   else if(op==8)q38_resident_persistent_gdn(capsule,seal.boot+16,worker,self);
   else q38_ffn_persistent_activation(capsule,seal.boot+16,worker,self);
   for(unsigned i=0;i<16;i++)if(Words(0x74000)[i]!=capsule[i])block_fault_park();
  }
  q38_mixed_seal();if(!Quiet())block_fault_park();
  for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
  if(counters[2]!=counters[6])block_fault_park();
#ifdef PIONEER_Q38_PRECISE_WORKER_IMAGE
  if(op==3)preciseInputReady=true;
  else if(downJob)preciseInputReady=false;
#endif
  if(outputJob){GlobalPartial(seal.job,seal.boot);globalWave++;}
  else if(op==1 && capsule[1]==12 && capsule[5]==17408)modelLayer++;
  previous=request;Words(M::Status)[5]=densePhase;Words(M::Status)[6]=seal.job[5];Words(M::Status)[7]=seal.job[6];
  Words(M::Status)[4]=0;Fence();Words(M::Status)[1]=previous;Fence();
 }
}
