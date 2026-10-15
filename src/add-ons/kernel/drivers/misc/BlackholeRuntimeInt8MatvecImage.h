/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_RUNTIME_INT8_MATVEC_IMAGE_H
#define BLACKHOLE_RUNTIME_INT8_MATVEC_IMAGE_H
#include "BlackholeRuntimeFp16MatvecImage.h"
// Independently assembled TT_RUNTIME_INT8 variant. The FP16 unpacker is
// only a bit overlay for signed integer operands. Force integer matrix
// arithmetic and converted signed-int32 destination readback. All addresses,
// NoC requests, four fidelity phases and completion/park markers are fixed.
static inline uint32_t BlackholeRuntimeInt8MatvecCodeWord(unsigned word)
{
	if (word==41) return 0xa02202b7; // ALU_ACC_CTRL_INT8_math_enabled (bit31)
	if (word==49) return 0x000102b7; // RISC_DEST_ACCESS_CTRL_SEC0_fmt = 1
	return BlackholeRuntimeFp16MatvecCodeWord(word);
}
#endif
