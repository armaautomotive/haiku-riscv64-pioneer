/* SPDX-License-Identifier: MIT */
// Returning device attention/FFN arithmetic. No host loop or completion hint
// is substituted for a real predecessor, persistent KV or native lifetime.
#ifdef PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE
#include "PioneerQ38GlobalChainComponentRole.h"
#else
#include "PioneerQ38LayerReturnComponentRole.h"
#endif
#ifndef PIONEER_Q38_LAYER_COMPONENT_DENSE
#error Dense returning translation unit requires its own family
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerAttentionMath.h"
#ifdef PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE
#include "PioneerQ38GlobalChainLayout.h"
#endif
#include "PioneerQ38LayerReturnComponentLayout.h"
#define q38_mixed_root_seal q38_layer_return_dense_seal
#define q38_mixed_root_packet_seal q38_layer_return_dense_packet_seal
#define q38_mixed_root_packet_frame q38_layer_return_dense_packet_frame
#define memset q38_layer_return_dense_zero
#ifdef PIONEER_Q38_LAYER_COMPONENT_DENSE_STOP
#define mixed_live_block_stopped q38_layer_chain_stopped
#endif
#include "blackhole_qwen38_mixed_live_block_root.cpp"
#ifdef PIONEER_Q38_LAYER_COMPONENT_DENSE_STOP
#undef mixed_live_block_stopped
#endif
#undef memset
#undef q38_mixed_root_seal
#undef q38_mixed_root_packet_seal
#undef q38_mixed_root_packet_frame
extern "C" __attribute__((noinline)) unsigned q38_return_dense_layer(){
 using namespace DensePartitionRoot;
#ifdef PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE
 namespace A=PioneerQ38GlobalChainLayout;
#else
 namespace A=PioneerQ38LayerReturnComponentLayout;
#endif
 L::PrivateSeal seal;unsigned parent;
 asm volatile("mv %0,s11":"=r"(parent));
 if(!A::PrivateFrames(parent,reinterpret_cast<unsigned>(&seal)))scalar_fault_park();
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(L::Boot)[i];
 for(unsigned i=0;i<128;i++)seal.outer[i]=Words(L::Outer)[i];
 if(!A::Frame(seal.boot,seal.outer,true) || *Words(L::Doorbell)!=seal.outer[48] || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<64;i++)seal.frame[i]=Words(L::Header)[i]=0;
 seal.phase=1;Words(L::Status)[0]=seal.outer[48];Words(L::Status)[4]=1;Words(L::Status)[5]=1;
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 LiveBlock::Input(&seal);LiveBlock::Compute(&seal);
 Projection::InputsSeal(seal.frame);for(unsigned w=0;w<119;w++)FrameSeal(w);
 q38_layer_return_dense_seal();if(!Quiet())scalar_fault_park();
 Words(L::Status)[4]=0;Words(L::Status)[6]=seal.outer[3];Words(L::Status)[7]=seal.outer[4];Fence();Words(L::Status)[1]=seal.outer[48];Fence();
 asm volatile("mv s11,%0"::"r"(parent):"s11","memory");
 return A::Hidden; // not a whole-token KV/state commit or native cleanup seal
}
#ifdef PIONEER_Q38_LAYER_COMPONENT_DENSE_STOP
// No caller-supplied completed/STOP/clean bit. This entry performs the actual
// FINAL dense arithmetic in its SAME private lifetime before issuing STOP.
extern "C" __attribute__((noinline,noreturn)) void q38_run_final_dense_and_stop(){
 using namespace DensePartitionRoot;namespace A=PioneerQ38LayerReturnComponentLayout;
 L::PrivateSeal seal;unsigned parent;asm volatile("mv %0,s11":"=r"(parent));
 if(!A::PrivateFrames(parent,reinterpret_cast<unsigned>(&seal)))scalar_fault_park();
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(L::Boot)[i];
 for(unsigned i=0;i<128;i++)seal.outer[i]=Words(L::Outer)[i];
 const auto* o=seal.outer;const unsigned layers=PIONEER_Q38_LAYER_COMPONENT_CHAIN_LAYERS;
 if(!A::Frame(seal.boot,o,true) || o[3]!=layers-1 || o[48]!=layers || o[4] || o[52]
  || o[6]!=1001+(layers-1)*16 || o[73]!=o[7] || o[74]!=o[9]+L::FinalOffset
  || *Words(L::Doorbell)!=o[48] || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<64;i++)seal.frame[i]=Words(L::Header)[i]=0;
 seal.phase=1;Words(L::Status)[0]=o[48];Words(L::Status)[4]=1;Words(L::Status)[5]=1;
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 LiveBlock::Input(&seal);LiveBlock::Compute(&seal);
 Projection::InputsSeal(seal.frame);for(unsigned w=0;w<119;w++)FrameSeal(w);
 q38_layer_return_dense_seal();if(!Quiet() || seal.frame[43]!=4 || seal.frame[48]!=layers)scalar_fault_park();
 // Only the just-completed private layer creates the following STOP frame.
 for(unsigned i=0;i<128;i++)seal.outer[i]=0;
 seal.outer[0]=L::Magic;seal.outer[1]=1;seal.outer[2]=128;seal.outer[5]=129;
 seal.outer[48]=layers+1;seal.outer[49]=119;seal.outer[72]=L::Stop;
 for(unsigned i=0;i<16;i++)seal.outer[16+i]=seal.boot[16+i];
 for(unsigned i=0;i<8;i++)seal.outer[32+i]=seal.boot[8+i];
 if(!L::StopShape(seal.outer))scalar_fault_park();
 for(unsigned i=0;i<128;i++)Words(L::Outer)[i]=seal.outer[i];
 Fence();*Words(L::Doorbell)=layers+1;Fence();
 LiveBlock::Stop(&seal,layers); // actual ALL119 closed-chain audit/STOP/terminal proof; NEVER returns
 scalar_fault_park(); // fail closed even if a future STOP implementation unexpectedly returns
}
#endif
