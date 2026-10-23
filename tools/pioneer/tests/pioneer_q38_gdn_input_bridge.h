/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_INPUT_BRIDGE_H
#define PIONEER_Q38_GDN_INPUT_BRIDGE_H
#include "pioneer_q38_projection_execution_machine.h"
#include <array>
class PioneerQ38CanonicalModelPlan;
struct GdnInputEmulationResult {std::array<uint32_t,322> values={};unsigned packets=0,steps=0,stack=0;};
// TEST emitted instructions + byte endpoints only,NO native owner facade.
// workspace is actual previous emitted QCB1/QPS1 output;learned exposes
// ORIGINAL bytes only,not precomputed references or execution authority.
bool ExecuteGdnInputResidentBytes(const char*,const PioneerQ38CanonicalModelPlan&,unsigned half,unsigned bank,
 const std::vector<uint32_t>& workspace,ProjectionExecutionLearnedBytes&,GdnInputEmulationResult&);
// Actual owner-installed bytes/control registers in SAME simulated native
// fabric. No descriptor/ready/producer manufacture;NO numerical oracle.
unsigned ExecuteGdnInputNativeBytes(const char*,const PioneerQ38CanonicalModelPlan&,unsigned worker,unsigned bank,
 ProjectionExecutionFabric&);
#endif
