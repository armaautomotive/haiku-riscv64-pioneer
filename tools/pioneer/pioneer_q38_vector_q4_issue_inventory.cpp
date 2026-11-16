/* SPDX-License-Identifier: MIT */
// Host-only inventory of the vector92 raw-Q4 decode/metadata schedule.
// Counts requested Tensix operations; it is not retired instructions, cycles,
// memory traffic, hardware utilization, or a speed measurement.
#include <cstdint>
#include <cstdio>
#include "PioneerQ38VectorQ4Metadata.h"
#include "PioneerQ38BlockAffineDst.h"
#include "PioneerQ38MatrixPhases.h"

struct Counts {
	uint64_t ops = 0, configs = 0, drains = 0, matrix = 0;
	void Op(unsigned value) {
		++ops;
		if ((value >> 24) == 0x26)
			++matrix;
	}
	void Config(unsigned, unsigned) { ++configs; }
	void Drain() { ++drains; }
};

int main() {
	Counts metadata;
	if (!PioneerQ38VectorQ4Metadata::Block(metadata, 0x90000, 0x3f800000))
		return 1;
	Counts affine;
	if (!PioneerQ38BlockAffineDst(affine))
		return 1;
	Counts phases;
	// The accepted Q4_K worker applies the same four-fidelity MVMUL sequence
	// once for each of the 16 K=16 panels in one K=256 quantization block.
	for (unsigned panel = 0; panel < 16; ++panel)
		PioneerQ38MatrixPhases(phases);
	Counts tile = metadata;
	tile.ops += affine.ops + phases.ops;
	tile.configs += affine.configs + phases.configs;
	tile.drains += affine.drains + phases.drains;
	tile.matrix += affine.matrix + phases.matrix;
	const auto print = [](const char* name, const Counts& c) {
		std::printf("%s ops%llu configs%llu drains%llu matrix%llu\n", name,
			(unsigned long long)c.ops, (unsigned long long)c.configs,
			(unsigned long long)c.drains, (unsigned long long)c.matrix);
	};
	print("VECTOR92_Q4_METADATA", metadata);
	print("VECTOR92_Q4_AFFINE_DST", affine);
	print("VECTOR92_Q4_K256_TILE", tile);
	print("VECTOR92_Q4_MVMUL_PHASES_PER_TILE", phases);
	std::puts("HOST_TEMPLATE_OPERATION_COUNTS_ONLY;NO_DEVICE_NO_CYCLE_OR_TPS_MEASUREMENT");
}
