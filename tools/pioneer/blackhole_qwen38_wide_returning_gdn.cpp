/* SPDX-License-Identifier: MIT */
// Separate returning-service IMAGE only;no native selection or Work grant.
#include "PioneerQ38WideGdnReturningImageRole.h"
// Keep the standalone's pinned type/symbol identity unchanged,while the
// linked multi-TU service gets an unambiguous private emitter type.
#define Emitter PioneerQ38WideReturningEmitter
#include "blackhole_qwen38_resident_gdn.cpp"
#undef Emitter
