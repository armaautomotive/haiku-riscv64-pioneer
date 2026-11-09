/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_BF16_MATRIX_H
#define PIONEER_Q38_DENSE_BF16_MATRIX_H
#if !defined(PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT) || PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Experimental offline dense matrix kernel only; not a driver or Work selection
#endif
#if defined(PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE) && (PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE != 1 \
 || (!defined(PIONEER_Q38_DENSE_BF16_SRCA_BASE_TEST_ONLY) && !defined(PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION)) \
 || (defined(PIONEER_Q38_DENSE_BF16_SRCA_BASE_TEST_ONLY) && PIONEER_Q38_DENSE_BF16_SRCA_BASE_TEST_ONLY != 1) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error Dense BF16 SrcA base candidate requires isolated test or worker inspection
#endif
#include "PioneerQ38NativeBfpMatvec.h"

// Matrix-vector product for 32 output rows and either 32 or 256 BF16 inputs.
// The caller supplies one or eight K32 tiles via stage.Tile(i), filling the
// caller-selected matrix and vector operands and their headers, then fence.
// Matrix operands are four face-major 16x16 BF16 faces; the query has eight
// copies of each 16-element half. Padded key rows must be zeroed by stage.
// Dst rows 32/40 retain the sums until the caller consumes them, and score
// scaling/masking/softmax and cross-chunk PV merging are outside this producer.
//
// The caller must own L1, FPU configuration, both unpack/source banks and Dst
// for the whole call. This is neither a device ownership grant nor integrated
// attention: BF16 rounding, existing FP16 KV conversion, errors, numerical
// quality, real instructions and timing still require qualification.
struct PioneerQ38DenseBf16Matrix {
 static constexpr unsigned Width=256, TileWidth=32, Tiles=Width/TileWidth;
private:
 template<class E> static bool Begin(E& e,unsigned matrix,unsigned vector,unsigned tiles) {
  // Header, payload and address ownership are caller duties; these two
  // spans must not overlap even when the caller supplies malformed input.
  if((tiles!=1 && tiles!=Tiles) || (matrix&15) || (vector&15) || matrix<16 || vector<16
    || matrix>0x16b800 || vector>0x16be00
    || !(matrix+2048<=vector-16 || vector+512<=matrix-16))return false;
  // Start from the existing four-fidelity-phase setup; change *both* SrcA
  // format declarations and its byte-addressed destination for BF16.
  if(!PioneerQ38NativeBfpMatvec::Setup(e,8))return false;
  e.Config(0,0x42b5);e.Config(1,0x60aa0000);
  e.Config(64,0x01000015);e.Config(72,0x125);
#ifdef PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE
  // Unverified physical-lane hypothesis: SrcA output base may be measured
  // in 16-bit slots even when its L1 payload is BF16. Never select in Work.
  e.Config(84,0x00400040);
#else
  e.Config(84,0x00400080);
#endif
  e.Config(76,matrix/16-1);e.Config(124,vector/16-1);
  e.Op(0x3700000f);e.Op(0x10180000);e.Drain(); // clear once, NOT per K32
  return true;
 }
 template<class E> static void AccumulateTile(E& e){
  e.Op(0x3700000f);e.Op(0x5e2ffc00);e.Op(0x5e43fc00);
  e.Op(0x5160000b);e.Op(0x5460000f);
  e.Op(0x420000c1);e.Op(0x428000c1);e.Drain();
  for(unsigned face=0;face<4;face++) {
   if(face){e.Op(0x37040201);e.Op(0x37040201);}
   e.Op(face&1?0x3700200a:0x3700000a);
   for(unsigned phase=0;phase<4;phase++)
    e.Op(0x26004000|(face<2?32:40)|((face==3 && phase==3)?0x00c00000:0));
  }
  e.Drain(); // both source banks released before stage can rewrite inputs
 }
public:
 template<class E,class Stage> static bool Product(E& e,Stage& stage,unsigned matrix,unsigned vector,unsigned tiles) {
  if(!Begin(e,matrix,vector,tiles))return false;
  for(unsigned tile=0;tile<tiles;tile++) {
   if(!stage.Tile(tile))return false; // no successful score publication
   AccumulateTile(e);
  }
  return true;
 }
 template<class E,class Stage> static bool Scores(E& e,Stage& stage,unsigned matrix,unsigned vector){
  return Product(e,stage,matrix,vector,Tiles);
 }
 template<class E,class Stage> static bool Values(E& e,Stage& stage,unsigned matrix,unsigned vector){
  return Product(e,stage,matrix,vector,1);
 }
 // Preserve Dst across all four BF16 component products. This avoids three
 // redundant hardware setups, Dst clears and intermediate result readbacks.
 // Changed FP32 accumulation order is approximate and needs validation.
 template<class E,class Stage> static bool ValuesFour(E& e,Stage& stage,unsigned matrix,unsigned vector){
  if(!Begin(e,matrix,vector,1))return false;
  for(unsigned part=0;part<4;part++){
   if(!stage.Select(part) || !stage.Tile(0))return false;
   AccumulateTile(e);
  }
  return true;
 }
 // Preconvert both input components once. The four distinct products reuse
 // those immutable L1 buffers and the same destination accumulator.
 template<class E,class Stage> static bool ValuesCached(E& e,Stage& stage){
  if(!Begin(e,Stage::Matrix(0),Stage::Vector(0),1) || !stage.Prepare())return false;
  for(unsigned part=0;part<4;part++){
   if(part){e.Config(76,Stage::Matrix(part)/16-1);e.Config(124,Stage::Vector(part)/16-1);}
   AccumulateTile(e);
  }
  return true;
 }
};
#endif
