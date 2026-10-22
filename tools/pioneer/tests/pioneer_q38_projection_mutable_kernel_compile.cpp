/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMutableBanks.h"
extern "C" bool pioneer_projection_joint_bind(PioneerQ38ProjectionMutableBanks& graph,PioneerResidentBanks& banks,
 const tt_dram_samples_result& proof,const PioneerQ38ProjectionIdentity& id,const uint8_t* ledger,unsigned pages,
 const PioneerQ38CanonicalModelPlan& plan,unsigned bank){return graph.BindCanonical(banks,proof,id,ledger,pages,plan,bank);}
extern "C" bool pioneer_projection_joint_span(const PioneerQ38ProjectionBanks& state,const PioneerQ38ProjectionIdentity& id,
 unsigned first,unsigned pages,PioneerQ38ProjectionExtent& out){return state.InitializeSpan(id,0,first,pages,out);}
