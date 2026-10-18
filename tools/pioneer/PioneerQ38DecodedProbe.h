/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DECODED_PROBE_H
#define PIONEER_Q38_DECODED_PROBE_H
// Isolated59 DIAGNOSTIC ONLY. Independent scalar Q8 byte -> expected
// opaque Dst32 -> existing converted signed32 RISC view. No dot product,
// approximation,route authority,host result or production fallback.
// ISA8a34e25e Dst.md/SFPSTORE.md/MOVD2A.md;Config3 remains0x10000.
static inline uint32_t PioneerQ38DecodedQ8Risc(unsigned code)
{
	int value=int(code)-int((code&128)*2);
	uint16_t operand=PioneerInt8Operand(value);
	uint32_t raw=(uint32_t(operand&0x8000)<<16)
		|(uint32_t(operand&1023)<<21)|0x00100000;
	uint32_t risc=(raw&0x8000ffff)|((raw&0x7f000000)>>8)|((raw&0x00ff0000)<<7);
	return risc&0x80000000?0U-(risc&0x7fffffff):risc;
}
#endif
