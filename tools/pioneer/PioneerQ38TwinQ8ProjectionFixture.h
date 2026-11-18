/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWIN_Q8_PROJECTION_FIXTURE_H
#define PIONEER_Q38_TWIN_Q8_PROJECTION_FIXTURE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE)
#error Twin Q8 projection fixture is a separate userspace diagnostic
#endif
#include "PioneerQ38NativeQ8FullSource.h"
#include "PioneerQwen38TiledReader.h"
#include "PioneerQ38TwinQ8ProjectionSequence.h"
#include <array>
#include <memory>

// DATA for an isolated paired projection, not a model-load, L1 ownership,
// user ABI, or compute grant. Authenticate the original GGUF on its own
// FILE before calling; the source covenant must remain true at every page.
namespace PioneerQ38TwinQ8ProjectionFixture {
using Codec=PioneerQ38NativeQ8Data;
enum : unsigned { Type=8,Inputs=5120,Groups=3,Rows=48,Blocks=160,
 Bytes=Groups*Blocks*Codec::TileBytes,Pages=(Bytes+4095)/4096,Storage=Pages*4096 };
static_assert(Inputs==Blocks*32 && Rows==Groups*16 && Bytes==276480 && Pages==68,
 "one fixed shard, not an arbitrary model upload");
static constexpr const char* Tensor="blk.0.attn_qkv.weight";

struct Fixture {
 using BytesArray=std::array<uint8_t,Storage>;
 BytesArray raw={},native={};
};

// Fixed public test activations for the same shard. No tokenizer/model
// state or numerical-reference code. These words can be written to the
// existing shared activation/scale source only by a future checked owner.
struct Activation {
 std::array<uint32_t,Inputs/2> words={};
 std::array<uint32_t,Blocks> scales={};
};
static inline bool MakeActivation(unsigned variant,Activation& out)
{
 if(variant>1)return false;
 namespace S=PioneerQ38TwinQ8ProjectionSequence;
 static_assert(Inputs==unsigned(S::K) && Blocks==unsigned(S::Blocks),"same exact test input");
 for(unsigned w=0;w<Inputs/2;w++)out.words[w]=S::InputWord(variant,w);
 for(unsigned b=0;b<Blocks;b++)out.scales[b]=S::ScaleWord(variant,b);
 return true;
}

// Use the same per-page shuffling and SM7 conversion as the full loader,
// including the split tiles at page edges and the padded last page.
template<class Ready>
static bool Build(PioneerQwen38Gguf& source,const Ready& ready,std::unique_ptr<const Fixture>& result)
{
 if(result || !ready() || !source.Stable())return false;
 const auto* tensor=source.Find(Tensor);
 if(!tensor || tensor->rank!=2 || tensor->type!=Type || tensor->inputs!=Inputs || tensor->outputs!=10240
  || tensor->bytes!=UINT64_C(10240)*Blocks*34)return false;
 auto next=std::make_unique<Fixture>();
 const unsigned index=unsigned(tensor-source.Tensors().data());
 PioneerQwen38TiledReader reader;
 if(!reader.Bind(source))return false;
 PioneerQ38PackedPart part={};part.kind=PIONEER_Q38_WEIGHT;part.tensor=index;part.layer=0;
 part.role=5;part.rows=Rows;part.bytes=Bytes;part.allocated=Storage;part.rowBytes=PioneerQ38TiledRowBytes(Type,Inputs);
 namespace Q=PioneerQ38NativeQ8FullSource;
 if(Q::Layout(part,*tensor)!=Q::Format::SignedMagnitude)return false;
 for(unsigned p=0;p<Pages;p++){
  auto* raw=next->raw.data()+p*Codec::PageBytes;auto* native=next->native.data()+p*Codec::PageBytes;
  if(!ready() || !reader.ReadPage(source,index,0,Rows,p,raw,Codec::PageBytes)
   || !Q::Prepare(part,p,raw,native,Codec::PageBytes) || !ready())return false;
 }
 if(!source.Stable() || !ready())return false;
 result=std::move(next);
 return true;
}
}
#endif
