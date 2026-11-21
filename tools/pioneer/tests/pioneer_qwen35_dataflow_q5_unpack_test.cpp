/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_LAYOUT_MACHINE_ONLY
#include "pioneer_q38_vector_layout_test.cpp"
#include "../PioneerQwen35DataflowVectorUnpack.h"
#include "../PioneerQwen35Q5Panel.h"
#include <cstdio>
#include <memory>
#include <vector>

int main()
{
	(void)Read; (void)Word;
	unsigned panels=0,weights=0;
	for (unsigned trial=0;trial<128;trial++) {
		std::vector<uint8_t> tile(16*176);
		uint32_t seed=0x6a09e667U^(trial*0x9e3779b9U);
		for (auto& byte:tile) {
			seed^=seed<<13; seed^=seed>>17; seed^=seed<<5;
			byte=uint8_t(seed);
		}
		for (unsigned row=0;row<16;row++) {
			tile[row]=0; tile[16+row]=0x3c; // d = finite 1.0
			tile[32+row]=0; tile[48+row]=0x3c; // dmin = finite 1.0
		}
		for (unsigned panel=0;panel<16;panel++) {
			PioneerQ38Panel expected{};
			assert(PioneerQwen35Q5Panel(tile.data(),unsigned(tile.size()),
				panel,&expected));
			PioneerQ38VectorPanelSpan span{};
			assert(PioneerQwen35DataflowQ5PanelSpan(panel,span));
			assert(span.second);
			auto machine=std::make_unique<VectorLayout>();
			for (unsigned k=0;k<16;k++) for (unsigned row=0;row<16;row++) {
				machine->Raw(k,row,tile[span.low+k*16+row]);
				machine->Raw(16+k,row,tile[span.high+k*16+row]);
			}
			assert(PioneerQwen35DataflowQ5Transform(*machine,panel));
			for (unsigned i=0;i<256;i++) {
				assert(machine->moves[i]==1);
				assert(machine->moved[i]
					==uint16_t(expected.operand[i/2]>>((i%2)*16)));
				weights++;
			}
			panels++;
		}
	}
	printf("PASS rewrite Q5_K Tensix SFPU unpack: %u panels, %u exact operands against the offline scalar reference; Q5 expansion stays on the Tensix instruction path\n",
		panels,weights);
}
