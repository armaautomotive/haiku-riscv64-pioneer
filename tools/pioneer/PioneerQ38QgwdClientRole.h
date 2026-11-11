/* SPDX-License-Identifier: MIT */
#include "PioneerQ38QgwdTwoTokenRole.h"
// Separate one-token diagnostic client. This is neither an Assistant Work
// mode nor authority to install or execute a driver.
#ifdef PIONEER_Q38_QGWD_CLIENT_141
#if PIONEER_Q38_QGWD_CLIENT_141 != 1 \
 || !defined(PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION) || PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION) || PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) || PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION) || PIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q38_QGWD_NATIVE_141) || defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) || defined(PIONEER_Q38_QGWC_CLIENT_136) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error QGWD client requires its isolated tag141 one-token diagnostic role
#endif
#endif
#ifndef PIONEER_Q38_QGWD_CLIENT_ROLE_BINDING_H
#define PIONEER_Q38_QGWD_CLIENT_ROLE_BINDING_H
#ifdef PIONEER_Q38_QGWD_CLIENT_141
enum {PioneerQ38QgwdClientRole=1};
#else
enum {PioneerQ38QgwdClientRole=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWD_CLIENT_141
static_assert(PioneerQ38QgwdClientRole==1,"late QGWD client selection");
#else
static_assert(PioneerQ38QgwdClientRole==0,"late QGWD client removal");
#endif
