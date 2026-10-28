/* SPDX-License-Identifier: MIT */
// Returning device attention/FFN arithmetic. No host loop or completion hint
// is substituted for a real predecessor, persistent KV or native lifetime.
#include "PioneerQ38LayerReturnComponentRole.h"
#ifndef PIONEER_Q38_LAYER_COMPONENT_DENSE
#error Dense returning translation unit requires its own family
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerAttentionMath.h"
#include "PioneerQ38LayerReturnComponentLayout.h"
#define q38_mixed_root_seal q38_layer_return_dense_seal
#define memset q38_layer_return_dense_zero
#include "blackhole_qwen38_mixed_live_block_root.cpp"
#undef memset
#undef q38_mixed_root_seal
extern "C" __attribute__((noinline)) unsigned q38_return_dense_layer(){
 using namespace DensePartitionRoot;namespace A=PioneerQ38LayerReturnComponentLayout;
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
