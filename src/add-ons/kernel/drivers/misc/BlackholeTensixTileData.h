/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_TENSIX_TILE_DATA_H
#define BLACKHOLE_TENSIX_TILE_DATA_H
#include "BlackholeTensixImage.h"

// Data only. Fixed 16x16 BF16 row-major A[k][column] and B[row][k].
// The device computes B*A, in two groups of eight rows. No code, addresses,
// dimensions, descriptors or arbitrary device-register inputs are accepted.
struct tt_tensix_tile_data {
	uint32_t version, flags;
	uint16_t weights[256], activations[256];
};
static inline bool BlackholeTileDataValid(const tt_tensix_tile_data& input)
{
	if (input.version != 1 || input.flags != 0) return false;
	for (unsigned i = 0; i < 256; i++)
		if ((input.weights[i] & 0x7f80) == 0x7f80
			|| (input.activations[i] & 0x7f80) == 0x7f80
			|| ((input.weights[i] & 0x7f80) == 0 && (input.weights[i] & 127) != 0)
			|| ((input.activations[i] & 0x7f80) == 0 && (input.activations[i] & 127) != 0)) return false;
	return true;
}
static inline uint32_t BlackholeTileDataImageWord(unsigned index,
	const tt_tensix_tile_data& input)
{
	uint32_t address = BlackholeTensixImageAddress(index,20);
	if (address == 0x70334) return 0x14c0006f;
	if (address == 0x70338 || address == 0x7033c) return 0x00000013;
	if (address >= 0x70480 && address < 0x704c4) {
		const uint32_t phases[] = {0x00000393,0xb20b02b7,0x0072e2b3,0x00542023,
			0xf31ff0ef,0x260002b7,0x02028293,0x00300e13,0x01c39663,
			0x26c002b7,0x02028293,0x00542023,0xf11ff0ef,0x00138393,
			0x00400e13,0xfdc394e3,0xe85ff06f};
		return phases[(address-0x70480)/4];
	}
	const uint16_t* values = 0;
	unsigned element = 0;
	if (address >= 0x70500 && address < 0x70700) {
		values = input.activations; element = (address-0x70500)/2;
	} else if (address >= 0x70710 && address < 0x70910) {
		values = input.weights; element = (address-0x70710)/2;
	}
	if (values != 0) return uint32_t(values[element]) | (uint32_t(values[element+1])<<16);
	return BlackholeTensixImageWord(index,false,20);
}
#endif
