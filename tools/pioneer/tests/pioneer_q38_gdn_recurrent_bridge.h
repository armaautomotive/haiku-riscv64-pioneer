/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_RECURRENT_BRIDGE_H
#define PIONEER_Q38_GDN_RECURRENT_BRIDGE_H
#include "pioneer_q38_decoder_conv_bridge.h"
// TEST byte endpoints ONLY. No ready/producer/native capability fabricated.
unsigned ExecuteGdnRecurrentNativeBytes(const char*,const PioneerQ38CanonicalModelPlan&,unsigned worker,unsigned bank,
 ProjectionExecutionFabric&,DecoderHistoryBytes&);
#endif
