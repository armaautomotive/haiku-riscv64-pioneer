/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DECODER_RUNTIME 1
#include "pioneer_q38_projection_mutable_kernel_compile.cpp"
extern "C" bool pioneer_decoder_history_span(const PioneerQ38ConvHistoryBanks& state,const PioneerQ38GdnIdentity& id,
 unsigned region,unsigned first,unsigned pages,PioneerQ38ConvHistoryExtent& out){return state.InitializeSpan(id,region,first,pages,out);}
extern "C" bool pioneer_decoder_history_fresh_span(const PioneerQ38ConvHistoryBanks& state,const PioneerQ38GdnIdentity& id,
 unsigned region,unsigned first,unsigned pages,PioneerQ38ConvHistoryExtent& out){return state.FreshInitializationSpan(id,region,first,pages,out);}
