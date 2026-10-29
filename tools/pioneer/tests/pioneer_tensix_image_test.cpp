/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_TENSIX_TEST_MACHINE_INCLUDED
#define PIONEER_TENSIX_TEST_MACHINE_INCLUDED
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iterator>
#include <vector>
#ifdef PIONEER_Q38_TEST_SHARED_WORKER_STORAGE
#include "PioneerQ38SharedTestStorage.h"
#endif
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTensixImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTensixTileData.h"

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
#ifdef PIONEER_Q38_TEST_SHARED_WORKER_STORAGE
	uint32_t regs[32] = {}, pc = 0x6000, config[224] = {}, thread[68] = {};
	::PioneerQ38SharedTestArray<0,0x180000/4> l1;
#else
	uint32_t regs[32] = {}, pc = 0x6000, l1[8192] = {}, config[224] = {}, thread[68] = {};
#endif
	float dst[64][16] = {}, a[16][16] = {}, b[8][16] = {};
	int32_t dstInt[64][16] = {};
	float lreg[2] = {};
	bool valid[64], ownerA = false, ownerB = false, drained = true, pendingDrainStore = false;
	bool dropMatrix = false;
	bool dropVector = false;
	bool dropClear = false;
	bool transposeA = false;
	bool dropLowFidelity = false;
	bool dropUnpack = false, backendWait = false;
	uint32_t gpr63 = 0;
	unsigned fidelityPhase=0,phaseVisits[4]={};
	unsigned unpackReads = 0, inputStores = 0;
	unsigned adcX0[2] = {}, adcX1[2] = {};
	bool UnpackBoundsValid() { return adcX0[0] == 0 && adcX1[0] == 255 && adcX0[1] == 0 && adcX1[1] == 127; }
	const bool configSmoke;
	const unsigned sfpuSmoke;
	unsigned matrixInstructions = 0, drains = 0, configWrites = 0, inputWrites = 0, vectorWrites = 0, outputReads = 0, steps = 0;
	unsigned moveInstructions = 0;
	Machine(bool configOnly = false, unsigned sfpuOnly = 0) : configSmoke(configOnly), sfpuSmoke(sfpuOnly) {
		// Explicit pre-launch coprocessor reset reverts ZEROACC validity.
		for (unsigned i = 0; i < 64; i++) valid[i] = true;
		for (unsigned i = 0; i < BlackholeTensixImageWords(sfpuSmoke); i++) l1[i] = BlackholeTensixImageWord(i,configSmoke,sfpuSmoke);
	}
	virtual ~Machine() = default;
	virtual bool Fp16Operands() const { return false; }
	virtual bool Int8Arithmetic() const { return false; }
	virtual bool Int8ConvertedRead() const { return Int8Arithmetic(); }
	virtual bool NativeInt8Unpack() const { return false; }
	virtual unsigned MatrixFidelityStep() const { return 1; }
	virtual bool InlineTensix() const { return false; }
	virtual uint32_t MatrixArithmeticConfig1() const { return config[1]; }
	virtual bool FloatFidelityArithmetic() const { return sfpuSmoke==24; }
	virtual uint32_t NativePayloadBase(unsigned half) const { return 0x706e0+half*256; }
	virtual uint32_t SourceBPayloadBase() const { return 0x70920; }
	virtual uint32_t SourceBOperandWord(uint32_t address) { return l1[Index(address)]; }
	virtual float Operand16(uint16_t half) const { return Float(uint32_t(half)<<16); }
	virtual unsigned Index(uint32_t address) {
		for (unsigned i = 0; i < BlackholeTensixImageWords(sfpuSmoke); i++) if (BlackholeTensixImageAddress(i,sfpuSmoke) == address) return i;
		assert(false); return 0;
	}
	virtual void Instruction(uint32_t op) {
		unsigned opcode = op >> 24;
		if (configSmoke) assert(opcode == 0xb2 || op == 0x3700000f);
		if (sfpuSmoke == 1 || sfpuSmoke == 2) assert(opcode == 0xb2 || opcode == 0x71 || (sfpuSmoke == 2 && opcode == 0x72) || op == 0x3700000f);
		if (sfpuSmoke == 3) assert(opcode != 0x26);
		if (sfpuSmoke == 5) assert(opcode == 0xb2 || opcode == 0x71 || opcode == 0x72 || opcode == 0x10 || op == 0x3700000f);
		if (sfpuSmoke == 6) assert(opcode != 0x26 && opcode != 0x43);
		if (sfpuSmoke == 10) assert(opcode == 0xb2 || opcode == 0x71 || opcode == 0x72 || op == 0x3700000f);
		if (sfpuSmoke == 19) assert(opcode == 0xb2 || op == 0x5e23fc00 || op == 0x5e41fc00 || op == 0xa2101816 || op == 0x4500017e);
		if (opcode == 0xb2) {
			unsigned index = (op >> 16) & 255; assert(index < 68);
			thread[index] = op & 65535;
		} else if (op == 0xa2101816) {
			assert(sfpuSmoke >= 16 && gpr63 == 0); backendWait = true;
		} else if (op == 0x4500017e) {
			assert(sfpuSmoke >= 16 && backendWait && gpr63 == 0);
			backendWait = false; gpr63 = 1; drained = true; drains++; return;
		} else if (op == 0x5e23fc00 || op == 0x5e41fc00) {
			assert(sfpuSmoke >= 16);
			unsigned which = op == 0x5e23fc00 ? 0 : 1;
			adcX0[which] = op & 1023; adcX1[which] = (op >> 10) & 1023;
		} else if (op == 0x5160000a || op == 0x5160000f) {
			assert(sfpuSmoke >= 16);
			for (unsigned which = 0; which < 2; which++) {
				if (op & 1) adcX0[which] = 0;
				if (op & 4) adcX1[which] = 0;
			}
		} else if (op == 0x5460000f) {
			assert(sfpuSmoke >= 16);
		} else if (op == 0x420000c1 || op == 0x428000c1) {
			assert(sfpuSmoke >= 16 && sfpuSmoke != 19 && config[50] == 0x100 && config[56] == 0x100000 && config[59] == 0x100);
			bool srcA = op == 0x420000c1;
			bool native=srcA && NativeInt8Unpack();
			assert(UnpackBoundsValid()); // an X1 reset would load just one datum
			assert(srcA ? !ownerA : !ownerB);
			assert(config[srcA ? 64 : 112] == (native?0x10001eU:Fp16Operands()?0x100011U:0x100015U) && config[srcA ? 65 : 113] == (srcA ? 16U : 8U));
			assert(config[srcA ? 72 : 120] == (native?14U:Fp16Operands()?1U:5U) && config[srcA ? 73 : 121] == 3);
			assert(native?((config[76]+1)*16==NativePayloadBase(0) || (config[76]+1)*16==NativePayloadBase(1)):config[srcA ? 76 : 124] == (srcA ? 0x7070U : (SourceBPayloadBase()-16)/16));
			if (native) assert(!(config[1]&0x8000) && config[49]==0 && (config[84]&65535)==64); // signed, row0 aligned for x8 bursts
			unsigned base = native?(config[76]+1)*16:srcA ? 0x70710 : SourceBPayloadBase(), rows = srcA ? 16 : 8;
			for (unsigned row = 0; row < rows; row++) for (unsigned col = 0; col < 16; col++) {
				unsigned element = row*16+col;
				unsigned address=base+(native?element/4:element/2)*4;
				uint32_t word=srcA?l1[Index(address)]:SourceBOperandWord(address);
				unsigned byte=native?((word>>(8*(element&3)))&255):0;
				float value = native?float(byte&127)*((byte&128)?-1:1):Operand16(uint16_t(word >> (16*(element&1))));
				if (!dropUnpack) (srcA ? a[row][col] : b[row][col]) = value;
				unpackReads++;
			}
			if (srcA) ownerA = true; else ownerB = true;
		} else if (op == 0x36c00000) {
			assert((sfpuSmoke == 17 || sfpuSmoke == 18 || sfpuSmoke == 22 || sfpuSmoke == 23) && ownerA && ownerB);
			ownerA = ownerB = false;
		} else if (op == 0x3700000f) {
			// All increments are zero and all RWCs are reset by this exact opcode.
			assert(thread[0] == 0);
			fidelityPhase=0;
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
				if (!dropVector) dst[row][col] = lreg[reg]; vectorWrites++;
			}
		} else if (opcode == 0x10) {
			if (op == 0x10180000) {
				assert(sfpuSmoke >= 16);
				if (!dropClear) for (unsigned row = 0; row < 64; row++) valid[row] = false;
			} else if (op==0x10280002) {
				// Mode16_ROWS=1,UseDst32b=1,Imm10=2:logical32..47.
				assert(sfpuSmoke>=16);
				if (!dropClear) for (unsigned row=32;row<48;row++) valid[row]=false;
			} else {
				assert((op & 0xffffff00) == 0x10000000 && (op & 255) >= 32 && (op & 255) < 40);
				valid[op & 255] = false;
			}
		} else if (opcode == 0x08 || opcode == 0x0a) {
			if (sfpuSmoke >= 7) assert(ownerA && ownerB);
			assert(((op >> 12) & 3) == 2 && ((op >> 14) & 7) == 0);
			unsigned target = (op >> 17) & 63, row = op & 4095;
			assert((target & 3) == 0 && (row & 3) == 0 && target + 4 <= (opcode == 8 ? 16U : 8U));
			for (unsigned i = 0; i < 4; i++) for (unsigned col = 0; col < 16; col++) {
				assert(row+i < 24 && valid[row+i]);
				(opcode == 8 ? a[target+i][col] : b[target+i][col]) = dst[row+i][col];
			}
		} else if (op == 0x430000c2 || op == 0x438000c2) {
			assert(sfpuSmoke >= 13);
			if (op == 0x430000c2) { assert(!ownerA); ownerA = true; }
			else { assert(!ownerB); ownerB = true; }
		} else if (op == 0x43000101) {
			assert(sfpuSmoke >= 7 && !ownerA);
			memset(a,0,sizeof(a)); ownerA = true;
		} else if (op == 0x43800101) {
			assert(sfpuSmoke >= 7 && !ownerB);
			memset(b,0,sizeof(b)); ownerB = true;
		} else if (op == 0x43000007) ownerA = true;
		else if (op == 0x43800007) ownerB = true;
		else if (op == 0x12002020 || op == 0x13001020) {
			assert((op == 0x12002020 && (sfpuSmoke == 11 || sfpuSmoke == 13 || sfpuSmoke == 17 || sfpuSmoke == 22) && ownerA) || (op == 0x13001020 && (sfpuSmoke == 12 || sfpuSmoke == 14 || sfpuSmoke == 18 || sfpuSmoke == 23) && ownerB));
			for (unsigned row = 0; row < 8; row++) {
				for (unsigned col = 0; col < 16; col++) dst[32+row][col] = op == 0x12002020 ? a[row][col] : b[0][col];
				valid[32+row] = true;
			}
			moveInstructions++;
		}
		else if ((op&~0x4000U) == 0x26000020 || (op&~0x4000U) == 0x26c00020) {
			unsigned modifier=(op>>14)&3; assert(modifier<=1);
			if (modifier) assert(thread[13]==0 && thread[29]==MatrixFidelityStep()*0x2000 && thread[48]==0);
			unsigned phase=(thread[11]+fidelityPhase)&3; phaseVisits[phase]++;
			assert(ownerA && ownerB && thread[2] == 1 && thread[3] == 1);
			assert(config[0] == (NativeInt8Unpack()?0x423eU:Fp16Operands()?0x4231U:0x42b5U) && MatrixArithmeticConfig1() == (NativeInt8Unpack()?0xa03c0000U:Int8Arithmetic()?0xa0220000U:Fp16Operands()?0x20220000U:0x20aa0000U) && config[2] == 0 && config[3] == (Int8ConvertedRead()?0x10000U:0U) && config[6] == 0 && config[7] == 0 && config[220] == 0);
			matrixInstructions++;
			for (unsigned row = 0; row < 8; row++) for (unsigned col = 0; col < 16; col++) {
				float value = valid[32+row] ? dst[32+row][col] : 0;
				int64_t integerValue=valid[32+row]?dstInt[32+row][col]:0;
				if (!dropMatrix) for (unsigned k = 0; k < 16; k++) {
					float av = transposeA ? a[col][k] : a[k][col], bv = b[row][k];
					if (Int8Arithmetic()) {
						int32_t ai=int32_t(fabsf(av))&(phase&1?31:224),bi=int32_t(fabsf(bv))&(phase&2?15:1008);
						if (dropLowFidelity && phase!=0) continue;
						integerValue+=(av<0?-ai:ai)*(bv<0?-bi:bi); continue;
					}
					// The documented FP16 SrcA fidelity phases do not consume
					// its least significant mantissa bit. SrcB keeps all ten.
					if (Fp16Operands()) av=Float(Bits(av)&~0x2000U);
					if (FloatFidelityArithmetic()) {
						float highA = Float(Bits(av)&0xfff80000), highB = Float(Bits(bv)&0xfffe0000);
						av = (phase&1) ? av-highA : highA;
						bv = (phase&2) ? bv-highB : highB;
						if (dropLowFidelity && phase != 0) av = 0;
					}
					value += bv*av;
				}
				dst[32+row][col] = value;
				assert(integerValue>=INT32_MIN && integerValue<=INT32_MAX);
				dstInt[32+row][col]=int32_t(integerValue);
			}
			for (unsigned row = 32; row < 40; row++) valid[row] = true;
			if ((op&~0x4000U) == 0x26c00020) { assert(sfpuSmoke == 16 || sfpuSmoke == 20 || sfpuSmoke == 21 || sfpuSmoke == 24); ownerA = ownerB = false; }
			if (modifier) fidelityPhase=(fidelityPhase+((thread[29]>>13)&3))&3;
		} else assert(false);
		drained = false;
	}
	virtual void Store(uint32_t address, uint32_t value) {
		assert((address & 3) == 0);
		if (address == 0xffe40000) { Instruction(value); return; }
		if (address == 0xffe80004) { assert(value == 0); pendingDrainStore = true; return; }
		if (address == 0xffe000fc) { assert(sfpuSmoke >= 16 && value == 0); gpr63 = 0; return; }
		if (address >= 0x70700 && address < 0x70a20) {
			assert(sfpuSmoke >= 16 && sfpuSmoke != 19);
			bool header = address < 0x70710 || (address >= 0x70910 && address < 0x70920);
			if (sfpuSmoke >= 20 && !header) {
				assert(address >= 0x70920); // preset A and lookup tables are read-only
			} else assert(header ? value == 0 : (value == 0x3f803f80 || value == 0x40004000 || value == 0x40404040));
			l1[Index(address)] = value; inputStores++; return;
		}
		if (address >= 0xffef0000 && address <= 0xffef0370) {
			assert(drained); unsigned offset = address-0xffef0000;
			assert(ConfigOffset(offset));
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
	virtual void Store16(uint32_t,uint16_t) { assert(false && "half store not allowed in this image"); }
	virtual uint32_t Load(uint32_t address) {
		assert((address & 3) == 0);
		if (address == 0xffe80004) { assert(pendingDrainStore); pendingDrainStore = false; drained = true; drains++; return 0xdeadbeef; }
		if (address == 0xffe000fc) { assert(sfpuSmoke >= 16); return gpr63; }
		if (address == 0x70a1c) { assert(sfpuSmoke >= 16 && sfpuSmoke != 19); return l1[Index(address)]; }
		if (address >= 0x70500 && address < 0x70700) { assert(sfpuSmoke >= 20); return l1[Index(address)]; }
		if (address >= 0xffef0000 && address <= 0xffef0370) {
			assert(drained); unsigned offset = address-0xffef0000;
			assert(ConfigOffset(offset));
			return config[offset/4];
		}
		if (address >= 0xffbd8800 && address < 0xffbd8a00) {
			assert(!configSmoke);
			assert(drained && (matrixInstructions != 0 || moveInstructions != 0)); unsigned index = (address-0xffbd8000)/4;
			assert(valid[index/16]); outputReads++;
			return Int8ConvertedRead()?uint32_t(dstInt[index/16][index%16]):Bits(dst[index/16][index%16]);
		}
		if (address >= 0xffbd8000 && address < 0xffbd8200) {
			assert(sfpuSmoke == 10 && drained && matrixInstructions == 0);
			unsigned index = (address-0xffbd8000)/4;
			assert(valid[index/16]); outputReads++; return Bits(dst[index/16][index%16]);
		}
		assert(address == 0x17ffc0 || address == 0x17ffc4 || address == 0x17ffc8); return l1[Index(address)];
	}
	bool ConfigOffset(unsigned offset) {
		if (offset == 0 || offset == 4 || offset == 8 || offset == 12 || offset == 24 || offset == 28 || offset == 880) return true;
		if (sfpuSmoke < 16) return false;
		const unsigned registers[] = {20,24,28,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
		for (unsigned reg : registers) if (offset == reg*4) return true;
		return false;
	}
	virtual unsigned StepLimit() const { return 20000; }
	// Explicit diagnostic subclasses ONLY. Ordinary image models reject
	// even read-only CSR access;never emulate arbitrary CSR writes.
	virtual uint32_t ReadCycleCsr(unsigned csr) { (void)csr; assert(false && "unexpected cycle CSR"); return 0; }
	virtual uint32_t ParkPc() const { return BlackholeTensixParkPc(sfpuSmoke); }
	virtual uint32_t FinalPhase() const { return BlackholeTensixFinalPhase(sfpuSmoke); }
	virtual uint32_t FinalMarker() const { return 0x4d415431; }
	virtual unsigned ExpectedFixtures() const { return 2; }
	virtual bool CodeAddress(uint32_t address) const {
		return address==0x6000 || (address>=0x70000
			&& address<0x70000+BlackholeTensixCodeWords(sfpuSmoke)*4
			&& (sfpuSmoke<16 || address<0x70700) && (sfpuSmoke<20 || address<0x70500));
	}
	// Single instruction entry also permits persistent-kernel tests to keep
	// ALL coprocessor/RISC state alive across job boundaries, without reset.
	void Step() {
			++steps;
			if (steps >= StepLimit() || !CodeAddress(pc))
				fprintf(stderr,"RISC execution bound: pc=%08x steps=%u limit=%u allowed=%u\n",pc,steps,StepLimit(),CodeAddress(pc));
			assert(steps < StepLimit() && CodeAddress(pc));
			uint32_t instruction = l1[Index(pc)], next = pc + 4;
			unsigned op = instruction & 127, rd = (instruction>>7)&31, f3 = (instruction>>12)&7;
			unsigned rs1 = (instruction>>15)&31, rs2 = (instruction>>20)&31;
			int32_t imm = Sign(instruction>>20,12);
			uint32_t result = 0; bool write = false;
			if ((instruction&3)!=3) {
				if (!InlineTensix()) fprintf(stderr,"Unexpected non-RISC instruction %08x at %08x\n",instruction,pc);
				assert(InlineTensix()); Instruction((instruction>>2)|(instruction<<30));
			}
			else if (op == 0x37) { result = instruction & 0xfffff000; write = true; }
			else if (op == 0x17) { result = pc+(instruction&0xfffff000); write = true; }
			else if (op == 0x13) {
				if (f3 == 0) result = regs[rs1] + imm;
				else if (f3 == 2) result = int32_t(regs[rs1]) < imm;
				else if (f3 == 3) result = regs[rs1] < uint32_t(imm);
				else if (f3 == 4) result = regs[rs1] ^ imm;
				else if (f3 == 6) result = regs[rs1] | imm;
				else if (f3 == 7) result = regs[rs1] & imm;
				else { unsigned high=instruction>>25; assert((f3==1 && !high) || (f3==5 && (high==0 || high==32)));
					result=f3==1?regs[rs1]<<(rs2&31):high?uint32_t(int32_t(regs[rs1])>>(rs2&31)):regs[rs1]>>(rs2&31); }
				write = true;
			} else if (op == 0x33) {
				unsigned high=instruction>>25;
				if (high==1) { assert(!f3); result=uint32_t(uint64_t(regs[rs1])*regs[rs2]); }
				else {
					assert(high==0 || (high==32 && (f3==0 || f3==5)));
					switch (f3) {
						case 0: result=high?regs[rs1]-regs[rs2]:regs[rs1]+regs[rs2]; break;
						case 1: result=regs[rs1]<<(regs[rs2]&31); break;
						case 2: result=int32_t(regs[rs1])<int32_t(regs[rs2]); break;
						case 3: result=regs[rs1]<regs[rs2]; break;
						case 4: result=regs[rs1]^regs[rs2]; break;
						case 5: result=high?uint32_t(int32_t(regs[rs1])>>(regs[rs2]&31)):regs[rs1]>>(regs[rs2]&31); break;
						case 6: result=regs[rs1]|regs[rs2]; break;
						case 7: result=regs[rs1]&regs[rs2]; break;
						default: assert(false);
					}
				} write=true;
			} else if (op == 0x23) {
				assert(f3 == 1 || f3 == 2); imm = Sign(((instruction>>25)<<5) | (instruction>>7 & 31),12);
				if (f3==1) Store16(regs[rs1]+imm,uint16_t(regs[rs2])); else Store(regs[rs1]+imm,regs[rs2]);
			} else if (op == 3) {
				assert(f3==2 || f3==4 || f3==5); uint32_t address=regs[rs1]+imm;
				if (f3==2) result=Load(address);
				else { assert(f3!=5 || !(address&1)); result=(Load(address&~3U)>>(8*(address&3)))&(f3==4?255:65535); }
				write=true;
			}
			else if (op == 0x63) {
				imm = Sign(((instruction>>31)<<12) | ((instruction>>7 & 1)<<11) | ((instruction>>25 & 63)<<5) | ((instruction>>8 & 15)<<1),13);
				bool take=false;
				switch (f3) {
					case 0: take=regs[rs1]==regs[rs2]; break; case 1: take=regs[rs1]!=regs[rs2]; break;
					case 4: take=int32_t(regs[rs1])<int32_t(regs[rs2]); break; case 5: take=int32_t(regs[rs1])>=int32_t(regs[rs2]); break;
					case 6: take=regs[rs1]<regs[rs2]; break; case 7: take=regs[rs1]>=regs[rs2]; break; default: assert(false);
				}
				if (take) next=pc+imm;
			} else if (op == 0x6f) {
				imm = Sign(((instruction>>31)<<20) | ((instruction>>12 & 255)<<12) | ((instruction>>20 & 1)<<11) | ((instruction>>21 & 1023)<<1),21);
				result = next; next = pc + imm; write = true;
			} else if (op == 0x67) { assert(f3 == 0); result = next; next = (regs[rs1]+imm) & ~1U; write = true; }
			else if (op==0x73) {
				assert(f3==2 && rs1==0 && ((instruction>>20)==0xb00 || (instruction>>20)==0xb80));
				result=ReadCycleCsr(instruction>>20); write=true;
			}
			else assert(instruction == 0x0ff0000f);
			if (write && rd != 0) regs[rd] = result;
			regs[0] = 0; pc = next;
	}
	void Run() {
		while (pc != ParkPc() || steps == 0) Step();
		assert(l1[Index(pc)] == 0x0000006f && drained);
		assert(l1[Index(0x17ffc0)] == FinalPhase() && l1[Index(0x17ffc4)] == ExpectedFixtures() && l1[Index(0x17ffc8)] == FinalMarker());
	}
	bool Correct() {
		for (unsigned i = 0; i < 256; i++) {
			uint32_t expected = ((sfpuSmoke >= 10 && sfpuSmoke <= 14) || sfpuSmoke == 17 || sfpuSmoke == 18) ? Bits(i < 128 ? 1 : (sfpuSmoke == 12 || sfpuSmoke == 14 || sfpuSmoke == 18) ? 3 : 2) : (configSmoke || sfpuSmoke == 19 || (sfpuSmoke != 0 && sfpuSmoke != 7 && sfpuSmoke < 10)) ? 0U : Bits(i < 128 ? 16 : 96);
			if (sfpuSmoke >= 20) {
				unsigned fixture = i/128, row = (i%128)/16, column = i%16;
				int sum = 0;
				for (unsigned k = 0; k < 16; k++) {
					int vector = int((3*row+k)%16)-8; if (fixture) vector = -vector-1;
					int weight = sfpuSmoke == 21 ? int((k+2*column)%5)-2 : (k == column ? 1 : 0);
					sum += vector*weight;
				}
				if (sfpuSmoke == 22) sum = int((row+2*column)%5)-2;
				if (sfpuSmoke == 23) { sum = int(column)-8; if (fixture) sum = -sum-1; }
				expected = Bits(float(sum));
			}
			if (l1[Index(0x17f800+i*4)] != expected) return false;
		}
		return true;
	}
};
#ifndef PIONEER_TENSIX_MACHINE_ONLY
int main(int argc, char** argv)
{
	assert(argc == 28);
	auto boot = Read(argv[1]), code = Read(argv[2]);
	auto configCode = Read(argv[3]);
	assert(boot.size() == 4 && Word(boot,0) == BlackholeTensixImageWord(0) && code.size() == TT_TENSIX_CODE_WORDS*4);
	for (unsigned i = 0; i < TT_TENSIX_CODE_WORDS; i++) assert(Word(code,i*4) == BlackholeTensixImageWord(i+1));
	assert(configCode.size() == code.size());
	for (unsigned i = 0; i < TT_TENSIX_CODE_WORDS; i++) assert(Word(configCode,i*4) == BlackholeTensixImageWord(i+1,true));
	auto loadCode = Read(argv[4]); assert(loadCode.size() == code.size());
	for (unsigned i = 0; i < TT_TENSIX_CODE_WORDS; i++) assert(Word(loadCode,i*4) == BlackholeTensixImageWord(i+1,false,true));
	auto storeCode = Read(argv[5]); assert(storeCode.size() == code.size());
	for (unsigned i = 0; i < TT_TENSIX_CODE_WORDS; i++) assert(Word(storeCode,i*4) == BlackholeTensixImageWord(i+1,false,2));
	for (unsigned stage = 3; stage <= 23; stage++) {
		auto bytes = Read(argv[stage+3]); assert(bytes.size() == BlackholeTensixCodeWords(stage)*4);
		for (unsigned i = 0; i < BlackholeTensixCodeWords(stage); i++) {
			if (Word(bytes,i*4) != BlackholeTensixImageWord(i+1,false,stage))
				fprintf(stderr,"Stage %u address %x: assembly %08x header %08x\n",stage,0x70000+i*4,Word(bytes,i*4),BlackholeTensixImageWord(i+1,false,stage));
			assert(Word(bytes,i*4) == BlackholeTensixImageWord(i+1,false,stage));
		}
	}
	assert(BlackholeTensixImageAddress(TT_TENSIX_IMAGE_WORDS) == UINT32_MAX);
	tt_tensix_tile_data assembledInput = {}; assembledInput.version = 1;
	for (unsigned i = 0; i < 256; i++) {
		assembledInput.weights[i] = Bits(i/16 == i%16 ? 1.0f : 0.0f)>>16;
		assembledInput.activations[i] = BlackholeFixedIntegerFp32(BlackholeFixedVector(i/128,(i%128)/16,i%16))>>16;
	}
	auto dataCode = Read(argv[27]); assert(dataCode.size() == TT_UNPACK_CODE_WORDS*4);
	for (unsigned i = 0; i < TT_UNPACK_CODE_WORDS; i++) assert(Word(dataCode,i*4) == BlackholeTileDataImageWord(i+1,assembledInput));
	Machine m; m.Run(); assert(m.Correct() && m.matrixInstructions == 2 && m.inputWrites == 0 && m.vectorWrites == 768 && m.outputReads == 256 && m.drains == 13);
	Machine disabled; disabled.dropMatrix = true; disabled.Run(); assert(!disabled.Correct());
	Machine configOnly(true); configOnly.Run();
	assert(configOnly.Correct() && configOnly.matrixInstructions == 0 && configOnly.inputWrites == 0 && configOnly.vectorWrites == 0 && configOnly.outputReads == 0 && configOnly.configWrites == 7 && configOnly.drains == 4);
	assert(configOnly.config[0] == 0x42b5 && configOnly.config[1] == 0x20aa0000 && configOnly.thread[2] == 1 && configOnly.thread[3] == 1);
	Machine loadOnly(false,true); loadOnly.Run();
	assert(loadOnly.Correct() && loadOnly.lreg[0] == 1 && loadOnly.lreg[1] == 1 && loadOnly.matrixInstructions == 0
		&& loadOnly.vectorWrites == 0 && loadOnly.outputReads == 0 && loadOnly.drains == 5);
	Machine storeOnly(false,2); storeOnly.Run();
	assert(storeOnly.Correct() && storeOnly.matrixInstructions == 0 && storeOnly.vectorWrites == 384 && storeOnly.outputReads == 0 && storeOnly.drains == 5);
	for (unsigned row = 0; row < 24; row++) for (unsigned col = 0; col < 16; col++) assert(storeOnly.dst[row][col] == 1);
	Machine operandOnly(false,3); operandOnly.Run();
	assert(operandOnly.Correct() && operandOnly.matrixInstructions == 0 && operandOnly.vectorWrites == 384 && operandOnly.outputReads == 0 && operandOnly.drains == 8);
	Machine matrixOnly(false,4); matrixOnly.Run();
	assert(matrixOnly.Correct() && matrixOnly.matrixInstructions == 1 && matrixOnly.vectorWrites == 384 && matrixOnly.outputReads == 0 && matrixOnly.drains == 9);
	Machine zeroOnly(false,5); zeroOnly.Run();
	assert(zeroOnly.Correct() && zeroOnly.matrixInstructions == 0 && zeroOnly.vectorWrites == 384 && zeroOnly.outputReads == 0 && zeroOnly.drains == 6 && !zeroOnly.ownerA && !zeroOnly.ownerB);
	Machine movdOnly(false,6); movdOnly.Run();
	assert(movdOnly.Correct() && movdOnly.matrixInstructions == 0 && movdOnly.vectorWrites == 384 && movdOnly.outputReads == 0 && movdOnly.drains == 7 && !movdOnly.ownerA && !movdOnly.ownerB);
	Machine kernel(false,7); kernel.Run();
	assert(kernel.Correct() && kernel.matrixInstructions == 2 && kernel.vectorWrites == 768 && kernel.outputReads == 256);
	Machine brokenKernel(false,7); brokenKernel.dropMatrix = true; brokenKernel.Run(); assert(!brokenKernel.Correct());
	Machine kernelOperands(false,8); kernelOperands.Run();
	assert(kernelOperands.Correct() && kernelOperands.matrixInstructions == 0 && kernelOperands.vectorWrites == 384 && kernelOperands.outputReads == 0);
	Machine kernelMatrix(false,9); kernelMatrix.Run();
	assert(kernelMatrix.Correct() && kernelMatrix.matrixInstructions == 1 && kernelMatrix.vectorWrites == 384 && kernelMatrix.outputReads == 0);
	Machine dstRead(false,10); dstRead.Run();
	assert(dstRead.Correct() && dstRead.matrixInstructions == 0 && dstRead.vectorWrites == 768 && dstRead.outputReads == 256);
	Machine brokenVector(false,10); brokenVector.dropVector = true; brokenVector.Run(); assert(!brokenVector.Correct());
	for (unsigned stage = 11; stage <= 12; stage++) {
		Machine roundtrip(false,stage); roundtrip.Run();
		assert(roundtrip.Correct() && roundtrip.matrixInstructions == 0 && roundtrip.moveInstructions == 2 && roundtrip.outputReads == 256);
	}
	for (unsigned stage = 13; stage <= 15; stage++) {
		Machine pure(false,stage); pure.Run();
		assert(pure.Correct() && pure.outputReads == 256 && pure.matrixInstructions == (stage == 15 ? 2U : 0U));
	}
	for (unsigned stage = 16; stage <= 19; stage++) {
		Machine unpack(false,stage); unpack.Run();
		assert(unpack.Correct() && unpack.matrixInstructions == (stage == 16 ? 2U : 0U));
		assert(unpack.unpackReads == (stage == 19 ? 0U : 768U));
		assert(unpack.inputStores == (stage == 19 ? 0U : 400U));
		assert(unpack.outputReads == (stage == 19 ? 0U : 256U));
	}
	Machine brokenUnpack(false,16); brokenUnpack.dropUnpack = true; brokenUnpack.Run(); assert(!brokenUnpack.Correct());
	Machine brokenUnpackMath(false,16); brokenUnpackMath.dropMatrix = true; brokenUnpackMath.Run(); assert(!brokenUnpackMath.Correct());
	Machine staleDst(false,16);
	for (unsigned row = 32; row < 40; row++) for (unsigned col = 0; col < 16; col++) staleDst.dst[row][col] = 3;
	staleDst.Run(); assert(staleDst.Correct());
	Machine noClear(false,16); noClear.dropClear = true;
	for (unsigned row = 32; row < 40; row++) for (unsigned col = 0; col < 16; col++) noClear.dst[row][col] = 3;
	noClear.Run(); assert(!noClear.Correct() && noClear.l1[noClear.Index(0x17f800)] == Bits(19) && noClear.l1[noClear.Index(0x17fa00)] == Bits(115));
	Machine bounds(false,16);
	bounds.Instruction(0x5e23fc00); bounds.Instruction(0x5e41fc00); assert(bounds.UnpackBoundsValid());
	bounds.Instruction(0x5160000a); assert(bounds.UnpackBoundsValid());
	bounds.Instruction(0x5160000f); assert(!bounds.UnpackBoundsValid());
	for (int integer = -256; integer <= 256; integer++) assert(BlackholeFixedIntegerFp32(integer) == Bits(float(integer)));
	for (unsigned stage = 20; stage <= 23; stage++) {
		Machine nonuniform(false,stage); nonuniform.Run();
		assert(nonuniform.Correct() && nonuniform.unpackReads == 768 && nonuniform.inputStores == 144 && nonuniform.outputReads == 256);
		assert(nonuniform.matrixInstructions == (stage <= 21 ? 2U : 0U));
	}
	Machine transposed(false,21); transposed.transposeA = true; transposed.Run(); assert(!transposed.Correct());
	Machine noNonuniformMath(false,21); noNonuniformMath.dropMatrix = true; noNonuniformMath.Run(); assert(!noNonuniformMath.Correct());
	// Same independently assembled identity program, data-only substitutions.
	// Fractional values ensure this is not the old fixed integer oracle.
	for (unsigned trial = 0; trial < 8; trial++) {
		tt_tensix_tile_data input = {}; input.version = 1;
		for (unsigned i = 0; i < 256; i++) {
			input.weights[i] = (0x3e80+(i*13+trial*7)%384) | ((i&1) ? 0x8000 : 0);
			input.activations[i] = (0x3f00+(i*11+trial*3)%256) | ((i&2) ? 0x8000 : 0);
		}
		assert(BlackholeTileDataValid(input));
		Machine data(false,24);
		for (unsigned i = 0; i < TT_UNPACK_IMAGE_WORDS; i++) data.l1[i] = BlackholeTileDataImageWord(i,input);
		data.Run();
		assert(data.matrixInstructions == 8 && data.outputReads == 256);
		Machine low(false,24); low.dropLowFidelity = true;
		for (unsigned i = 0; i < TT_UNPACK_IMAGE_WORDS; i++) low.l1[i] = BlackholeTileDataImageWord(i,input);
		low.Run(); unsigned lowMismatches = 0;
		for (unsigned row = 0; row < 16; row++) for (unsigned col = 0; col < 16; col++) {
			float reference = 0;
			for (unsigned k = 0; k < 16; k++) reference += Float(uint32_t(input.activations[row*16+k])<<16)*Float(uint32_t(input.weights[k*16+col])<<16);
			assert(fabsf(Float(data.l1[data.Index(0x17f800+4*(row*16+col))])-reference) < 0.00001f);
			lowMismatches += low.l1[low.Index(0x17f800+4*(row*16+col))] != Bits(reference);
		}
		assert(lowMismatches > 200);
	}
	printf("Offline fixed image: %u steps, %u manual drains, two nonzero matrix fixtures; no hardware timing modeled\n",m.steps,m.drains);
}
#endif
#endif
