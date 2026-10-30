/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ObservedTrace.h" // exact HOST observer,not CPU products
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)
#error EOS state DATA bridge is HOST DATA only,never native
#endif
#ifndef PIONEER_Q38_EOS_STATE_DATA_BRIDGE_H
#define PIONEER_Q38_EOS_STATE_DATA_BRIDGE_H
// Const output-only DATA. No model/processor/memory callbacks,expected CPU
// products,ready/ownership/history setters or native capabilities accepted.
// A label/count/file marker is NEVER actual EOS,STOP or END evidence.
bool SaveQ38ObservedEosStateData(const char*,unsigned,unsigned,const uint32_t*,unsigned);
bool FinishQ38ObservedEosStateData(const char*,unsigned);
#endif
