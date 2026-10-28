/* SPDX-License-Identifier: MIT */
// Returning device arithmetic only. No boot, host wait, STOP, native owner or
// whole-token publication. A future scheduler must stage exact canonical DATA
// and prove the preceding embedding/layer and current state independently.
#include "PioneerQ38LayerReturnComponentRole.h"
#ifndef PIONEER_Q38_LAYER_COMPONENT_RECURRENT
#error Recurrent returning translation unit requires its own family
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerAttentionMath.h"
#include "PioneerQ38LayerReturnComponentLayout.h"
#include "PioneerQ38RecurrentHistoryLayout.h"
// Separate TUs retain their own genuine private seal TYPES. Do not cast a
// recurrent private owner into a dense one or transplant an old native owner.
#define q38_mixed_root_seal q38_layer_return_recurrent_seal
#define q38_mixed_recurrent_frame q38_layer_return_recurrent_frame
#define memset q38_layer_return_recurrent_zero
#include "blackhole_qwen38_mixed_recurrent_block_root.cpp"
#undef memset
#undef q38_mixed_recurrent_frame
#undef q38_mixed_root_seal
// GCC also lowers aggregate zero-initialization to the ordinary C symbol.
// One implementation serves both components; it retains the zero/alignment
// checks of the existing device helper, not a host runtime or allocator.
extern "C" void* memset(void* target,int value,__SIZE_TYPE__ bytes){
 return q38_layer_return_recurrent_zero(target,value,bytes);
}
extern "C" __attribute__((noinline)) unsigned q38_return_recurrent_layer(){
 using namespace RecurrentBlock;namespace A=PioneerQ38LayerReturnComponentLayout;
 R::PrivateSeal seal;unsigned parent;
 asm volatile("mv %0,s11":"=r"(parent));
 if(!A::PrivateFrames(parent,reinterpret_cast<unsigned>(&seal)))scalar_fault_park();
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(R::Boot)[i];
 for(unsigned i=0;i<128;i++)seal.outer[i]=Words(R::Outer)[i];
 if(!A::Frame(seal.boot,seal.outer,false) || *Words(R::Doorbell)!=seal.outer[48] || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<64;i++)seal.frame[i]=Words(R::Header)[i]=0;
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 const auto* o=seal.outer;Words(R::Status)[0]=o[48];Words(R::Status)[4]=1;Phase(&seal,1);
 Read(o[69],PioneerQ38ProjectionRoute(o[68]),R::Original,20480);Copy(R::OriginalCopy,R::Original,5120);
 Norm(&seal,0);CaptureRecipes(&seal,0,4);for(unsigned i=0;i<4;i++)Matrix(&seal,i,4,2+i);
 Cohort(&seal);CaptureRecipes(&seal,4,1);Matrix(&seal,0,1,8);
 Phase(&seal,9);Norm(&seal,1);CaptureRecipes(&seal,5,3);Matrix(&seal,0,3,10);Matrix(&seal,1,3,11);
 Activation(&seal);Matrix(&seal,2,3,13);Finish(&seal);
 // Preserve Conv NEXT before a later recurrent layer reuses common scratch.
 // This is provisional on-card staging, not a current-history slot flip.
 namespace H=PioneerQ38RecurrentHistoryLayout;uint32_t history=0,tracked[8];
 if(!H::Address(o[9],o[3],history))scalar_fault_park();
 for(unsigned i=0;i<8;i++)tracked[i]=*Words(0xffb20000+Counter(i));
 for(unsigned at=0;at<H::Bytes;at+=16384){
  unsigned bytes=H::Bytes-at;if(bytes>16384)bytes=16384;
  Transfer(o[9]+TT_Q38_CONV_NEXT+at,o[8],R::Scratch,bytes,false,tracked);
  Transfer(history+at,o[8],R::Scratch,bytes,true,tracked);
 }
 EndSeal(&seal);
 if(!Quiet())scalar_fault_park();
 Words(R::Status)[4]=0;Words(R::Status)[6]=o[3];Words(R::Status)[7]=o[4];Fence();Words(R::Status)[1]=o[48];Fence();
 asm volatile("mv s11,%0"::"r"(parent):"s11","memory");
 return A::Hidden; // NEXT state/history are not committed by this return
}
