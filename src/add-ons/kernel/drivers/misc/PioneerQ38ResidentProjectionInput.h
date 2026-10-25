/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ResidentOutputOwnerRole.h"
#ifndef PIONEER_Q38_RESIDENT_PROJECTION_INPUT_H
#define PIONEER_Q38_RESIDENT_PROJECTION_INPUT_H
#include "PioneerQ38GdnIdentity.h"
class PioneerQ38ProjectionGraphOwner;
class PioneerQ38ResidentOutputOwner;
class PioneerQ38ProjectionMutableBanks;
class PioneerQ38CanonicalModelPlan;
class PioneerQ38LearnedRmsController;
// Private borrowed completion lineage, NOT an initialized-state or IO grant.
// ONLY actual QPS1 Close can publish after ALL four genuine GPU projections,
// STOP/unheld physical terminals/ALL holds/full originals+commands restored.
// Current is CPU lineage ONLY. The future actual consumer MUST compare its
// physical source/commands/counters to the parent's retained accepted ledger,
// retain ALL original context/clock/model/hosts, and claim initialized history
// and recurrent state separately. No public constructor/setter/ready claim.
class PioneerQ38ResidentProjectionInput {
 friend class PioneerQ38ProjectionGraphOwner;
 friend class PioneerQ38ResidentOutputOwner;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT
 friend class PioneerQ38ResidentLayerOwner;
#endif
 const PioneerQ38ProjectionGraphOwner* owner=nullptr;
 const PioneerQ38ResidentOutputOwner* consumer=nullptr;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT
 const class PioneerQ38ResidentLayerOwner* layerConsumer=nullptr;
#endif
 const PioneerQ38ProjectionMutableBanks* graph=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38LearnedRmsController* rms=nullptr;
 const uint8_t* ledger=nullptr;
 PioneerQ38GdnIdentity identity={};
 uint32_t token=UINT32_MAX,position=UINT32_MAX,bank=7,phase=0,pages=0;
 uint32_t raw=0,z=0,a=0,b=0,source[16]={};
 PioneerQ38ResidentProjectionInput()=default;
 ~PioneerQ38ResidentProjectionInput()=default;
 PioneerQ38ResidentProjectionInput(const PioneerQ38ResidentProjectionInput&)=delete;
 PioneerQ38ResidentProjectionInput& operator=(const PioneerQ38ResidentProjectionInput&)=delete;
public:
 bool Current()const; // defined after the complete actual parent owner
};
static_assert(sizeof(PioneerQ38ResidentProjectionInput)<256,"private borrowed identity/addresses,not model or IO authority");
#endif
