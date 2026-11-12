/* SPDX-License-Identifier: MIT */
// Reserved native prototype identity. Merely selecting this role does not
// establish a continuous device reservation, transfer an owner, or authorize
// a physical test. Work and CPU-reference execution remain excluded.
#if defined(PIONEER_Q38_QGWD_RESIDENT_NATIVE_146) && (PIONEER_Q38_QGWD_RESIDENT_NATIVE_146 != 1 \
 || !defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) || PIONEER_Q38_QGWF_UNROLL_NATIVE_145 != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 || !defined(PIONEER_DRIVER_BUILD_TAG) \
 || PIONEER_DRIVER_BUILD_TAG != 0x2026111100010146ULL \
 || defined(PIONEER_Q38_QGWD_RESIDENT_CLIENT_146) \
 || defined(PIONEER_Q38_QGWD_RESIDENT_REBIND_INSPECTION) || defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWD resident native146 requires its exact separate non-Work owner profile
#endif
#if defined(PIONEER_Q38_QGWD_RESIDENT_CLIENT_146) && (PIONEER_Q38_QGWD_RESIDENT_CLIENT_146 != 1 \
 || !defined(PIONEER_Q38_QGWF_UNROLL_CLIENT_145) || PIONEER_Q38_QGWF_UNROLL_CLIENT_145 != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_QGWD_RESIDENT_NATIVE_146) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_RESIDENT) || defined(PIONEER_Q38_QGWD_RESIDENT_REBIND_INSPECTION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWD resident client146 requires its exact separate non-Work diagnostic profile
#endif
#ifndef PIONEER_Q38_QGWD_RESIDENT_ROLE_BINDING_H
#define PIONEER_Q38_QGWD_RESIDENT_ROLE_BINDING_H
#ifdef PIONEER_Q38_QGWD_RESIDENT_NATIVE_146
enum {PioneerQ38QgwdResidentRole=1};
#elif defined(PIONEER_Q38_QGWD_RESIDENT_CLIENT_146)
enum {PioneerQ38QgwdResidentRole=2};
#else
enum {PioneerQ38QgwdResidentRole=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWD_RESIDENT_NATIVE_146
static_assert(PioneerQ38QgwdResidentRole==1,"late QGWD resident selection");
#elif defined(PIONEER_Q38_QGWD_RESIDENT_CLIENT_146)
static_assert(PioneerQ38QgwdResidentRole==2,"late QGWD resident client selection");
#else
static_assert(PioneerQ38QgwdResidentRole==0,"late QGWD resident removal");
#endif
