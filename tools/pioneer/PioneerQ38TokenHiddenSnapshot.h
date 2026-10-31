/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_HIDDEN_SNAPSHOT_H
#define PIONEER_Q38_TOKEN_HIDDEN_SNAPSHOT_H
#include <stdint.h>
// OUTPUT DATA ONLY. Never an upload,ready hint,CPU product or execution grant.
// header:magic/version/position/input/layers/words/native terminal kind/commits,
// actual driver tag low+high/generation low+high/chunks/three reserved words.
struct PioneerQ38TokenHiddenSnapshot {uint32_t header[16],words[5120],footer;};
static_assert(sizeof(PioneerQ38TokenHiddenSnapshot)==20548,"fixed little-endian raw hidden DATA");
namespace PioneerQ38TokenHiddenSnapshotData {
enum {Magic=0x31444851,Version=1,Footer=0x454e4448};
}
#endif
