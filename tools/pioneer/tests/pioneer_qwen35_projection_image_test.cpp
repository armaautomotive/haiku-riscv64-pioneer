/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SHARED_ROWS_MACHINE_ONLY
#include "pioneer_qwen38_rows_image_test.cpp"
#include "../PioneerQwen35Projection.h"
#include "../PioneerQwen35TiledReader.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <memory>
#include <unistd.h>

namespace Q35 = PioneerQwen35Projection;

static float DecodeQ5(const uint8_t* raw, unsigned index)
{
	const unsigned group = index / 32;
	const uint8_t* scales = raw + 4;
	unsigned scale = scales[group] & 63;
	unsigned minimum = scales[group + 4] & 63;
	if (group >= 4) {
		scale = ((scales[group - 4] >> 6) << 4) | (scales[group + 4] & 15);
		minimum = ((scales[group] >> 6) << 4) | (scales[group + 4] >> 4);
	}
	const unsigned lane = index % 32;
	const unsigned packed = raw[48 + (group / 2) * 32 + lane];
	const unsigned q = ((group & 1) ? packed >> 4 : packed & 15)
		+ ((raw[16 + lane] & (1U << group)) ? 16 : 0);
	float d = PioneerHalfFloat(uint16_t(raw[0] | (raw[1] << 8)));
	float m = PioneerHalfFloat(uint16_t(raw[2] | (raw[3] << 8)));
	return (d * scale) * q - (m * minimum);
}

struct Q35Machine : RowsMachine {
	std::vector<std::vector<uint8_t>> original;

	Q35Machine(const char* image, PioneerQwen35Gguf& model, unsigned ordinal, unsigned first)
		: RowsMachine(image, model.Tensors()[ordinal].type==13 ? 12 : model.Tensors()[ordinal].type,
			model.Tensors()[ordinal].inputs, 2)
	{
		const auto& tensor = model.Tensors()[ordinal];
		type = tensor.type;
		const unsigned blockBytes = PioneerQ35BlockBytes(type);
		const unsigned blocks = inputs / count;
		const unsigned rowBytes = unsigned(tensor.bytes / tensor.outputs);
		assert(rowBytes == blocks * blockBytes);
		tile.resize(Q35::TileBytes(type));
		tiles.assign(groups * blocks, tile);
		for (auto& bytes : tiles)
			std::fill(bytes.begin(), bytes.end(), 0);
		original.resize(groups * 16);
		for (unsigned row = 0; row < original.size(); row++) {
			original[row].resize(rowBytes);
			assert(model.ReadPacked(ordinal, uint64_t(first + row) * rowBytes,
				rowBytes, original[row].data(), rowBytes));
			for (unsigned b = 0; b < blocks; b++)
				for (unsigned byte = 0; byte < blockBytes; byte++)
					tiles[(row / 16) * blocks + b][byte * 16 + row % 16]
						= original[row][b * blockBytes + byte];
		}
		memory[0x74000] = Q35::Magic;
		memory[0x74004] = type;
		memory[0x74008] = tile.size();
		delay = 3;
	}

	bool RunProjection()
	{
		while (pc != 0x72038 && pc != 0x72058)
			StepBlock();
		assert(minStack >= 0x6e000);
		if (pc == 0x72058) {
			assert(l1[Index(0x17ffc8)] == Q35::Fault);
			return false;
		}
		assert(l1[Index(0x17ffc0)] == 0x7200c && l1[Index(0x17ffc8)] == Q35::Magic);
		return l1[Index(0x17ffc4)] == groups * (inputs / 16)
			&& requests == tiles.size() && responses == requests && !pending;
	}

	bool MatchesReference() const
	{
		if (matrixInstructions != 4 * groups * (inputs / 16))
			return false;
		for (unsigned row = 0; row < original.size(); row++) {
			double expected = 0, magnitude = 0;
			for (unsigned block = 0; block < inputs / count; block++) {
				const uint8_t* raw = original[row].data() + block * PioneerQ35BlockBytes(type);
				float values[256] = {};
				if (type == 13) {
					for (unsigned lane = 0; lane < count; lane++)
						values[lane] = DecodeQ5(raw, lane);
				} else {
					assert(type == 8 ? PioneerDecodeQ8_0(raw, values) : type == 12
						? PioneerDecodeQ4K(raw, values) : PioneerDecodeQ6K(raw, values));
				}
				for (unsigned lane = 0; lane < count; lane++) {
					const uint16_t a = vector[block * count + lane];
					const int q = int(a & 1023) * ((a & 0x8000) ? -1 : 1);
					const double term = double(values[lane]) * q * Float(scales[block]);
					expected += term;
					magnitude += fabs(term);
				}
			}
			const float actual = Float(buffers[4096 + row]);
			if (!isfinite(actual) || fabs(actual - expected) > 1e-6 + magnitude * 2e-6)
				return false;
		}
		for (unsigned row = original.size(); row < 4096; row++)
			if (buffers[4096 + row] != 0x7fc00000)
				return false;
		return true;
	}
};

int main(int argc, char** argv)
{
	assert(argc == 3);
	int fd = open(argv[2], O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
	assert(fd >= 0);
	std::unique_ptr<FILE, decltype(&fclose)> file(fdopen(fd, "rb"), &fclose);
	assert(file);
	std::string error;
	PioneerModelWeightCache::Digest digest{};
	assert(PioneerModelWeightCache::HashFile(fd, PioneerQwen35Gguf::FullBytes, digest, error));
	assert(PioneerModelWeightCache::Hex(digest)
		== "00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4");
	PioneerQwen35Gguf model;
	assert(model.Open(file.get()));
	std::set<std::pair<unsigned, unsigned>> covered;
	const unsigned pairs = 8;
	for (unsigned i = 0; i < model.Tensors().size(); i++) {
		const auto& tensor = model.Tensors()[i];
		if (!tensor.type)
			continue;
		assert(Q35::Supports(tensor.type, tensor.inputs));
		if (!covered.emplace(tensor.type, tensor.inputs).second)
			continue;
		Q35Machine m(argv[1], model, i, tensor.outputs - 32);
		assert(m.RunProjection() && m.MatchesReference());
		printf("Projection image replay passed Q%u K%u N%u with %u matrix instructions\n",
			m.type, m.inputs, m.groups * 16, m.matrixInstructions);
	}
	assert(covered.size() == pairs && model.Stable());
	unsigned q5 = model.Find("blk.0.attn_qkv.weight") - model.Tensors().data();
	for (unsigned field : {0x74000U, 0x74008U, 0x74014U, 0x74018U, 0x7401cU}) {
		Q35Machine m(argv[1], model, q5, 0);
		m.memory.at(field) ^= 1;
		if (field == 0x7401c)
			m.memory.at(field) = 17;
		assert(!m.RunProjection() && !m.requests && !m.matrixInstructions);
	}
	Q35Machine fault(argv[1], model, q5, 0);
	fault.badCounter = true;
	assert(!fault.RunProjection() && fault.requests == 1 && !fault.matrixInstructions);
	puts("Verified all pinned format/width pairs and preflight/fetch refusals in the software engine model.");
}
