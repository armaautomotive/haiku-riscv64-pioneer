#!/usr/bin/env python3
"""Execute the actual RV32 startup shim up to its first role tick, offline."""

import struct
import sys
from pathlib import Path


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def run(code, role, bypass_works, decision):
    base, target = (0x70000, 0xB8000) if role == "reader" else (0x70100, 0xBA000)
    handoff, ready_value = 0xAB540, (0x31 if role == "reader" else 0x32)
    receipt_address = handoff + (0 if role == "reader" else 4)
    local_base, count = 0xFFB00000, 2048
    original = [(0x9E3779B9 * (i + 1) ^ 0xA1556C3D) & 0xFFFFFFFF for i in range(count)]
    local = original.copy()
    copied = [None] * count
    regs = [0] * 32
    pc, cfg = base, 0
    writes, fences, visible, dependent, stack_writes = 0, 0, False, False, 0
    receipt, polls, release = 0, 0, 0
    for _ in range(30000):
        assert base <= pc <= base + len(code) - 4 and not (pc & 3)
        insn = struct.unpack_from("<I", code, pc - base)[0]
        opcode, rd, funct3 = insn & 127, (insn >> 7) & 31, (insn >> 12) & 7
        rs1, rs2 = (insn >> 15) & 31, (insn >> 20) & 31
        nxt, result = pc + 4, None
        if opcode == 0x37:  # LUI
            result = insn & 0xFFFFF000
        elif opcode == 0x17:  # AUIPC
            result = (pc + (insn & 0xFFFFF000)) & 0xFFFFFFFF
        elif opcode == 0x13:  # ADDI or ANDI
            immediate = signed(insn >> 20, 12)
            if funct3 == 0:
                result = (regs[rs1] + immediate) & 0xFFFFFFFF
                if rd == 0 and rs1 == 28 and immediate == 0 and visible:
                    dependent = True
            else:
                assert funct3 == 7
                result = regs[rs1] & immediate
        elif opcode == 0x03:  # LW
            assert funct3 == 2
            address = (regs[rs1] + signed(insn >> 20, 12)) & 0xFFFFFFFF
            if local_base <= address < local_base + count * 4:
                assert not stack_writes
                result = local[(address - local_base) // 4]
            elif address == handoff + 8:
                assert receipt == ready_value and dependent and not stack_writes
                polls += 1
                if polls == 3:
                    if decision == "wait":
                        return
                    release = 0x53 if decision == "release" else 0x43
                result = release
            else:
                assert address == target + count * 4 - 4 and writes == count and fences >= 2
                assert copied == original
                result = copied[-1]
                visible = True
        elif opcode == 0x23:  # SW
            assert funct3 == 2
            immediate = signed(((insn >> 25) << 5) | ((insn >> 7) & 31), 12)
            address = (regs[rs1] + immediate) & 0xFFFFFFFF
            if target <= address < target + count * 4:
                assert address == target + writes * 4 and not stack_writes
                copied[writes] = regs[rs2]
                writes += 1
            elif address == receipt_address:
                assert not receipt and regs[rs2] == ready_value
                assert writes == count and copied == original and dependent and not stack_writes
                receipt = regs[rs2]
            else:
                assert address == local_base + count * 4 - 16
                assert writes == count and copied == original and dependent and release == 0x53 and polls >= 3
                local[(address - local_base) // 4] = regs[rs2]
                stack_writes += 1
        elif opcode == 0x63:  # BEQ or BNE
            immediate = (((insn >> 31) & 1) << 12) | (((insn >> 7) & 1) << 11)
            immediate |= (((insn >> 25) & 63) << 5) | (((insn >> 8) & 15) << 1)
            assert funct3 in (0, 1)
            if (regs[rs1] == regs[rs2]) == (funct3 == 0):
                nxt = pc + signed(immediate, 13)
        elif opcode == 0x0F:  # FENCE
            fences += 1
        elif opcode == 0x73:  # cfg0 CSR set/read
            assert (insn >> 20) == 0x7C0
            if funct3 == 6:
                if bypass_works:
                    cfg |= rs1
            else:
                assert funct3 == 2 and rs1 == 0
                result = cfg
        elif opcode == 0x67:  # call to role tick
            assert bypass_works and rd == 1 and writes == count and stack_writes == 1
            assert copied == original and dependent and regs[10] == 0xAB500
            return
        elif opcode == 0x6F:  # jump or park loop
            assert rd == 0
            if insn == 0x6F:
                assert not stack_writes
                if bypass_works:
                    assert decision == "cancel" and receipt == ready_value and release == 0x43
                else:
                    assert not writes and not receipt
                return
            immediate = (((insn >> 31) & 1) << 20) | (((insn >> 12) & 255) << 12)
            immediate |= (((insn >> 20) & 1) << 11) | (((insn >> 21) & 1023) << 1)
            nxt = pc + signed(immediate, 21)
        else:
            raise AssertionError(f"unsupported instruction {insn:08x} at {pc:08x}")
        if result is not None and rd:
            regs[rd] = result & 0xFFFFFFFF
        regs[0] = 0
        pc = nxt
    raise AssertionError("startup shim did not finish")


if len(sys.argv) != 3 or sys.argv[2] not in ("reader", "writer"):
    raise SystemExit("usage: test.py SHIM reader|writer")
image = Path(sys.argv[1]).read_bytes()
assert image and len(image) <= 256 and len(image) % 4 == 0
for decision in ("wait", "release", "cancel"):
    run(image, sys.argv[2], True, decision)
run(image, sys.argv[2], False, "wait")
print(f"PASS {sys.argv[2]} RV32 pre-stack copy, wait, release, cancel and failed-cache-bypass park; SIMULATED ONLY")
