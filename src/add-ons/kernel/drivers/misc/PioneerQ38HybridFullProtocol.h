/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38QgwdClientRole.h"
#include "../../../../../tools/pioneer/PioneerQ38Qgw8TraceClientRole.h"
#include "../../../../../tools/pioneer/PioneerQ38QgwcClientRole.h"
#if defined(PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION) && (PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION != 1 \
 || !defined(PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION) || PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION != 1 \
 || defined(PIONEER_Q38_QGWD_CLIENT_141) || defined(PIONEER_Q38_QGWD_NATIVE_141) || defined(_KERNEL_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWD multi-token simulation is a separate synthetic-only role
#endif
#ifndef PIONEER_Q38_QGWD_MULTI_SIM_IDENTITY_H
#define PIONEER_Q38_QGWD_MULTI_SIM_IDENTITY_H
#ifdef PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION
enum {PioneerQ38QgwdMultiSimulationIdentity=1};
#else
enum {PioneerQ38QgwdMultiSimulationIdentity=0};
#endif
#endif
#ifdef PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION
static_assert(PioneerQ38QgwdMultiSimulationIdentity==1,"late multi-token simulation selection");
#else
static_assert(PioneerQ38QgwdMultiSimulationIdentity==0,"late multi-token simulation removal");
#endif
#if defined(PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION) && (PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION) || PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) || PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION != 1 \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) || defined(PIONEER_Q38_QGWC_CLIENT_136) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGWD full-session inspection requires its isolated synthetic role
#endif
#ifdef PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
#include "../../../../../tools/pioneer/PioneerQ38Qgw8NativeInspectionRole.h"
#endif
#if defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) && (PIONEER_Q38_QGW8_HIDDEN_CLIENT_129 != 1 \
 || !defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) || PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128 != 1 \
 || defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) || defined(_KERNEL_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGW8 hidden client requires the isolated tag129 diagnostic role
#endif
#if defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) && (PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128 != 1 \
 || !defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) || PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION != 1 \
 || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || defined(_KERNEL_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE))
#error QGW8 tag128 client protocol requires its exact separate candidate role
#endif
#ifdef PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION
#include "../../../../../tools/pioneer/PioneerQ38Qgw8NativeInspectionRole.h"
#if PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) || PIONEER_Q38_QGW8_NATIVE_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_CONTROL_INSPECTION) || PIONEER_Q38_QGW8_CONTROL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_SPAN_INSPECTION) || PIONEER_Q38_QGW8_SPAN_INSPECTION != 1 \
 || defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION)
#error QGW8 native full control requires its exact non-loadable native inspection role
#endif
#endif
#if defined(PIONEER_Q38_FULL_OFFLOAD_PROTOCOL_H) || defined(PIONEER_Q38_BF16_FULL_PROTOCOL_H)
#error Hybrid full-card protocol cannot alias an older driver protocol
#endif
#if (defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) && PIONEER_Q38_HYBRID_MATRIX_NATIVE_127 != 1) \
 || (defined(PIONEER_Q38_HYBRID_MATRIX_CLIENT_127) && PIONEER_Q38_HYBRID_MATRIX_CLIENT_127 != 1)
#error Matrix candidate protocol selectors require exact value one
#endif
#if defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) && (PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_USER_SESSION_INSPECTION) || PIONEER_Q38_QGW8_USER_SESSION_INSPECTION != 1 \
 || defined(PIONEER_Q38_HYBRID_FIXED_126) || defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) \
 || defined(PIONEER_Q38_HYBRID_MATRIX_CLIENT_127) || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) \
 || defined(PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT) || defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \
 || defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE))
#error QGW8 full-session protocol is isolated inspection only
#endif
#if (defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) || defined(PIONEER_Q38_HYBRID_MATRIX_CLIENT_127)) \
 && (defined(PIONEER_Q38_HYBRID_FIXED_126) || (defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) && defined(PIONEER_Q38_HYBRID_MATRIX_CLIENT_127)))
#error Matrix candidate protocol cannot alias another candidate or its client role
#endif
#ifndef PIONEER_Q38_HYBRID_PROTOCOL_IDENTITY_BINDING_H
#define PIONEER_Q38_HYBRID_PROTOCOL_IDENTITY_BINDING_H
#if defined(PIONEER_Q38_QGWD_NATIVE_141) || defined(PIONEER_Q38_QGWD_CLIENT_141)
enum {PioneerQ38HybridProtocolIdentity=141};
#elif defined(PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION)
enum {PioneerQ38HybridProtocolIdentity=14};
#elif defined(PIONEER_Q38_QGWC_NATIVE_136) || defined(PIONEER_Q38_QGWC_CLIENT_136)
enum {PioneerQ38HybridProtocolIdentity=136};
#elif defined(PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134) || defined(PIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134)
enum {PioneerQ38HybridProtocolIdentity=134};
#elif defined(PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133) || defined(PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133)
enum {PioneerQ38HybridProtocolIdentity=133};
#elif defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132) || defined(PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132)
enum {PioneerQ38HybridProtocolIdentity=132};
#elif defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131) || defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131)
enum {PioneerQ38HybridProtocolIdentity=131};
#elif defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129)
enum {PioneerQ38HybridProtocolIdentity=129};
#elif defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128)
enum {PioneerQ38HybridProtocolIdentity=128};
#elif defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) || defined(PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION)
enum {PioneerQ38HybridProtocolIdentity=8};
#elif defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) || defined(PIONEER_Q38_HYBRID_MATRIX_CLIENT_127)
enum {PioneerQ38HybridProtocolIdentity=127};
#elif defined(PIONEER_Q38_HYBRID_FIXED_126)
enum {PioneerQ38HybridProtocolIdentity=126};
#else
enum {PioneerQ38HybridProtocolIdentity=125};
#endif
#endif
#if defined(PIONEER_Q38_QGWD_NATIVE_141) || defined(PIONEER_Q38_QGWD_CLIENT_141)
static_assert(PioneerQ38HybridProtocolIdentity==141,"late QGWD candidate protocol identity change");
#elif defined(PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION)
static_assert(PioneerQ38HybridProtocolIdentity==14,"late QGWD full-session identity change");
#elif defined(PIONEER_Q38_QGWC_NATIVE_136) || defined(PIONEER_Q38_QGWC_CLIENT_136)
static_assert(PioneerQ38HybridProtocolIdentity==136,"late corrected QGWC protocol identity change");
#elif defined(PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134) || defined(PIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134)
static_assert(PioneerQ38HybridProtocolIdentity==134,"late QGW8 dense-value protocol identity change");
#elif defined(PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133) || defined(PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133)
static_assert(PioneerQ38HybridProtocolIdentity==133,"late QGW8 dense-input protocol identity change");
#elif defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132) || defined(PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132)
static_assert(PioneerQ38HybridProtocolIdentity==132,"late QGW8 dense-stage protocol identity change");
#elif defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131) || defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131)
static_assert(PioneerQ38HybridProtocolIdentity==131,"late QGW8 trace protocol identity change");
#elif defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129)
static_assert(PioneerQ38HybridProtocolIdentity==129,"late QGW8 hidden protocol identity change");
#elif defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128)
static_assert(PioneerQ38HybridProtocolIdentity==128,"late QGW8 candidate protocol identity change");
#elif defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) || defined(PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION)
static_assert(PioneerQ38HybridProtocolIdentity==8,"late QGW8 protocol identity change");
#elif defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) || defined(PIONEER_Q38_HYBRID_MATRIX_CLIENT_127)
static_assert(PioneerQ38HybridProtocolIdentity==127,"late matrix protocol identity change");
#elif defined(PIONEER_Q38_HYBRID_FIXED_126)
static_assert(PioneerQ38HybridProtocolIdentity==126,"late fixed protocol identity change");
#else
static_assert(PioneerQ38HybridProtocolIdentity==125,"late hybrid protocol identity removal");
#endif
#ifndef PIONEER_Q38_HYBRID_FULL_PROTOCOL_H
#define PIONEER_Q38_HYBRID_FULL_PROTOCOL_H
#include "PioneerQ38TokenControlPacket.h"
#include "../../../../../tools/pioneer/PioneerQ38FullOffloadEog.h"
#define TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT (B_DEVICE_OP_CODES_END+0x8800)
#define TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL (B_DEVICE_OP_CODES_END+0x8801)
namespace PioneerQ38FullOffload {
#if defined(PIONEER_Q38_QGWD_NATIVE_141) || defined(PIONEER_Q38_QGWD_CLIENT_141)
static constexpr uint64_t BuildTag=UINT64_C(0x2026111000010141);
#elif defined(PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION)
static constexpr uint64_t BuildTag=UINT64_C(0x445747514f4e4452); // distinct synthetic QGWD owner only
#elif defined(PIONEER_Q38_QGWC_NATIVE_136) || defined(PIONEER_Q38_QGWC_CLIENT_136)
static constexpr uint64_t BuildTag=UINT64_C(0x2026110900010136);
#elif defined(PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134) || defined(PIONEER_Q38_QGW8_DENSE_VALUE_CLIENT_134)
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010134);
#elif defined(PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133) || defined(PIONEER_Q38_QGW8_DENSE_INPUT_CLIENT_133)
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010133);
#elif defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132) || defined(PIONEER_Q38_QGW8_DENSE_STAGE_CLIENT_132)
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010132);
#elif defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131) || defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131)
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010131);
#elif defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129)
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010129);
#elif defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128)
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010128);
#elif defined(PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION) || defined(PIONEER_Q38_QGW8_FULL_NATIVE_INSPECTION)
static constexpr uint64_t BuildTag=UINT64_C(0x51544b4e4f445256); // fake native inspection identity only
#elif defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) || defined(PIONEER_Q38_HYBRID_MATRIX_CLIENT_127)
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010127);
#elif defined(PIONEER_Q38_HYBRID_FIXED_126)
static_assert(PIONEER_Q38_HYBRID_FIXED_126==1,"exact fixed candidate selection required");
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010126);
#else
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010125);
#endif
#if defined(PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION)
enum {Version=1,MaxPrompt=1,MaxOutput=2,MaxPositions=2,Vocabulary=248320};
#elif defined(PIONEER_Q38_QGWD_NATIVE_141) || defined(PIONEER_Q38_QGWD_CLIENT_141) || defined(PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION) || defined(PIONEER_Q38_QGWC_NATIVE_136) || defined(PIONEER_Q38_QGWC_CLIENT_136) \
 || defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131) || defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131) \
 || defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129)
enum {Version=1,MaxPrompt=1,MaxOutput=1,MaxPositions=1,Vocabulary=248320};
#else
enum {Version=1,MaxPrompt=96,MaxOutput=32,MaxPositions=128,Vocabulary=248320};
#endif
enum {Matrices=1,Attention=2,Recurrent=4,KV=8,Embedding=16,Normalize=32,
 Residual=64,Activation=128,Greedy=256,AllCompute=511};
struct Support {uint32_t driver,version,prompt,output,positions,vocabulary,compute,reserved;};
static_assert(sizeof(Support)==32,"pointer-free complete-model capability DATA");
static inline bool SupportRequest(const Support& s){
 return !s.driver && !s.version && !s.prompt && !s.output && !s.positions && !s.vocabulary && !s.compute && !s.reserved;
}
static inline bool Supported(const Support& s){
 return s.driver==55 && s.version==Version && s.prompt==MaxPrompt && s.output==MaxOutput
  && s.positions==MaxPositions && s.vocabulary==Vocabulary && s.compute==AllCompute && !s.reserved;
}
static inline bool RequestShape(const PioneerQ38TokenControl::Packet& p){
 namespace P=PioneerQ38TokenControl;if(!P::RequestShape(p))return false;const auto& r=p.request;
 if(r.operation==P::Start)return r.prompts<=MaxPrompt && r.limit<=MaxPositions && r.limit-r.prompts+1<=MaxOutput
#if defined(PIONEER_Q38_QGWD_NATIVE_141) || defined(PIONEER_Q38_QGWD_CLIENT_141) || defined(PIONEER_Q38_QGWC_NATIVE_136) || defined(PIONEER_Q38_QGWC_CLIENT_136)
  && r.prompts==1 && r.limit==1 && p.prompt[0]==248319
#endif
#if defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131) || defined(PIONEER_Q38_QGW8_TRACE_CLIENT_131)
  && r.stopEos==0
#else
  && r.stopEos==1
#endif
  && r.eos0==PioneerQ38FullOffloadEog::Eos0 && r.eos1==PioneerQ38FullOffloadEog::Eos1;
 return true;
}
}
#endif
