/* SPDX-License-Identifier: MIT */
// BUILD ONLY. No old program package or native subject selects this policy.
#include "PioneerQ38Bf16RootRole.h"
#include "PioneerQ38WorkerSealedAdmissionRole.h"
#include "PioneerQ38Bf16ProfileBuildRole.h"
#if !defined(PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE) || PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE != 1 \
 || !defined(__riscv) || __riscv != 1 || __riscv_xlen != 32 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE) || PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT) || PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION) || PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION != 1 \
 || !defined(PIONEER_Q38_BF16_PROFILE_IMAGE) || PIONEER_Q38_BF16_PROFILE_IMAGE != 1 \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(TT_PIONEER_DRIVER_RELOAD)
#error Deferred prepare audit requires its separate RV32 BF16 profiled returning component BUILD
#endif
