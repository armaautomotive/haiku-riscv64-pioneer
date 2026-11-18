/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_RECIPE_BATCH_H
#define PIONEER_Q38_TOKEN_RECIPE_BATCH_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Layer recipe batching is a separate card-ROOT inspection
#endif
#include "PioneerQ38GlobalChainLayout.h"

// Groups only the existing adjacent immutable templates and their same
// output slots. The owning caller must retain the original package, route,
// Transfer/Scope proofs and output lifetime. The helper grants no I/O.
namespace PioneerQ38TokenRecipeBatch {
namespace G=PioneerQ38GlobalChainLayout;
enum : unsigned { Bytes=5120, AtMost=3, ScratchBytes=16384 };
static_assert(unsigned(G::R::RecipeStride)==Bytes && unsigned(G::D::RecipeStride)==Bytes,
 "only original fixed-stride metadata templates");
static_assert(AtMost*Bytes<=ScratchBytes && G::R::Scratch+ScratchBytes<=G::D::Recipes,
 "one existing scratch transfer, disjoint from the destination for dense recipes");
static constexpr unsigned Take(unsigned left) {return left<AtMost?left:AtMost;}
static constexpr unsigned PacketCount(unsigned count) {return (count+AtMost-1)/AtMost;}

template<class E>
static bool Run(E& e,unsigned from,unsigned to,unsigned first,unsigned count,bool gddr)
{
 const bool shape=(count==8 && first==0 && gddr)
  || (count==4 && first==0 && !gddr && to==G::D::Recipes)
  || (count==3 && first==4 && gddr);
 if (((from|to)&63) || !shape) return false;
 const unsigned extent=count*Bytes;
 if(from>~0U-extent || to>~0U-extent || (gddr && from<to+extent && to<from+extent))return false;
 for(unsigned offset=0;offset<count;){
  const unsigned n=Take(count-offset),bytes=n*Bytes;
  if(!e.Read(from+offset*Bytes,bytes))return false;
  // The original per-recipe validation and all transformed bytes remain
  // exactly the same. Reject a bad group before any group publication.
  for(unsigned j=0;j<n;j++)if(!e.Transform(j*Bytes,first+offset+j))return false;
  if(!e.Publish(to+offset*Bytes,bytes,gddr))return false;
  offset+=n;
 }
 return true;
}
}
#endif
