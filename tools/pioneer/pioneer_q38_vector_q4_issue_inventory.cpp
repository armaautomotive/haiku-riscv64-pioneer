/* SPDX-License-Identifier: MIT */
// Host-only inventory of two isolated vector92 raw-Q4 helpers. This is
// deliberately partial: use the linked issue trace for the whole worker.
#include <cstdint>
#include <cstdio>
#include "PioneerQ38VectorQ4Metadata.h"
#include "PioneerQ38BlockAffineDst.h"
#include "PioneerQ38Q4Phases.h"

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
	// The linked vector92 worker applies the two nonzero Q4 fidelity phases
	// once per K16 panel. Q8 and Q6 still require four phases.
	for (unsigned panel = 0; panel < 16; ++panel)
		PioneerQ38QueuedPhases(phases, 12);
	if (phases.matrix != 32 || phases.drains != 16)
		return 1;
	const auto print = [](const char* name, const Counts& c) {
		std::printf("%s ops%llu configs%llu drains%llu matrix%llu\n", name,
			(unsigned long long)c.ops, (unsigned long long)c.configs,
			(unsigned long long)c.drains, (unsigned long long)c.matrix);
	};
	print("VECTOR92_Q4_METADATA", metadata);
	print("VECTOR92_Q4_MVMUL_PHASES_PER_TILE", phases);
	// This helper is replaced by Block above in vector92. In particular,
	// never add these two metadata paths together or call their sum a job.
	print("OLDER_Q4_AFFINE_DST_NOT_VECTOR92", affine);
	std::puts("PARTIAL_HOST_TEMPLATE_COUNTS_ONLY;NOT_WHOLE_WORKER_DEVICE_CYCLES_OR_TPS");
}
