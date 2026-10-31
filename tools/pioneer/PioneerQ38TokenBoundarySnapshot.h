/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_BOUNDARY_SNAPSHOT_H
#define PIONEER_Q38_TOKEN_BOUNDARY_SNAPSHOT_H
#include <stdint.h>
// Distinct OUTPUT DATA family:OBSERVED position0/cancel-stage1 boundary.
// The header's layer count is observed,NEVER selected/proved by a host delay.
struct PioneerQ38TokenBoundarySnapshot {uint32_t header[16],words[5120],footer;};
static_assert(sizeof(PioneerQ38TokenBoundarySnapshot)==20548,"fixed raw boundary snapshot");
namespace PioneerQ38TokenBoundarySnapshotData {
enum {Magic=0x31424851,Version=1,Footer=0x454e4442};
}
#endif
