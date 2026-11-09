/* SPDX-License-Identifier: MIT */
// Fake-tag-only control inspection. Never creates an ioctl or loadable role.
#ifdef PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION
#if PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_TRACE_READ_INSPECTION) || PIONEER_Q38_QGW8_TRACE_READ_INSPECTION != 1 \
 || defined(PIONEER_Q38_QGW8_TRACE_PACKET_INSPECTION)
#error Trace control inspection requires the distinct fake-owner role
#endif
#include "PioneerQ38Qgw8TraceReadInspectionRole.h"
#endif
#ifndef PIONEER_Q38_QGW8_TRACE_CONTROL_BINDING_H
#define PIONEER_Q38_QGW8_TRACE_CONTROL_BINDING_H
#ifdef PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION
enum {PioneerQ38Qgw8TraceControlInspection=1};
#else
enum {PioneerQ38Qgw8TraceControlInspection=0};
#endif
#endif
#ifdef PIONEER_Q38_QGW8_TRACE_CONTROL_INSPECTION
static_assert(PioneerQ38Qgw8TraceControlInspection==1,"late trace control selection");
#else
static_assert(PioneerQ38Qgw8TraceControlInspection==0,"late trace control removal");
#endif
