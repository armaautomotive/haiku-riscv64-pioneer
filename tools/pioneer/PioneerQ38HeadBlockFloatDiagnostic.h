/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC_H
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC_H
#if !defined(PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC) || PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(__FAST_MATH__)
#error Output-head conversion is isolated CPU-only diagnostic code
#endif
#include "PioneerQwen38Gguf.h"
#include "PioneerBlackholeBlockFloat.h"
#include <array>
#include <cfenv>
#include <cstring>
#include <vector>

// One 32-row band of the EXACT original output.weight in tile-major order.
// This performs no cache write, device allocation, upload or inference.
// Caller authenticates the full same-open source independently. The two
// representations are lossy and deliberately not valid old Q6 consumers.
namespace PioneerQ38HeadBlockFloatDiagnostic {
using Codec=PioneerBlackholeBlockFloat;
template<class Source> bool PackBand(Source& source,unsigned index,unsigned first,Codec::Format format,
 std::vector<uint8_t>& output){
 if(!source.Ready() || !source.Stable() || !source.Epoch() || std::fegetround()!=FE_TONEAREST
  || index>=source.Tensors().size() || first%32 || first>=248320
  || (format!=Codec::Format::Bfp4B && format!=Codec::Format::Bfp8B))return false;
 const auto tensor=source.Tensors()[index];PioneerQ38Tensor expected;
 if(!PioneerQ38Expected(64,1,expected) || tensor.name!=expected.name || tensor.rank!=2 || tensor.type!=14
  || tensor.inputs!=5120 || tensor.outputs!=248320 || tensor.bytes!=expected.bytes || first>tensor.outputs-32)return false;
 const uint64_t epoch=source.Epoch();const unsigned tileBytes=Codec::Bytes(format);
 std::vector<float> rows(size_t(32)*5120);std::vector<uint8_t> staged(size_t(160)*tileBytes);
 std::array<float,1024> tile={};
 for(unsigned row=0;row<32;row++){
  if(!source.ReadRow(index,first+row,rows.data()+size_t(row)*5120,5120)
   || !source.Ready() || !source.Stable() || source.Epoch()!=epoch)return false;
 }
 for(unsigned col=0;col<160;col++){
  for(unsigned row=0;row<32;row++)for(unsigned k=0;k<32;k++)tile[row*32+k]=rows[size_t(row)*5120+col*32+k];
  if(!Codec::PackTile(format,tile.data(),tile.size(),staged.data()+size_t(col)*tileBytes,tileBytes))return false;
 }
 if(!source.Ready() || !source.Stable() || source.Epoch()!=epoch || index>=source.Tensors().size())return false;
 const auto& after=source.Tensors()[index];
 if(after.name!=tensor.name || after.type!=tensor.type || after.rank!=tensor.rank || after.inputs!=tensor.inputs
  || after.outputs!=tensor.outputs || after.bytes!=tensor.bytes || after.offset!=tensor.offset)return false;
 output=std::move(staged);return true;
}
}
#endif
