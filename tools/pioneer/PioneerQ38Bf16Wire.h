/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BF16_WIRE_H
#define PIONEER_Q38_BF16_WIRE_H
// Shared DATA only. Naming the format grants neither device arithmetic nor
// permission to launch a worker, ROOT image, native owner or Work session.
struct PioneerQ38Bf16Wire {
 enum : unsigned {Magic=0x31494251,Version=1,Values=256,Words=128}; // QBI1
};
#endif
