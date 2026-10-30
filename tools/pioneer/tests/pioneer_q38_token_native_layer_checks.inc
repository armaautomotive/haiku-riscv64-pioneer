/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE != 1
#error Explicit offline native layer test required
#endif
#include "pioneer_q38_projection_input_reference.h"
static void CheckNativeLayerDispatch(Fixture& f){
 namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;namespace W=PioneerQ38SelectedGlobalWorker;
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && !state->Unsafe() && !io.embeddingFault);
 const unsigned hostPackets=io.runtimePackets,resets=io.resets.size();cpu.ResumeLayer();
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  unsigned observedBucket=cpu.steps/2000000;
  for(unsigned round=0;round<3000000 && !cpu.layerHeld && cpu.pc!=0x60000 && cpu.pc!=0x60004;round++){
   io.Pause();if(cpu.steps/2000000!=observedBucket){observedBucket=cpu.steps/2000000;
    printf("OBSERVED native ROOT layer0 pre-dispatch/steps%u pc%x packets%u raw%u peerR%u peerW%u phase%x;NOT matrix/layer/token completion\n",
     cpu.steps,cpu.pc,cpu.layerPackets,cpu.layerRawReads,cpu.layerPeerReads,cpu.layerPeerWrites,io.Raw(0,R::Status+20));
   }
  }
  if(!cpu.layerHeld)fprintf(stderr,"LAYER_DISPATCH_DIAGNOSTIC pc%x steps%u packets%u raw%u peerR%u peerW%u status%x phase%x;NO completion supplied\n",
   cpu.pc,cpu.steps,cpu.layerPackets,cpu.layerRawReads,cpu.layerPeerReads,cpu.layerPeerWrites,io.Raw(0,R::Status+8),io.Raw(0,R::Status+20));
  assert(cpu.layerHeld && cpu.dispatchWorker && io.Core(0).pending && cpu.submitted==cpu.completed+1
   && cpu.pc!=0x60000 && cpu.pc!=0x60004 && cpu.layerRawReads==2 && cpu.layerPeerReads && cpu.layerPeerWrites
   && !state->Unsafe() && !state->Closed());
  PioneerQ38TokenExecutionResult observed;assert(ObserveQ38TokenNativeRoot(c,observed) && observed.acknowledged && !observed.unsafe);
 }
 // OBSERVERS ONLY,after actual firmware normalization+quantization+job
 // submission. Neither reference enters the shared byte/processor endpoints.
 std::array<uint32,128> outer;for(unsigned i=0;i<128;i++)outer[i]=io.Raw(0,R::Outer+i*4);
 assert(outer[3]==0 && outer[4]==0 && outer[7]==0 && outer[9]==io.runtimeWorkspace);
 std::array<uint8_t,20480> packed;assert(cpu.original->Read(PioneerQ38ProjectionRoute(outer[64]),outer[65],packed.data(),16384)
  && cpu.original->Read(PioneerQ38ProjectionRoute(outer[64]),outer[65]+16384,packed.data()+16384,4096));
 double sum=0;for(unsigned i=0;i<5120;i++){
  const uint32 input=io.Raw(0,PioneerQ38GlobalFeedbackLayout::Local+i*4);
  assert(io.Raw(0,R::Original+i*4)==input && io.Raw(0,R::OriginalCopy+i*4)==input);
  float value=Float(input);sum+=double(value*value);
 }
 const float scale=1.f/sqrtf(float(sum/5120.0)+1e-6f);
 for(unsigned i=0;i<5120;i++){
  const uint32 bits=uint32(packed[i*4])|uint32(packed[i*4+1])<<8|uint32(packed[i*4+2])<<16|uint32(packed[i*4+3])<<24;
  assert(io.Raw(0,R::Weight+i*4)==bits);
  const float expected=(Float(io.Raw(0,R::OriginalCopy+i*4))*scale)*Float(bits);
  assert(io.Raw(0,R::Input+i*4)==Bits(expected));
 }
 const unsigned type=io.Raw(0,R::Header+15*4),inputs=io.Raw(0,R::Header+14*4);
 assert(inputs==5120 && (type==8 || type==12 || type==14) && io.Raw(0,R::Header+43*4)==10);
 (void)&PioneerQ38ProjectionQuantize;
 for(unsigned at=0;at<inputs;at+=256){
  float normalized[256];PioneerQ38ProjectionInput expected;
  for(unsigned i=0;i<256;i++)normalized[i]=Float(io.Raw(0,R::Input+(at+i)*4));
  ProjectionInputReference(type,normalized,expected);
  for(unsigned i=0;i<128;i++)assert(io.Raw(0,R::Native+(at/2+i)*4)==expected.operand[i]);
  const unsigned count=type==8?8:1,first=at>>(type==8?5:8);
  for(unsigned i=0;i<count;i++)assert(io.Raw(0,R::Scales+(first+i)*4)==expected.scale[i]);
 }
 const unsigned worker=cpu.dispatchWorker;std::array<uint32,64> boot,job;
 for(unsigned i=0;i<64;i++){boot[i]=io.Raw(worker,W::Boot+i*4);job[i]=io.Raw(worker,W::Job+i*4);}
 assert(W::JobShape(job.data(),boot.data(),0,0) && job[2]==1 && !job[5] && !job[6] && !io.Raw(worker,W::Doorbell));
 for(const auto& p:io.bootstrapPeer)assert(p->Idle() && !p->matrixInstructions && !p->floatOps && !p->requests && !p->responses);
 assert(io.runtimePackets==hostPackets && io.resets.size()==resets && !io.pciWrites && !io.HostReadMock::bmeWrites && !state->Unsafe());
 printf("PASS SAME native full851+1010 actual zero jobs+119 VIRGIN boot histories/unchanged full scalar ISA from first ROOT instruction/original27B embedding -> genuine layer0 metadata+RAW norm+ALL5120 bit-exact RMS+ALL native operands/scales vs independent OBSERVERS ONLY/actual valid worker%u matrix job delivered/doorbell packet PENDING/ROOTsteps%u layerPackets%u peerR%u peerW%u;NO CPU products,worker completion or private-history transplant;NOT matrix/layer/token/STOP/END/physical/Work/TPS\n",
  worker,cpu.steps,cpu.layerPackets,cpu.layerPeerReads,cpu.layerPeerWrites);
}
