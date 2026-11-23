#!/usr/bin/env python3
"""Interpret the linked takeover shim through stop/failure, without hardware."""
import struct
import sys
from pathlib import Path


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def run(code, role, tick, bypass, replies):
    base = 0x70000 if role == "reader" else 0x70100
    stack = 0xFFB02000 - 16
    regs = [0] * 32
    pc, cfg, calls, stores, fences = base, 0, 0, [], 0
    for _ in range(1000):
        assert base <= pc <= base + len(code) - 4 and not (pc & 3)
        insn = struct.unpack_from("<I", code, pc - base)[0]
        opcode, rd, funct3 = insn & 127, (insn >> 7) & 31, (insn >> 12) & 7
        rs1, rs2 = (insn >> 15) & 31, (insn >> 20) & 31
        nxt, result = pc + 4, None
        if opcode == 0x37:  # LUI
            result = insn & 0xFFFFF000
        elif opcode == 0x17:  # AUIPC
            result = (pc + (insn & 0xFFFFF000)) & 0xFFFFFFFF
        elif opcode == 0x13:  # ADDI, ANDI
            immediate = signed(insn >> 20, 12)
            assert funct3 in (0, 7)
            result = regs[rs1] + immediate if funct3 == 0 else regs[rs1] & immediate
        elif opcode == 0x23:  # only the initial local stack state may be stored
            assert funct3 == 2 and bypass and not calls
            immediate = signed(((insn >> 25) << 5) | ((insn >> 7) & 31), 12)
            address = (regs[rs1] + immediate) & 0xFFFFFFFF
            assert address == stack and regs[rs2] == 0
            stores.append(address)
        elif opcode == 0x63:  # BEQ, BNE
            immediate = (((insn >> 31) & 1) << 12) | (((insn >> 7) & 1) << 11)
            immediate |= (((insn >> 25) & 63) << 5) | (((insn >> 8) & 15) << 1)
            assert funct3 in (0, 1)
            if (regs[rs1] == regs[rs2]) == (funct3 == 0):
                nxt = pc + signed(immediate, 13)
        elif opcode == 0x0F:
            fences += 1
        elif opcode == 0x73:
            assert (insn >> 20) == 0x7C0
            if funct3 == 6:
                if bypass:
                    cfg |= rs1
            else:
                assert funct3 == 2 and rs1 == 0
                result = cfg
        elif opcode == 0x67:  # intercept the actual linked tick call
            target = (regs[rs1] + signed(insn >> 20, 12)) & 0xFFFFFFFE
            assert target == tick and rd == 1 and bypass and stores == [stack]
            assert regs[10] == 0xAB500 and regs[11] == stack and calls < len(replies)
            regs[10] = replies[calls]
            calls += 1
            result = pc + 4
        elif opcode == 0x6F and insn == 0x6F:  # park
            assert fences >= 2 and calls == len(replies)
            assert stores == ([stack] if bypass else [])
            return
        else:
            raise AssertionError(f"unexpected memory access or opcode {insn:08x} at {pc:08x}")
        if result is not None and rd:
            regs[rd] = result & 0xFFFFFFFF
        regs[0] = 0
        pc = nxt
    raise AssertionError("exclusive role did not park")


if len(sys.argv) != 4 or sys.argv[2] not in ("reader", "writer"):
    raise SystemExit("usage: test.py SHIM reader|writer TICK_HEX")
image = Path(sys.argv[1]).read_bytes()
assert image and len(image) <= 256 and len(image) % 4 == 0
tick = int(sys.argv[3], 16)
for replies in ([0], [6], [1, 2, 3, 4, 5, 6]):
    run(image, sys.argv[2], tick, True, replies)
run(image, sys.argv[2], tick, False, [])
print(f"PASS {sys.argv[2]} exclusive shim: own stack, tick loop, stop and failed cache bypass; SIMULATED ONLY")
