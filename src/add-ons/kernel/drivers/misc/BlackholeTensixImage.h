/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_TENSIX_IMAGE_H
#define BLACKHOLE_TENSIX_IMAGE_H
#include <stdint.h>
#include "BlackholeTensixUnpackImage.h"

// Original fixed TRISC0 program, separately assembled and decoded by tests.
// No firmware upload API. Compute context on the sole idle tile is reset;
// only the fixed L1 footprint is preserved/restored, not Src/Dst contents.
enum {
	TT_TENSIX_CODE_WORDS = 160, TT_TENSIX_OUTPUT_WORDS = 256,
	TT_TENSIX_IMAGE_WORDS = 1 + TT_TENSIX_CODE_WORDS + TT_TENSIX_OUTPUT_WORDS + 3,
	TT_TENSIX_STATUS_INDEX = 1 + TT_TENSIX_CODE_WORDS + TT_TENSIX_OUTPUT_WORDS,
	TT_TENSIX_PARK_PC = 0x70258, TT_TENSIX_FINAL_PHASE = 0x70240
};
static inline unsigned BlackholeTensixCodeWords(unsigned stage) { if (stage >= 16) return TT_UNPACK_CODE_WORDS; return TT_TENSIX_CODE_WORDS; }
static inline unsigned BlackholeTensixImageWords(unsigned stage) { if (stage >= 16) return TT_UNPACK_IMAGE_WORDS; return TT_TENSIX_IMAGE_WORDS; }
static inline unsigned BlackholeTensixStatusIndex(unsigned stage) { return 1 + BlackholeTensixCodeWords(stage) + TT_TENSIX_OUTPUT_WORDS; }
static inline uint32_t BlackholeTensixParkPc(unsigned stage) { if (stage >= 16) return TT_UNPACK_PARK_PC; return TT_TENSIX_PARK_PC; }
static inline uint32_t BlackholeTensixFinalPhase(unsigned stage) { if (stage >= 16) return TT_UNPACK_FINAL_PHASE; return TT_TENSIX_FINAL_PHASE; }
static inline uint32_t BlackholeTensixImageAddress(unsigned index, unsigned stage = 0)
{
	if (index == 0) return 0x6000;
	if (index <= BlackholeTensixCodeWords(stage)) return 0x70000 + (index - 1) * 4;
	index -= 1 + BlackholeTensixCodeWords(stage);
	if (index < TT_TENSIX_OUTPUT_WORDS) return 0x17f800 + index * 4;
	index -= TT_TENSIX_OUTPUT_WORDS;
	return index < 3 ? 0x17ffc0 + index * 4 : UINT32_MAX;
}
static inline uint32_t BlackholeTensixImageWord(unsigned index, bool configSmoke = false, unsigned sfpuSmoke = 0)
{
	if (sfpuSmoke >= 16) return BlackholeTensixUnpackImageWord(index,sfpuSmoke);
	uint32_t address = BlackholeTensixImageAddress(index);
	if (((sfpuSmoke >= 7 && sfpuSmoke <= 9) || (sfpuSmoke >= 11 && sfpuSmoke <= 15)) && address >= 0x70170 && address <= 0x701d4) {
		// Same-sized independent kernel-style handoff-before-MOVD2 prefix.
		const uint32_t kernelOperands[] = {
			0x020a1063,0x430002b7,0x10128293,0x00542023,
			0x438002b7,0x10128293,0x00542023,0x0d0000ef,
			0x080022b7,0x00542023,0x080822b7,0x00428293,0x00542023,
			0x081022b7,0x00828293,0x00542023,0x081822b7,0x00c28293,0x00542023,
			0x0a0022b7,0x01028293,0x00542023,0x0a0822b7,0x01428293,0x00542023,0x088000ef
		};
		unsigned word = (address-0x70170)/4;
		if (sfpuSmoke >= 13 && (word == 2 || word == 5)) return 0x0c228293;
		return kernelOperands[word];
	}
	// Independent fixed configuration-only control. The decoder test proves
	// these two same-width substitutions bypass every Dst and matrix access.
	if (configSmoke && BlackholeTensixImageAddress(index) == 0x700e0) return 0x1540006f;
	if (sfpuSmoke == 1 && BlackholeTensixImageAddress(index) == 0x70118) return 0x144000ef;
	if (sfpuSmoke == 1 && BlackholeTensixImageAddress(index) == 0x7011c) return 0x1180006f;
	if (sfpuSmoke == 2 && BlackholeTensixImageAddress(index) == 0x70150) return 0x0e40006f;
	if (sfpuSmoke == 2 && BlackholeTensixImageAddress(index) == 0x70154) return 0x00000013;
	if ((sfpuSmoke == 3 || sfpuSmoke == 8) && address == 0x701d8) return 0x05c0006f;
	if ((sfpuSmoke == 3 || sfpuSmoke == 8) && address == 0x701dc) return 0x00000013;
	if ((sfpuSmoke == 4 || sfpuSmoke == 9) && address == 0x701e8) return 0x04c0006f;
	if ((sfpuSmoke == 4 || sfpuSmoke == 9) && address == 0x701ec) return 0x00000013;
	if (sfpuSmoke == 5 && BlackholeTensixImageAddress(index) == 0x70170) return 0x0c40006f;
	if (sfpuSmoke == 6 && BlackholeTensixImageAddress(index) == 0x701b8) return 0x07c0006f;
	if (sfpuSmoke == 10 && address == 0x70150) return 0x0980006f;
	if (sfpuSmoke == 10 && address == 0x70154) return 0x00000013;
	if (sfpuSmoke == 10 && address == 0x701e8) return 0xffbd83b7;
	if (sfpuSmoke == 10 && address == 0x701ec) return 0x00000013;
	if ((sfpuSmoke == 11 || sfpuSmoke == 13) && address == 0x701d8) return 0x120022b7;
	if ((sfpuSmoke == 12 || sfpuSmoke == 14) && address == 0x701d8) return 0x130012b7;
	if ((configSmoke || (sfpuSmoke != 0 && sfpuSmoke != 7 && sfpuSmoke < 10)) && address == 0x70238) return 0x0059a223;
	if (index == 0) return 0x0006a06f;
	const uint32_t code[] = {
		0xffe40437,0xffe804b7,0x00448493,0xffef0937,
		0x001809b7,0xfc098993,0x00100293,0x0059a023,
		0x0ff0000f,0xb20002b7,0x00542023,0x230000ef,
		0x00100393,0x04400e13,0x01039e93,0xb20002b7,
		0x01d2e2b3,0x00542023,0x00138393,0xffc396e3,
		0xb20202b7,0x00128293,0x00542023,0xb20302b7,
		0x00128293,0x00542023,0x1f4000ef,0x000042b7,
		0x2b528293,0x00592023,0x00092303,0x00030013,
		0x20aa02b7,0x00592223,0x00492303,0x00030013,
		0x00092423,0x00892303,0x00030013,0x00092623,
		0x00c92303,0x00030013,0x00092c23,0x01892303,
		0x00030013,0x00092e23,0x01c92303,0x00030013,
		0x36092823,0x37092303,0x00030013,0x0ff0000f,
		0x370002b7,0x00f28293,0x00542023,0x180000ef,
		0x00000a13,0x3f8002b7,0x3f800337,0x000a0663,
		0x400002b7,0x40400337,0x0102d293,0x710003b7,
		0x0072e2b3,0x00542023,0x01035313,0x711003b7,
		0x00736333,0x00642023,0x720302b7,0x00800393,
		0x00542023,0x00228293,0xfff38393,0xfe039ae3,
		0x721302b7,0x01028293,0x00400393,0x00542023,
		0x00228293,0xfff38393,0xfe039ae3,0x110000ef,
		0x100002b7,0x02028293,0x00800393,0x00542023,
		0x00128293,0xfff38393,0xfe039ae3,0x0f0000ef,
		0x080022b7,0x00542023,0x080822b7,0x00428293,
		0x00542023,0x081022b7,0x00828293,0x00542023,
		0x081822b7,0x00c28293,0x00542023,0x0a0022b7,
		0x01028293,0x00542023,0x0a0822b7,0x01428293,
		0x00542023,0x0a8000ef,0x020a1063,0x430002b7,
		0x00728293,0x00542023,0x438002b7,0x00728293,
		0x00542023,0x088000ef,0x260002b7,0x02028293,
		0x00542023,0x078000ef,0xffbd93b7,0x80038393,
		0x00180e37,0x800e0e13,0x009a1e93,0x01de0e33,
		0x08000f13,0x0003a283,0x005e2023,0x00438393,
		0x004e0e13,0xffff0f13,0xfe0f16e3,0x0ff0000f,
		0x001a0a13,0x0149a223,0x0049a283,0x00028013,
		0x0ff0000f,0x00200293,0xea5a16e3,0x020000ef,
		0x4d4152b7,0x43128293,0x0059a423,0x0089a303,
		0x00030013,0x0ff0000f,0x0000006f,0x0019a023,
		0x0009a303,0x00030013,0x0ff0000f,0x0004a023,
		0x0004a303,0x00030013,0x0ff0000f,0x00008067
	};
	return index <= TT_TENSIX_CODE_WORDS ? code[index - 1] : 0;
}
#endif
