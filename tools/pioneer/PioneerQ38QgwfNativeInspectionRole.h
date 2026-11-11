/* SPDX-License-Identifier: MIT */
// Non-loadable QGWF compile inspection. Never a device or Work capability.
#include "PioneerQ38QgweNativeInspectionRole.h"
#ifdef PIONEER_Q38_QGWF_NATIVE_INSPECTION
#if PIONEER_Q38_QGWF_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWE_NATIVE_INSPECTION) || PIONEER_Q38_QGWE_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWF_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWF_PROGRAMS_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWF_DUAL_BATCH_PLAN) || PIONEER_Q38_QGWF_DUAL_BATCH_PLAN != 1
#error QGWF native inspection requires the complete fake-tag compile-only profile
#endif
#endif
#ifndef PIONEER_Q38_QGWF_NATIVE_INSPECTION_BINDING_H
#define PIONEER_Q38_QGWF_NATIVE_INSPECTION_BINDING_H
#ifdef PIONEER_Q38_QGWF_NATIVE_INSPECTION
enum {PioneerQ38QgwfNativeInspection=1};
#else
enum {PioneerQ38QgwfNativeInspection=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWF_NATIVE_INSPECTION
static_assert(PioneerQ38QgwfNativeInspection==1,"late QGWF native inspection selection");
#else
static_assert(PioneerQ38QgwfNativeInspection==0,"late QGWF native inspection removal");
#endif
