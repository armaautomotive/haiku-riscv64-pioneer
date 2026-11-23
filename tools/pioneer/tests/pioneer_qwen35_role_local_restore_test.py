#!/usr/bin/env python3
"""Execute a linked RV32 local-restore witness; simulated device only."""
import struct
import sys
from pathlib import Path


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def run(code, role, bypass=True, dropped_store=None):
    base = 0x70000 if role == 'reader' else 0x70100
    source = 0xB8000 if role == 'reader' else 0xBA000
    receipt_address = 0xAB540 + (0 if role == 'reader' else 4)
    success, failure = (0x81, 0xE1) if role == 'reader' else (0x82, 0xE2)
    local_base, words = 0xFFB00000, 2048
    expected = [(0x9E3779B9 * (i + 1) ^ 0xA1556C3D) & 0xFFFFFFFF for i in range(words)]
    memory = [0xDEADBEEF] * words
    regs = [0] * 32
    pc, cfg, stores, checks, receipt, fences = base, 0, 0, 0, 0, 0
    observed_receipt = False
    for _ in range(50000):
        assert base <= pc <= base + len(code) - 4 and not (pc & 3)
        insn = struct.unpack_from('<I', code, pc - base)[0]
        op, rd, f3 = insn & 127, (insn >> 7) & 31, (insn >> 12) & 7
        rs1, rs2 = (insn >> 15) & 31, (insn >> 20) & 31
        nxt, value = pc + 4, None
        if op == 0x37:
            value = insn & 0xFFFFF000
        elif op == 0x13:
            imm = signed(insn >> 20, 12)
            if f3 == 0:
                value = (regs[rs1] + imm) & 0xFFFFFFFF
            else:
                assert f3 == 7
                value = regs[rs1] & imm
        elif op == 0x03:
            assert f3 == 2
            address = (regs[rs1] + signed(insn >> 20, 12)) & 0xFFFFFFFF
            if source <= address < source + words * 4:
                index = (address - source) // 4
                assert index == (stores if stores < words else checks)
                value = expected[index]
            elif local_base <= address < local_base + words * 4:
                assert stores == words and fences >= 2
                index = (address - local_base) // 4
                assert index == checks
                value = memory[index]
                checks += 1
            else:
                assert address == receipt_address and receipt and fences >= 3
                value = receipt
                observed_receipt = True
        elif op == 0x23:
            assert f3 == 2
            imm = signed(((insn >> 25) << 5) | ((insn >> 7) & 31), 12)
            address = (regs[rs1] + imm) & 0xFFFFFFFF
            if local_base <= address < local_base + words * 4:
                assert address == local_base + stores * 4 and stores < words and not receipt
                if stores != dropped_store:
                    memory[stores] = regs[rs2]
                stores += 1
            else:
                assert address == receipt_address and stores == words and not receipt
                receipt = regs[rs2]
                assert receipt == (success if dropped_store is None else failure)
                assert checks == (words if dropped_store is None else dropped_store + 1)
        elif op == 0x63:
            imm = (((insn >> 31) & 1) << 12) | (((insn >> 7) & 1) << 11)
            imm |= (((insn >> 25) & 63) << 5) | (((insn >> 8) & 15) << 1)
            assert f3 in (0, 1)
            if (regs[rs1] == regs[rs2]) == (f3 == 0):
                nxt = pc + signed(imm, 13)
        elif op == 0x0F:
            fences += 1
        elif op == 0x73:
            assert (insn >> 20) == 0x7C0
            if f3 == 6:
                if bypass:
                    cfg |= rs1
            else:
                assert f3 == 2 and rs1 == 0
                value = cfg
        elif op == 0x6F:
            assert rd == 0
            if insn == 0x6F:
                if not bypass:
                    assert not stores and not receipt and memory == [0xDEADBEEF] * words
                else:
                    assert receipt and observed_receipt
                    if dropped_store is None:
                        assert memory == expected and receipt == success
                    else:
                        assert memory[dropped_store] != expected[dropped_store] and receipt == failure
                return
            imm = (((insn >> 31) & 1) << 20) | (((insn >> 12) & 255) << 12)
            imm |= (((insn >> 20) & 1) << 11) | (((insn >> 21) & 1023) << 1)
            nxt = pc + signed(imm, 21)
        else:
            raise AssertionError(f'unsupported instruction {insn:08x} at {pc:08x}')
        if value is not None and rd:
            regs[rd] = value & 0xFFFFFFFF
        regs[0] = 0
        pc = nxt
    raise AssertionError('restore witness did not park')


if len(sys.argv) != 3 or sys.argv[2] not in ('reader', 'writer'):
    raise SystemExit('usage: test.py SHIM reader|writer')
image = Path(sys.argv[1]).read_bytes()
assert image and len(image) <= 256 and len(image) % 4 == 0
run(image, sys.argv[2])
run(image, sys.argv[2], dropped_store=1024)
run(image, sys.argv[2], bypass=False)
print(f'PASS {sys.argv[2]} RV32 local restore, mismatch receipt and cache-bypass refusal; SIMULATED ONLY')
