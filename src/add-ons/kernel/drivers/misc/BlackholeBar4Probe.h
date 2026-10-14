/* Distributed under the MIT License. */
#ifndef BLACKHOLE_BAR4_PROBE_H
#define BLACKHOLE_BAR4_PROBE_H

#include <stdint.h>

enum {
	TT_BAR4_REFUSED = 0,
	TT_BAR4_RESTORED = 1,
	TT_BAR4_RESTORE_FAILED = 2,
	TT_BAR4_INTERRUPTED = 3
};

struct tt_bar4_probe_result {
	uint32_t version;
	uint32_t state;
	uint32_t before[6];
	uint32_t trial[2];
	uint32_t after[6];
	uint32_t command_before;
	uint32_t command_after;
	uint64_t size[3];
};

// IO provides Read(offset, width), Write(offset, width, value), and Size(bar).
// This tests address-register retention only. It NEVER enables decoding/DMA,
// sizes a BAR, touches MMIO, changes a bridge, or modifies firmware.
template<class IO>
void
BlackholeBar4AddressProbe(IO& io, tt_bar4_probe_result& result)
{
	result = {};
	result.version = 1;
	if (io.Read(0, 4) != 0xb1401e52 || io.Read(0x2c, 4) != 0x00431e52)
		return;
	result.command_before = io.Read(4, 2);
	result.command_after = result.command_before;
	if (result.command_before != 0)
		return;
	for (unsigned i = 0; i < 6; i++) {
		result.before[i] = io.Read(0x10 + 4 * i, 4);
		result.after[i] = result.before[i];
		if (result.before[i] != (i % 2 == 0 ? 0x0cU : 0U))
			return;
	}
	for (unsigned i = 0; i < 3; i++)
		result.size[i] = io.Size(i * 2);
	if (result.size[0] != UINT64_C(0x20000000)
		|| result.size[1] != UINT64_C(0x100000)
		|| result.size[2] != UINT64_C(0x800000000))
		return;
	result.command_after = io.Read(4, 2);
	if (result.command_after != 0)
		return;

	// All address decoders are off. Ask which high bits the actual device
	// retains, then restore both words before reporting any result.
	io.Write(0x20, 4, 0x0c);
	io.Write(0x24, 4, 0xfffffff8);
	result.trial[0] = io.Read(0x20, 4);
	result.trial[1] = io.Read(0x24, 4);
	result.command_after = io.Read(4, 2);
	bool interrupted = result.command_after != 0;
	if (result.command_after != 0) {
		// An unexpected concurrent writer/removal is not a usable result.
		// Fail closed; never rewrite a BAR while decoding might be active.
		io.Write(4, 2, 0);
		result.command_after = io.Read(4, 2);
		if (result.command_after != 0) {
			result.state = TT_BAR4_RESTORE_FAILED;
			return;
		}
	}
	io.Write(0x24, 4, result.before[5]);
	io.Write(0x20, 4, result.before[4]);
	result.state = interrupted ? TT_BAR4_INTERRUPTED : TT_BAR4_RESTORED;
	for (unsigned i = 0; i < 6; i++) {
		result.after[i] = io.Read(0x10 + 4 * i, 4);
		if (result.after[i] != result.before[i])
			result.state = TT_BAR4_RESTORE_FAILED;
	}
	result.command_after = io.Read(4, 2);
	if (result.command_after != 0)
		result.state = TT_BAR4_RESTORE_FAILED;
}

#endif
