/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_BLACKHOLE_SFP_LOAD_TEST_H
#define PIONEER_BLACKHOLE_SFP_LOAD_TEST_H
#include <cassert>
#include <stdint.h>
// Independent observer,ISA8a34e25e BlackholeA0/SFPLOAD.md,NOT Wormhole:
// mode5 preserves8 magnitude bits;deprecated mode13 preserves10 bits.
// BOTH return SIGN-MAGNITUDE on Blackhole,NOT two's complement.
static inline uint32_t BlackholeRawSfpLoad(uint16_t raw,unsigned mode)
{
	assert(mode==5 || mode==13);
	return (uint32_t(raw>>15)<<31)|((raw>>5)&(mode==5?255U:1023U));
}
#endif
