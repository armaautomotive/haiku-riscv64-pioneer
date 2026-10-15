/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTensixImage.h"

static uint32_t Bits(float f) { uint32_t u; memcpy(&u,&f,4); return u; }
static float Float(uint32_t u) { float f; memcpy(&f,&u,4); return f; }
static int32_t Sign(uint32_t u, unsigned bits) { return (int32_t)(u << (32-bits)) >> (32-bits); }
static std::vector<uint8_t> Read(const char* path) {
	std::ifstream file(path,std::ios::binary); assert(file.good());
	return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),{});
}
static uint32_t Word(const std::vector<uint8_t>& b, unsigned i) {
	assert(i + 4 <= b.size()); return b[i] | (uint32_t(b[i+1])<<8) | (uint32_t(b[i+2])<<16) | (uint32_t(b[i+3])<<24);
}

struct Machine {
	uint32_t regs[32] = {}, pc = 0x6000, l1[TT_TENSIX_IMAGE_WORDS] = {}, config[224] = {}, thread[68] = {};
	float dst[64][16] = {}, a[16][16] = {}, b[8][16] = {};
	float lreg[2] = {};
	bool valid[64], ownerA = false, ownerB = false, drained = true, pendingDrainStore = false;
	bool dropMatrix = false;
	const bool configSmoke;
	unsigned matrixInstructions = 0, drains = 0, configWrites = 0, inputWrites = 0, vectorWrites = 0, outputReads = 0, steps = 0;
	Machine(bool configOnly = false) : configSmoke(configOnly) {
		// Explicit pre-launch coprocessor reset reverts ZEROACC validity.
		for (unsigned i = 0; i < 64; i++) valid[i] = true;
		for (unsigned i = 0; i < TT_TENSIX_IMAGE_WORDS; i++) l1[i] = BlackholeTensixImageWord(i,configSmoke);
	}
	unsigned Index(uint32_t address) {
		for (unsigned i = 0; i < TT_TENSIX_IMAGE_WORDS; i++) if (BlackholeTensixImageAddress(i) == address) return i;
		assert(false); return 0;
	}
	void Instruction(uint32_t op) {
		unsigned opcode = op >> 24;
		if (configSmoke) assert(opcode == 0xb2 || op == 0x3700000f);
		if (opcode == 0xb2) {
			unsigned index = (op >> 16) & 255; assert(index < 68);
			thread[index] = op & 65535;
		} else if (op == 0x3700000f) {
			// All increments are zero and all RWCs are reset by this exact opcode.
			assert(thread[0] == 0);
		} else if (opcode == 0x71) {
			unsigned reg = (op >> 20) & 15;
			assert(reg < 2 && ((op >> 16) & 15) == 0);
			lreg[reg] = Float((op & 65535) << 16);
			assert(lreg[reg] == 1 || lreg[reg] == 2 || lreg[reg] == 3);
		} else if (opcode == 0x72) {
			unsigned reg = (op >> 20) & 15, address = op & 1023;
			assert(reg < 2 && (op & 0x000ffc00) == 0x00030000 && (address & 1) == 0);
			assert(address < 24 && config[6] == 0 && config[7] == 0 && thread[1] == 0 && thread[36] == 0);
			for (unsigned lane = 0; lane < 32; lane++) {
				unsigned row = (address & ~3U) + lane/8, col = 2*(lane&7) + ((address&2) != 0);
				assert(row < 24 && valid[row]);
				dst[row][col] = lreg[reg]; vectorWrites++;
			}
		} else if (opcode == 0x10) {
			assert((op & 0xffffff00) == 0x10000000 && (op & 255) >= 32 && (op & 255) < 40);
			valid[op & 255] = false;
		} else if (opcode == 0x08 || opcode == 0x0a) {
			assert(((op >> 12) & 3) == 2 && ((op >> 14) & 7) == 0);
			unsigned target = (op >> 17) & 63, row = op & 4095;
			assert((target & 3) == 0 && (row & 3) == 0 && target + 4 <= (opcode == 8 ? 16U : 8U));
			for (unsigned i = 0; i < 4; i++) for (unsigned col = 0; col < 16; col++) {
				assert(row+i < 24 && valid[row+i]);
				(opcode == 8 ? a[target+i][col] : b[target+i][col]) = dst[row+i][col];
			}
		} else if (op == 0x43000007) ownerA = true;
		else if (op == 0x43800007) ownerB = true;
		else if (op == 0x26000020) {
			assert(ownerA && ownerB && thread[2] == 1 && thread[3] == 1);
			assert(config[0] == 0x42b5 && config[1] == 0x20aa0000 && config[2] == 0 && config[3] == 0 && config[6] == 0 && config[7] == 0 && config[220] == 0);
			matrixInstructions++;
			for (unsigned row = 0; row < 8; row++) for (unsigned col = 0; col < 16; col++) {
				float value = valid[32+row] ? dst[32+row][col] : 0;
				if (!dropMatrix) for (unsigned k = 0; k < 16; k++) value += b[row][k] * a[k][col];
				dst[32+row][col] = value;
			}
			for (unsigned row = 32; row < 40; row++) valid[row] = true;
		} else assert(false);
		drained = false;
	}
	void Store(uint32_t address, uint32_t value) {
		assert((address & 3) == 0);
		if (address == 0xffe40000) { Instruction(value); return; }
		if (address == 0xffe80004) { assert(value == 0); pendingDrainStore = true; return; }
		if (address >= 0xffef0000 && address <= 0xffef0370) {
			assert(drained); unsigned offset = address-0xffef0000;
			assert(offset == 0 || offset == 4 || offset == 8 || offset == 12 || offset == 24 || offset == 28 || offset == 880);
			config[offset/4] = value; configWrites++; return;
		}
		if (address >= 0xffbd8000 && address < 0xffbd8600) {
			assert(false && "No direct RISC-to-Dst input stores in this image");
			assert(drained && config[3] == 0); unsigned index = (address-0xffbd8000)/4;
			assert(value == Bits(1) || value == Bits(2) || value == Bits(3));
			dst[index/16][index%16] = Float(value); inputWrites++; return;
		}
		assert((address >= 0x17f800 && address < 0x17fc00) || (address >= 0x17ffc0 && address <= 0x17ffc8));
		assert(drained || address == 0x17ffc0); // progress label precedes a completion wait
		l1[Index(address)] = value;
	}
	uint32_t Load(uint32_t address) {
		assert((address & 3) == 0);
		if (address == 0xffe80004) { assert(pendingDrainStore); pendingDrainStore = false; drained = true; drains++; return 0xdeadbeef; }
		if (address >= 0xffef0000 && address <= 0xffef0370) {
			assert(drained); unsigned offset = address-0xffef0000;
			assert(offset == 0 || offset == 4 || offset == 8 || offset == 12 || offset == 24 || offset == 28 || offset == 880);
			return config[offset/4];
		}
		if (address >= 0xffbd8800 && address < 0xffbd8a00) {
			assert(!configSmoke);
			assert(drained && matrixInstructions != 0); unsigned index = (address-0xffbd8000)/4;
			assert(valid[index/16]); outputReads++; return Bits(dst[index/16][index%16]);
		}
		assert(address == 0x17ffc0 || address == 0x17ffc4 || address == 0x17ffc8); return l1[Index(address)];
	}
	void Run() {
		while (pc != TT_TENSIX_PARK_PC || steps == 0) {
			assert(++steps < 20000 && (pc == 0x6000 || (pc >= 0x70000 && pc < 0x70000 + TT_TENSIX_CODE_WORDS*4)));
			uint32_t instruction = l1[Index(pc)], next = pc + 4;
			unsigned op = instruction & 127, rd = (instruction>>7)&31, f3 = (instruction>>12)&7;
			unsigned rs1 = (instruction>>15)&31, rs2 = (instruction>>20)&31;
			int32_t imm = Sign(instruction>>20,12);
			uint32_t result = 0; bool write = false;
			if (op == 0x37) { result = instruction & 0xfffff000; write = true; }
			else if (op == 0x13) {
				if (f3 == 0) result = regs[rs1] + imm;
				else { assert((f3 == 1 || f3 == 5) && (instruction>>25) == 0); result = f3 == 1 ? regs[rs1] << (instruction>>20 & 31) : regs[rs1] >> (instruction>>20 & 31); }
				write = true;
			} else if (op == 0x33) {
				assert((instruction>>25) == 0 && (f3 == 0 || f3 == 6));
				result = f3 == 0 ? regs[rs1] + regs[rs2] : regs[rs1] | regs[rs2]; write = true;
			} else if (op == 0x23) {
				assert(f3 == 2); imm = Sign(((instruction>>25)<<5) | (instruction>>7 & 31),12);
				Store(regs[rs1]+imm,regs[rs2]);
			} else if (op == 3) { assert(f3 == 2); result = Load(regs[rs1]+imm); write = true; }
			else if (op == 0x63) {
				assert(f3 == 0 || f3 == 1);
				imm = Sign(((instruction>>31)<<12) | ((instruction>>7 & 1)<<11) | ((instruction>>25 & 63)<<5) | ((instruction>>8 & 15)<<1),13);
				if ((regs[rs1] == regs[rs2]) == (f3 == 0)) next = pc + imm;
			} else if (op == 0x6f) {
				imm = Sign(((instruction>>31)<<20) | ((instruction>>12 & 255)<<12) | ((instruction>>20 & 1)<<11) | ((instruction>>21 & 1023)<<1),21);
				result = next; next = pc + imm; write = true;
			} else if (op == 0x67) { assert(f3 == 0); result = next; next = (regs[rs1]+imm) & ~1U; write = true; }
			else assert(instruction == 0x0ff0000f);
			if (write && rd != 0) regs[rd] = result;
			regs[0] = 0; pc = next;
		}
		assert(l1[Index(pc)] == 0x0000006f && drained);
		assert(l1[Index(0x17ffc0)] == TT_TENSIX_FINAL_PHASE && l1[Index(0x17ffc4)] == 2 && l1[Index(0x17ffc8)] == 0x4d415431);
	}
	bool Correct() {
		for (unsigned i = 0; i < 256; i++) if (l1[Index(0x17f800+i*4)] != (configSmoke ? 0U : Bits(i < 128 ? 16 : 96))) return false;
		return true;
	}
};
int main(int argc, char** argv)
{
	assert(argc == 4);
	auto boot = Read(argv[1]), code = Read(argv[2]);
	auto configCode = Read(argv[3]);
	assert(boot.size() == 4 && Word(boot,0) == BlackholeTensixImageWord(0) && code.size() == TT_TENSIX_CODE_WORDS*4);
	for (unsigned i = 0; i < TT_TENSIX_CODE_WORDS; i++) assert(Word(code,i*4) == BlackholeTensixImageWord(i+1));
	assert(configCode.size() == code.size());
	for (unsigned i = 0; i < TT_TENSIX_CODE_WORDS; i++) assert(Word(configCode,i*4) == BlackholeTensixImageWord(i+1,true));
	assert(BlackholeTensixImageAddress(TT_TENSIX_IMAGE_WORDS) == UINT32_MAX);
	Machine m; m.Run(); assert(m.Correct() && m.matrixInstructions == 2 && m.inputWrites == 0 && m.vectorWrites == 768 && m.outputReads == 256 && m.drains == 13);
	Machine disabled; disabled.dropMatrix = true; disabled.Run(); assert(!disabled.Correct());
	Machine configOnly(true); configOnly.Run();
	assert(configOnly.Correct() && configOnly.matrixInstructions == 0 && configOnly.inputWrites == 0 && configOnly.vectorWrites == 0 && configOnly.outputReads == 0 && configOnly.configWrites == 7 && configOnly.drains == 4);
	assert(configOnly.config[0] == 0x42b5 && configOnly.config[1] == 0x20aa0000 && configOnly.thread[2] == 1 && configOnly.thread[3] == 1);
	printf("Offline fixed image: %u steps, %u manual drains, two nonzero matrix fixtures; no hardware timing modeled\n",m.steps,m.drains);
}
