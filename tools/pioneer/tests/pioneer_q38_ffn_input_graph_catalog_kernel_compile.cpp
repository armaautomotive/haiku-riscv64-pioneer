/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnInputGraphImages.h"
extern "C" uint32_t pioneer_q38_input_graph_catalog_word(unsigned worker,uint32_t address) {
 return PioneerQ38FfnInputGraphProgramWord(worker,address);
}
extern "C" bool pioneer_q38_input_graph_catalog_executable(unsigned worker,uint32_t address) {
 return PioneerQ38FfnInputGraphExecutableAddress(worker,address);
}
