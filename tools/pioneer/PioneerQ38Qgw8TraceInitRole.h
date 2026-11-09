/* SPDX-License-Identifier: MIT */
// Only the fake-tag, non-loadable trace-read inspection may select this.
#ifdef PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION
#if PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_TRACE_READ_INSPECTION) || PIONEER_Q38_QGW8_TRACE_READ_INSPECTION != 1 \
 || defined(PIONEER_Q38_QGW8_TRACE_INIT_DATA_INSPECTION)
#error Trace initialization requires its separate fake-owner inspection
#endif
#include "PioneerQ38Qgw8TraceReadInspectionRole.h"
#endif
#ifndef PIONEER_Q38_QGW8_TRACE_INIT_BINDING_H
#define PIONEER_Q38_QGW8_TRACE_INIT_BINDING_H
#ifdef PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION
enum {PioneerQ38Qgw8TraceInitInspection=1};
#else
enum {PioneerQ38Qgw8TraceInitInspection=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION
static_assert(PioneerQ38Qgw8TraceInitInspection==1,"late trace init selection");
#else
static_assert(PioneerQ38Qgw8TraceInitInspection==0,"late trace init removal");
#endif
