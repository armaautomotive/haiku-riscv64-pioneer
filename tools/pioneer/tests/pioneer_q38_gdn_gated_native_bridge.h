/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_GATED_NATIVE_BRIDGE_H
#define PIONEER_Q38_GDN_GATED_NATIVE_BRIDGE_H
#include "pioneer_q38_decoder_conv_bridge.h"
// TEST actual concurrently stepped instruction streams on byte endpoints.
// Starts ONLY on each real native release. No ready/producer/state grant,
// waiting-for-all-starts shortcut or numeric reference input is accepted.
class GatedNativeCohort {
 struct Impl;Impl* impl;
public:
 GatedNativeCohort(const char*,const PioneerQ38CanonicalModelPlan&,unsigned,ProjectionExecutionFabric&,DecoderHistoryBytes&);
 ~GatedNativeCohort();
 GatedNativeCohort(const GatedNativeCohort&)=delete;GatedNativeCohort& operator=(const GatedNativeCohort&)=delete;
 void Start(unsigned worker);void Advance(unsigned quantum=128);unsigned Pc(unsigned worker)const;
};
#endif
