/* Distributed under the MIT License. */
#ifndef BLACKHOLE_PIONEER_MMIO_PROBE_H
#define BLACKHOLE_PIONEER_MMIO_PROBE_H

#include "PioneerMinimumBarLayout.h"

enum {
	TT_PIONEER_MMIO_REFUSED = 0,
	TT_PIONEER_MMIO_READ_OK = 1,
	TT_PIONEER_MMIO_READ_BAD = 2,
	TT_PIONEER_MMIO_MAP_FAILED = 3,
	TT_PIONEER_MMIO_CONFIG_FAILED = 4,
	TT_PIONEER_MMIO_RESTORE_FAILED = 5
};

struct tt_pioneer_mmio_result {
	uint32_t version;
	uint32_t state;
	uint32_t stage;
	uint32_t restored;
	int32_t profile_status;
	uint32_t profile_stage;
	uint32_t commands_before[2];
	uint32_t commands_after[2];
	uint32_t bars_before[6];
	uint32_t bars_trial[6];
	uint32_t bars_after[6];
	uint32_t bridge_before[3];
	uint32_t bridge_after[3];
	uint32_t node_id[3];
};

template<class IO>
bool
BlackholeCardControlsQuiet(IO& io)
{
	if ((io.CardRead(0x30, 4) & 1) != 0
		|| (io.CardRead(4, 4) & 0x00100000) == 0)
		return false;
	uint32_t cap = io.CardRead(0x34, 1);
	if (cap == 0)
		return false;
	uint64_t visited = 0;
	for (unsigned hops = 0; cap != 0 && hops < 48; hops++) {
		if (cap < 0x40 || cap > 0xfc || (cap & 3) != 0
			|| (visited & (UINT64_C(1) << (cap / 4))) != 0)
			return false;
		visited |= UINT64_C(1) << (cap / 4);
		uint32_t header = io.CardRead(cap, 2);
		uint32_t id = header & 0xff;
		if ((id == 5 && (io.CardRead(cap + 2, 2) & 1) != 0)
			|| (id == 0x11 && (io.CardRead(cap + 2, 2) & 0x8000) != 0))
			return false;
		if (id == 1 && (cap > 0xf8 || (io.CardRead(cap + 4, 2) & 3) != 0))
			return false;
		cap = (header >> 8) & 0xff;
	}
	return cap == 0;
}

// Strict, one-shot Pioneer experiment. Only memory + INTx-disable (0x402)
// may be enabled, NEVER bus mastering. All addressing is fixed and audited.
// IO maps exactly the BAR0 NoC identification page read-only and uncached.
template<class IO>
void
BlackholePioneerMmioProbe(IO& io, tt_pioneer_mmio_result& r)
{
	r = {};
	r.version = 1;
	r.profile_status = io.Profile().status;
	r.profile_stage = io.Profile().stage;
	if (!PioneerMinimumBarLayoutMatches(io.Profile()) || !io.Quiet()
		|| !BlackholeCardControlsQuiet(io)
		|| io.CardRead(0, 4) != 0xb1401e52 || io.CardRead(0x2c, 4) != 0x00431e52
		|| io.RootRead(0, 4) != 0x20421e30
		|| (io.RootRead(8, 4) >> 8) != 0x060400)
		return;
	r.commands_before[0] = io.CardRead(4, 2);
	r.commands_before[1] = io.RootRead(4, 2);
	if (r.commands_before[0] != 0 || r.commands_before[1] != 0
		|| io.RootRead(0x18, 4) != 0x003f0100
		|| io.RootRead(0x20, 4) != 0x0000fff0)
		return;
	for (unsigned i = 0; i < 6; i++) {
		r.bars_before[i] = io.CardRead(0x10 + 4 * i, 4);
		if (r.bars_before[i] != (i % 2 == 0 ? 0x0cU : 0U))
			return;
	}
	for (unsigned i = 0; i < 3; i++)
		r.bridge_before[i] = io.RootRead(0x24 + 4 * i, 4);
	if (r.bridge_before[0] != 0x0001fff1 || r.bridge_before[1] != 0
		|| r.bridge_before[2] != 0 || io.Size(0) != UINT64_C(0x20000000)
		|| io.Size(2) != UINT64_C(0x100000) || io.Size(4) != UINT64_C(0x800000000)
		|| io.CardRead(4, 2) != 0 || io.RootRead(4, 2) != 0)
		return;
	r.stage = 1;
	r.state = TT_PIONEER_MMIO_CONFIG_FAILED;
	const uint32_t trial[] = {0x0c, 0x49, 0x2000000c, 0x49, 0x0c, 0xfffffff8};
	const unsigned order[] = {5, 4, 1, 0, 3, 2};
	for (unsigned i : order)
		io.CardWrite(0x10 + 4 * i, 4, trial[i]);
	io.RootWrite(0x28, 4, 0x49);
	io.RootWrite(0x2c, 4, 0x49);
	io.RootWrite(0x24, 4, 0x20010001);
	bool mapAttempted = false;
	do {
		bool configured = true;
		for (unsigned i = 0; i < 6; i++) {
			r.bars_trial[i] = io.CardRead(0x10 + 4 * i, 4);
			configured &= r.bars_trial[i] == trial[i];
		}
		configured &= io.RootRead(0x24, 4) == 0x20010001
			&& io.RootRead(0x28, 4) == 0x49 && io.RootRead(0x2c, 4) == 0x49;
		if (!configured)
			break;
		mapAttempted = true;
		if (!io.Map()) {
			r.state = TT_PIONEER_MMIO_MAP_FAILED;
			break;
		}
		r.stage = 2;
		io.RootWrite(4, 2, 0x0402);
		if (io.RootRead(4, 2) != 0x0402)
			break;
		io.CardWrite(4, 2, 0x0402);
		if (io.RootRead(4, 2) != 0x0402 || io.CardRead(4, 2) != 0x0402)
			break;
		r.stage = 3;
		for (unsigned i = 0; i < 3; i++)
			r.node_id[i] = io.ReadNodeId();
		// NoC node ID contains other fields above the six-bit X/Y pair.
		// The two PCIe tiles are (2, 0) and (11, 0); keep the full raw word.
		uint32_t xy = r.node_id[0] & 0xfff;
		r.state = (xy == 2 || xy == 11) && r.node_id[0] == r.node_id[1]
			&& r.node_id[0] == r.node_id[2]
			? TT_PIONEER_MMIO_READ_OK : TT_PIONEER_MMIO_READ_BAD;
		if (r.state == TT_PIONEER_MMIO_READ_OK && !io.AfterNodeId(r.node_id[0]))
			r.state = TT_PIONEER_MMIO_READ_BAD;
		r.stage = 4;
	} while (false);

	bool unmapped = !mapAttempted || io.Unmap();
	io.CardWrite(4, 2, 0);
	io.RootWrite(4, 2, 0);
	bool decodersOff = io.CardRead(4, 2) == 0 && io.RootRead(4, 2) == 0;
	if (decodersOff) {
		for (unsigned i : order)
			io.CardWrite(0x10 + 4 * i, 4, r.bars_before[i]);
		io.RootWrite(0x28, 4, r.bridge_before[1]);
		io.RootWrite(0x2c, 4, r.bridge_before[2]);
		io.RootWrite(0x24, 4, r.bridge_before[0]);
	}
	bool restored = decodersOff && unmapped;
	for (unsigned i = 0; i < 6; i++) {
		r.bars_after[i] = io.CardRead(0x10 + 4 * i, 4);
		restored &= r.bars_after[i] == r.bars_before[i];
	}
	for (unsigned i = 0; i < 3; i++) {
		r.bridge_after[i] = io.RootRead(0x24 + 4 * i, 4);
		restored &= r.bridge_after[i] == r.bridge_before[i];
	}
	r.commands_after[0] = io.CardRead(4, 2);
	r.commands_after[1] = io.RootRead(4, 2);
	restored &= r.commands_after[0] == 0 && r.commands_after[1] == 0;
	r.restored = restored;
	if (!restored)
		r.state = TT_PIONEER_MMIO_RESTORE_FAILED;
	else
		r.stage = 5;
}

#endif
