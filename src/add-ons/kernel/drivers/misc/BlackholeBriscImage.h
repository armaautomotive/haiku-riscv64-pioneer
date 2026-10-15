/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_BRISC_IMAGE_H
#define BLACKHOLE_BRISC_IMAGE_H
#include <stdint.h>

// Original fixed RV32I image, independently assembled/decoded by host tests.
// No general firmware upload interface. Index 0 is the sole reset word;
// 1..19 are code, 20..21 the two L1 result words (not hardware mailboxes).
enum { TT_BRISC_CODE_WORDS = 19, TT_BRISC_IMAGE_WORDS = 22 };
static inline uint32_t BlackholeBriscImageAddress(unsigned index)
{
	if (index == 0) return 0;
	if (index <= TT_BRISC_CODE_WORDS) return 0x70000 + (index - 1) * 4;
	if (index < TT_BRISC_IMAGE_WORDS) return 0x17ffc0 + (index - 20) * 4;
	return UINT32_MAX;
}
static inline uint32_t BlackholeBriscImageWord(unsigned index)
{
	const uint32_t image[] = {
		0x0007006f, // reset jump, changes no bytes in ARC's low-L1 scratch
		0x001802b7, 0xfc028293, 0x00000313, 0x00100393, 0x02100e13,
		0x00730333, 0x00138393, 0xffc3cce3, 0x0062a023, 0x0002ae83,
		0x000e8013, 0x0ff0000f, 0x50495337, 0xf4e30313, 0x0062a223,
		0x0042ae83, 0x000e8013, 0x0ff0000f, 0x0000006f, 0, 0
	};
	return index < TT_BRISC_IMAGE_WORDS ? image[index] : 0;
}
#endif
