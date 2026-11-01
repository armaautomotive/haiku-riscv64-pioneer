/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_LAYOUT_H
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_LAYOUT_H
#include "PioneerQ38GlobalFeedbackLayout.h"
#include "PioneerQ38TokenStateLayout.h"
// Geometry ONLY. No caller-provided readiness,initialized state or lifetime.
namespace PioneerQ38TokenCommitBoundaryLayout {
namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;
namespace F=PioneerQ38GlobalFeedbackLayout;namespace S=PioneerQ38TokenStateLayout;
static constexpr bool Apart(unsigned begin,unsigned bytes,unsigned other,unsigned length){
 return bytes && length && begin<=~0U-bytes && other<=~0U-length
  && (begin+bytes<=other || other+length<=begin);
}
static constexpr bool NumericDisjoint(unsigned begin,unsigned bytes){
 return Apart(begin,bytes,R::Original,20480) && Apart(begin,bytes,R::OriginalCopy,20480)
  && Apart(begin,bytes,R::Input,20480) && Apart(begin,bytes,R::InputShadow,20480)
  && Apart(begin,bytes,R::Native,10240) && Apart(begin,bytes,R::NativeShadow,10240)
  && Apart(begin,bytes,R::Scales,128) && Apart(begin,bytes,R::ScaleShadow,128)
  && Apart(begin,bytes,F::Local,F::Bytes) && Apart(begin,bytes,F::Shadow,F::Bytes);
}
// During CommitState only these L1 copy/descriptor/private-stack ranges
// are written. Packet/Transfer also update the existing owned counters and
// command CSR,whose HeadSeal/Scope/quiet/range checks remain per-packet.
static_assert(NumericDisjoint(R::Scratch,16384) && NumericDisjoint(R::Weight,16384)
 && NumericDisjoint(S::Local,S::Bytes) && NumericDisjoint(C::StackBegin,C::StackEnd-C::StackBegin),
 "state-copy destinations cannot change numeric input/feedback seal buffers");
static_assert(Apart(R::Scratch,16384,R::Weight,16384)
 && Apart(R::Scratch,16384,S::Local,S::Bytes) && Apart(R::Weight,16384,S::Local,S::Bytes),
 "copy scratch,readback shadow and checked state descriptor stay disjoint");
}
#endif
