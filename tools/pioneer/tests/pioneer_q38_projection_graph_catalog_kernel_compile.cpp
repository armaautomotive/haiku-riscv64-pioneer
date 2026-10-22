/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
extern "C" uint32_t pioneer_projection_catalog_word(unsigned worker,uint32_t address){
 return PioneerQ38ProjectionGraphProgramWord(worker,address);
}
extern "C" bool pioneer_projection_catalog_executable(unsigned worker,uint32_t address){
 return PioneerQ38ProjectionGraphExecutableAddress(worker,address);
}
extern "C" uint32_t pioneer_projection_snapshot_address(unsigned worker,unsigned ordinal){
 return PioneerQ38ProjectionGraphSnapshotAddress(worker,ordinal);
}
extern "C" bool pioneer_projection_snapshot_contains(unsigned worker,uint32_t address){
 return PioneerQ38ProjectionGraphSnapshotContains(worker,address);
}
