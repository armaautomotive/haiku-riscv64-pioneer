/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeBriscImage.h"

static std::vector<uint8_t> read(const char* path)
{
	std::ifstream stream(path, std::ios::binary);
	assert(stream);
	return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

static uint32_t word(const std::vector<uint8_t>& bytes, uint32_t offset)
{
	assert((offset & 3) == 0 && uint64_t(offset) + 4 <= bytes.size());
	uint32_t value = 0;
	for (unsigned i = 0; i < 4; i++) value |= uint32_t(bytes[offset + i]) << (8 * i);
	return value;
}

static int32_t signedBits(uint32_t value, unsigned bits)
{
	assert(bits > 0 && bits < 32 && value < (uint32_t(1) << bits));
	return value & (uint32_t(1) << (bits - 1))
		? int32_t(int64_t(value) - (int64_t(1) << bits)) : int32_t(value);
}

int main(int argc, char** argv)
{
	assert(argc == 4);
	const auto boot = read(argv[1]), code = read(argv[2]);
	const auto triscBoot = read(argv[3]);
	assert(triscBoot.size() == 4 && word(triscBoot,0) == BlackholeRiscSmokeImageWord(0,true));
	assert(boot.size() == 4 && code.size() >= 4 && code.size() <= 128 && code.size() % 4 == 0);
	assert(word(boot, 0) == BlackholeBriscImageWord(0));
	assert(code.size() == 4 * TT_BRISC_CODE_WORDS);
	for (unsigned i = 1; i <= TT_BRISC_CODE_WORDS; i++) {
		assert(BlackholeBriscImageAddress(i) == 0x70000 + (i - 1) * 4);
		assert(word(code, (i - 1) * 4) == BlackholeBriscImageWord(i));
	}
	assert(BlackholeBriscImageAddress(0) == 0);
	assert(BlackholeBriscImageAddress(20) == 0x17ffc0 && BlackholeBriscImageAddress(21) == 0x17ffc4);
	assert(BlackholeBriscImageWord(20) == 0 && BlackholeBriscImageWord(21) == 0);
	assert(BlackholeBriscImageAddress(22) == UINT32_MAX);
	for (unsigned variant = 0; variant < 2; variant++) {
	const uint32_t entry = variant ? 0x6000 : 0;
	std::array<uint32_t, 32> regs = {};
	std::array<uint32_t, 2> mailbox = {};
	unsigned writes = 0, loads = 0, fences = 0, readConsumers = 0, parks = 0;
	uint32_t pc = entry;
	for (unsigned step = 0; step < 1000; step++) {
		assert(pc == entry || (pc >= 0x70000 && uint64_t(pc) + 4 <= 0x70000 + code.size()));
		uint32_t insn = pc == entry ? word(variant ? triscBoot : boot, 0) : word(code, pc - 0x70000);
		uint32_t opcode = insn & 0x7f, rd = (insn >> 7) & 31;
		uint32_t funct3 = (insn >> 12) & 7, rs1 = (insn >> 15) & 31, rs2 = (insn >> 20) & 31;
		uint32_t next = pc + 4, result = 0;
		bool writeReg = false;
		switch (opcode) {
			case 0x37: result = insn & 0xfffff000; writeReg = true; break; // LUI
			case 0x13: // ADDI only
				assert(funct3 == 0);
				result = regs[rs1] + signedBits(insn >> 20, 12); writeReg = true;
				if (rd == 0 && rs1 == 29 && (insn >> 20) == 0) readConsumers++;
				break;
			case 0x33: // ADD only
				assert(funct3 == 0 && (insn >> 25) == 0);
				result = regs[rs1] + regs[rs2]; writeReg = true; break;
			case 0x63: { // BLT, positive bounded counter
				assert(funct3 == 4 && regs[rs1] <= 33 && regs[rs2] == 33);
				uint32_t imm = ((insn >> 31) << 12) | (((insn >> 7) & 1) << 11)
					| (((insn >> 25) & 63) << 5) | (((insn >> 8) & 15) << 1);
				if (regs[rs1] < regs[rs2]) next = pc + signedBits(imm, 13);
				break;
			}
			case 0x23: { // SW: only checksum then completion, no MMIO
				assert(funct3 == 2 && writes < 2);
				uint32_t imm = ((insn >> 25) << 5) | ((insn >> 7) & 31);
				uint32_t address = regs[rs1] + signedBits(imm, 12);
				assert(address == 0x17ffc0 + 4 * writes);
				assert(regs[rs2] == (writes == 0 ? 528u : 0x50494f4eu));
				mailbox[writes++] = regs[rs2]; break;
			}
			case 0x03: { // LW: only each just-written result
				assert(funct3 == 2 && loads < 2 && writes == loads + 1 && rd == 29);
				uint32_t address = regs[rs1] + signedBits(insn >> 20, 12);
				assert(address == 0x17ffc0 + 4 * loads);
				result = mailbox[loads++]; writeReg = true; break;
			}
			case 0x0f: // FENCE, not FENCE.I or an unsupported extension
				assert(insn == 0x0ff0000f && loads == fences + 1 && readConsumers == loads);
				fences++; break;
			case 0x6f: { // JAL x0: reset jump or final self-loop only
				assert(rd == 0);
				uint32_t imm = ((insn >> 31) << 20) | (((insn >> 12) & 255) << 12)
					| (((insn >> 20) & 1) << 11) | (((insn >> 21) & 1023) << 1);
				next = pc + signedBits(imm, 21);
				if (pc == entry) assert(next == 0x70000);
				else { assert(pc == 0x70000 + code.size() - 4 && next == pc); parks++; }
				break;
			}
			default: assert(false && "Instruction outside the minimal RV32I allowlist");
		}
		if (writeReg && rd != 0) regs[rd] = result;
		regs[0] = 0; pc = next;
		if (parks == 8) break;
	}
	assert(parks == 8 && writes == 2 && loads == 2 && fences == 2 && readConsumers == 2);
	assert(mailbox[0] == 528 && mailbox[1] == 0x50494f4e);
	}
}
