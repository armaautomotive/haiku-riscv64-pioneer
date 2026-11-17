/* SPDX-License-Identifier: MIT */
// Isolated 16-output two-activation Q4_K proof. The optional full-width
// role reads weights under fixed NoC limits. No native owner, speculative
// state, Work selector or model inference.
#if !defined(PIONEER_Q38_TWO_TOKEN_Q4_IMAGE) || PIONEER_Q38_TWO_TOKEN_Q4_IMAGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Separate offline two-token Q4 image required
#endif
#define block_main unused_single_token_q38_block
#include "blackhole_qwen38_block.cpp"
#undef block_main
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_STREAM
#if PIONEER_Q38_TWO_TOKEN_Q4_STREAM != 1
#error Exact separate two-token Q4 stream role required
#endif
#include "PioneerQ38TwoTokenQ4StreamLayout.h"
#endif
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
#if PIONEER_Q38_TWO_TOKEN_Q4_VECTOR != 1 || !defined(PIONEER_Q38_TWO_TOKEN_Q4_STREAM)
#error Exact separate two-token Q4 vector stream role required
#endif
#include "PioneerQ38TwoTokenQ4Affine.h"
#include "PioneerQ38VectorFixedFormat.h"
#include "PioneerQ38Q4Phases.h"

namespace {
using VectorAffine=PioneerQ38TwoTokenQ4Affine;
struct PairEmitter {
 void Op(uint32_t word) { Emit(word); }
 void Config(unsigned i,uint32_t v) { ::Config(i,v); }
 void Drain() { ::Drain(); }
};
}
// Separate linked boundaries for the instruction-level qualification.
extern "C" __attribute__((noinline)) void pair_vector_begin(){
 PairEmitter e;
 Emit(0x10180000);Drain(); // one power-on initialization, never a panel clear
 e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
 e.Op(0xb2050000);e.Drain();
 e.Config(1,0xa0200000);e.Config(49,0);e.Config(64,0x100010);
 e.Config(65,16);e.Config(72,0);e.Config(73,0x13);e.Config(76,0x9fff);
 // Same full-row zero UNPACK as VectorDstInit. Input0 already contains
 // >1KiB of validated packed operands and is never modified by the worker.
 for(unsigned row=64;row<192;row+=16){
  if(row==80)continue; // no scratch here
  e.Config(84,0x00400000+(row+4)*16);e.Op(0x42000091);e.Drain();
 }
 e.Config(1,0xa03c8000);e.Config(64,0x10001e);
 e.Config(72,14);e.Config(73,0x13);e.Config(76,0x7070);e.Config(84,0x00400040);
 e.Op(0xb2050004);e.Drain();
 PioneerQ38Q4PhasesSetup(e,12);
 VectorAffine::Begin(e);
}
extern "C" __attribute__((noinline)) void pair_vector_block(uint32_t a,uint32_t b){
 constexpr unsigned Tile=PioneerQ38TwoTokenQ4StreamLayout::Tile;
 PairEmitter e;
 if(!PioneerQ38VectorQ4Metadata::FiniteBlock(Words(Tile),2304)
  || !VectorAffine::Block(e,Tile,a,b))block_fault_park();
}
extern "C" __attribute__((noinline)) void pair_vector_affine(unsigned p,int a,int b){
 PairEmitter e;
 if(!VectorAffine::Panel(e,p,a,b))block_fault_park();
}
static void PairVectorMatrix(unsigned panel){
 PairEmitter e;
 if(!PioneerQ38VectorFixedUnpack(e,12,panel))block_fault_park();
 Emit(0x3700000f);
 // Preserve both halves of all the decoded, metadata and accumulator rows.
 Emit(0x10080004);Emit(0x10080005);Drain();
 Emit(0x5160000a);Emit(0x5460000f);Emit(0x428000c1);Drain();
 PioneerQ38Q4Phases(e,12);
}
#endif

namespace {
enum {Magic=0x32543851,Width=256,Outputs=16,Tokens=2,
 Descriptor=0x74000,Input=0x80000,InputStride=Width*2,Output=0x84000,Weight=0x90000,
 Operand=0x70920,ReadDst=0xffbd8800};
static_assert(Input+Tokens*InputStride<=Output && Output+Tokens*Outputs*4<=Weight,
 "separate immutable inputs and packed tile, disjoint output");

bool ScaleValid(uint32_t bits){
 const unsigned m=bits&0x7fffffff;
 return !m || (m>=0x2f800000 && m<=0x47800000);
}
int Value(uint16_t x){return int(x&1023)*((x&0x8000)?-1:1);}

#ifdef PIONEER_Q38_TWO_TOKEN_Q4_STREAM
namespace Stream=PioneerQ38TwoTokenQ4StreamLayout;
bool Idle(){return !*Words(0xffb20040) && !(*Words(0xffb20240)&255);}
void CheckJob(const uint32_t* sealed){
 for(unsigned i=0;i<Stream::Words;i++)if(Words(Stream::Descriptor)[i]!=sealed[i])block_fault_park();
}
void Fetch(const uint32_t* sealed,unsigned block,uint32_t* tracked){
 const unsigned address=sealed[6]+block*Stream::TileBytes;
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
 static constexpr unsigned fixed[]={0,0,0,Stream::Tile,0,Stream::Self,0,0,Stream::TileBytes,0};
 const unsigned route=PioneerQ38ProjectionRoute(sealed[7]);
 auto command=[&](unsigned i){return i==0?address:i==2?route:fixed[i];};
#else
 const unsigned command[]={address,0,PioneerQ38ProjectionRoute(sealed[7]),Stream::Tile,0,Stream::Self,0,0,Stream::TileBytes,0};
#endif
 uint32_t expected[8];CheckJob(sealed);
 if(!Idle())block_fault_park();
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Stream::Counter(i))!=tracked[i])block_fault_park();
  expected[i]=tracked[i]+Stream::Delta(i);
 }
 // Only change the bounded address; all other fields must already equal
 // the real parent's immutable NoC setup. A job is not authority to map.
 for(unsigned i=1;i<10;i++)if(Words(0xffb20000)[i]!=
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
  command(i)
#else
  command[i]
#endif
 )block_fault_park();
 *Words(0xffb20000)=address;
 if(*Words(0xffb20000)!=address)block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool done=Idle();
  for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Stream::Counter(i))==expected[i];
  if(!done)continue;
  CheckJob(sealed);
  for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
   command(i)
#else
   command[i]
#endif
  )block_fault_park();
  for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
  Fence();return;
 }
 block_fault_park(); // uncertain NoC: no another request or output
}
#endif
}

extern "C" unsigned block_main(){
 const auto* job=Words(Descriptor);
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_STREAM
 uint32_t sealed[Stream::Words];for(unsigned w=0;w<Stream::Words;w++)sealed[w]=job[w];
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
 if(!Stream::Shape<Stream::VectorMagic>(sealed))return 0;
#else
 if(!Stream::Shape(sealed))return 0;
#endif
 const unsigned inputs=sealed[5],blocks=inputs/Width;
 for(unsigned t=0;t<Tokens;t++){
  for(unsigned b=0;b<blocks;b++)if(!ScaleValid(Words(Stream::Scale(t))[b]))return 0;
  for(unsigned w=0;w<inputs/2;w++){
   const uint32_t word=Words(Stream::Input(t))[w];
   if(!PioneerInt8OperandValid(uint16_t(word)) || !PioneerInt8OperandValid(uint16_t(word>>16)))return 0;
  }
 }
#else
 if(job[0]!=Magic || job[1]!=12 || job[2]!=2304 || job[3]
  || !ScaleValid(job[4]) || !ScaleValid(job[5]))return 0;
 for(unsigned w=6;w<16;w++)if(job[w])return 0;
 const float scale[Tokens]={Float(job[4]),Float(job[5])};
 for(unsigned w=0;w<Tokens*Width/2;w++){
  uint32_t word=Words(Input)[w];
  if(!PioneerInt8OperandValid(uint16_t(word)) || !PioneerInt8OperandValid(uint16_t(word>>16)))return 0;
 }
#endif
#ifndef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
 volatile float accum[Tokens][Outputs];
 for(unsigned t=0;t<Tokens;t++)for(unsigned n=0;n<Outputs;n++)accum[t][n]=0;
#endif
 Setup(); // initializes all eight SrcB rows, including unused rows 2..7
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
 pair_vector_begin();
#endif
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_STREAM
 uint32_t tracked[8];if(!Idle())block_fault_park();
 for(unsigned w=0;w<8;w++)tracked[w]=*Words(0xffb20000+Stream::Counter(w));
 if(tracked[2]!=tracked[6])block_fault_park();
 for(unsigned block=0;block<blocks;block++){
  Fetch(sealed,block,tracked);
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
  pair_vector_block(Words(Stream::Scale(0))[block],Words(Stream::Scale(1))[block]);
#else
  const float scale[Tokens]={Float(Words(Stream::Scale(0))[block]),Float(Words(Stream::Scale(1))[block])};
#endif
#endif
 for(unsigned p=0;p<Width/16;p++){
#ifndef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
  PioneerQ38Panel* local=reinterpret_cast<PioneerQ38Panel*>(0x91000);
  if(!PioneerQ38UnpackPanel(12,reinterpret_cast<const uint8_t*>(Weight),2304,p,local))block_fault_park();
  for(unsigned w=0;w<128;w++)Words(0x70710)[w]=local->operand[w];
#endif
  int sums[Tokens]={};
  for(unsigned t=0;t<Tokens;t++)for(unsigned w=0;w<8;w++){
   const uint32_t word=
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_STREAM
    Words(Stream::Input(t))[block*(Width/2)+p*8+w];
#else
    Words(Input+t*InputStride)[p*8+w];
#endif
   Words(Operand)[t*8+w]=word;
   sums[t]+=Value(uint16_t(word))+Value(uint16_t(word>>16));
  }
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
  (void)Words(Operand)[15];Fence();
  PairVectorMatrix(p);
  pair_vector_affine(p,sums[0],sums[1]);
#else
  (void)Words(0x70710)[127];(void)Words(Operand)[15];Fence();
  // The unchanged non-broadcast MVMUL issues four fidelity phases.
  // One SrcA unpack is shared by both independent SrcB rows.
  Matrix();
  for(unsigned t=0;t<Tokens;t++)for(unsigned n=0;n<Outputs;n++){
   const int dot=int(Words(ReadDst+t*Outputs*4)[n]);
   if(dot<-262144 || dot>262144)block_fault_park();
   const float product=(Float(local->coefficient[n])*scale[t])*float(dot);
   const float correction=(Float(local->minimum[n])*scale[t])*float(sums[t]);
   const float next=accum[t][n]+(product+correction);
   if((Bits(next)&0x7f800000)==0x7f800000)block_fault_park();
   accum[t][n]=next;
  }
#endif
 }
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_STREAM
 }
 CheckJob(sealed);
 if(!Idle())block_fault_park();
 for(unsigned w=0;w<8;w++)if(*Words(0xffb20000+Stream::Counter(w))!=tracked[w])block_fault_park();
#endif
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_VECTOR
 Config(3,0); // final FP32 export, non-overlapping logical rows 108 and 109
 constexpr unsigned Result=0xffbd8000+64*VectorAffine::Acc;
 uint32_t result[Tokens*Outputs];
 for(unsigned i=0;i<Tokens*Outputs;i++){
  const uint32_t word=Words(Result)[i];
  if((word&0x7f800000)==0x7f800000)block_fault_park();
  result[i]=word;
 }
 for(unsigned i=0;i<Tokens*Outputs;i++)Words(Output)[i]=result[i];
#else
 for(unsigned t=0;t<Tokens;t++)for(unsigned n=0;n<Outputs;n++)Words(Output)[t*Outputs+n]=Bits(accum[t][n]);
#endif
 Fence();
#ifdef PIONEER_Q38_TWO_TOKEN_Q4_STREAM
 return inputs/16;
#else
 return Width/16;
#endif
}
