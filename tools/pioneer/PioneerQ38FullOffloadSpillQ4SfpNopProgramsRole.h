/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h"
#if defined(PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PROGRAMS_H) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE)
#error SFPNOP spill DATA cannot alias another root or peer package
#endif
#if !defined(PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP) \
 || PIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP != 1
#error SFPNOP-ordered spill-Q4 DATA requires its explicit distinct selection
#endif
static_assert(PioneerQ38FullOffloadFirmwareSelection==14,"exact tag101 SFPNOP spill Q4 DATA binding");
