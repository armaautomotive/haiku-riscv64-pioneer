/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_INT8_UNPACK_H
#define PIONEER_INT8_UNPACK_H
#include <stdint.h>
// External INT8 unpack is sign-magnitude, not two's complement. Native
// signed unpack cannot encode -128. This distinct path refuses it; the
// existing expanded/table modes still support every signed-byte value.
static inline bool PioneerInt8UnpackEncode(int q,uint8_t& encoded)
{
	if (q < -127 || q > 127) return false;
	encoded=uint8_t(q<0?0x80|unsigned(-q):unsigned(q)); return true;
}
#endif
