#!/usr/bin/env python3
"""Run the linked probe and exact relocated restore bytes; simulated only."""
import struct
import sys
from pathlib import Path


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def run(shim, restore, role, decision='restore', bypass=True, corrupt_copy=False,
        drop_mutation=False, drop_restore=None, drift_source=None):
    shim_base, code_base, copy_base = ((0x70000, 0xB0000, 0xB8000) if role == 'reader'
                                        else (0x70100, 0xB5000, 0xBA000))
    ready = 0x31 if role == 'reader' else 0x32
    success, failure = (0x81, 0xE1) if role == 'reader' else (0x82, 0xE2)
    handoff = 0xAB540
    receipt_address = handoff + (0 if role == 'reader' else 4)
    local_base, words = 0xFFB00000, 2048
    original = [(0x9E3779B9 * (i + 1) ^ 0xA1556C3D) & 0xFFFFFFFF for i in range(words)]
    local = original.copy()
    copied = [None] * words
    regs = [0] * 32
    pc, cfg, receipt, command = shim_base, 0, 0, 0
    copy_stores = local_loads = restore_stores = polls = mutation = 0
    initial_checks = restore_checks = fences = 0
    entered_restore = corrupted = source_changed = receipt_observed = False
    for _ in range(100000):
        if shim_base <= pc <= shim_base + len(shim) - 4:
            insn = struct.unpack_from('<I', shim, pc - shim_base)[0]
        else:
            assert code_base <= pc <= code_base + len(restore) - 4 and entered_restore
            insn = struct.unpack_from('<I', restore, pc - code_base)[0]
        assert not (pc & 3)
        op, rd, f3 = insn & 127, (insn >> 7) & 31, (insn >> 12) & 7
        rs1, rs2 = (insn >> 15) & 31, (insn >> 20) & 31
        nxt, value = pc + 4, None
        if op == 0x37:
            value = insn & 0xFFFFF000
        elif op == 0x13:
            imm = signed(insn >> 20, 12)
            if f3 == 0:
                value = (regs[rs1] + imm) & 0xFFFFFFFF
            elif f3 == 7:
                value = regs[rs1] & imm
            else:
                assert f3 == 4
                value = regs[rs1] ^ imm
        elif op == 0x03:
            assert f3 == 2
            address = (regs[rs1] + signed(insn >> 20, 12)) & 0xFFFFFFFF
            if local_base <= address < local_base + words * 4:
                index = (address - local_base) // 4
                if not entered_restore:
                    local_loads += 1
                elif restore_stores == words:
                    assert index == restore_checks
                    restore_checks += 1
                value = local[index]
            elif copy_base <= address < copy_base + words * 4:
                index = (address - copy_base) // 4
                assert copy_stores == words and fences >= 2
                if not entered_restore:
                    if address == copy_base + (words - 1) * 4 and not initial_checks:
                        value = copied[index]  # visibility read after copy
                    else:
                        if corrupt_copy and not corrupted:
                            copied[words // 2] ^= 1
                            corrupted = True
                        assert index == initial_checks
                        initial_checks += 1
                        value = copied[index]
                else:
                    if restore_stores == words and drift_source is not None and not source_changed:
                        copied[drift_source] ^= 1
                        source_changed = True
                    value = copied[index]
            elif address == handoff + 8:
                assert receipt == ready and copied == original and local == original
                polls += 1
                if polls == 3:
                    if decision == 'wait':
                        return
                    command = 0x43 if decision == 'restore' else 0x44
                value = command
            else:
                assert address == receipt_address and entered_restore and receipt in (success, failure)
                receipt_observed = True
                value = receipt
        elif op == 0x23:
            assert f3 == 2
            imm = signed(((insn >> 25) << 5) | ((insn >> 7) & 31), 12)
            address = (regs[rs1] + imm) & 0xFFFFFFFF
            if copy_base <= address < copy_base + words * 4:
                assert not receipt and address == copy_base + copy_stores * 4
                copied[copy_stores] = regs[rs2]
                copy_stores += 1
            elif local_base <= address < local_base + words * 4:
                assert command == 0x43 and receipt == ready
                index = (address - local_base) // 4
                if not entered_restore:
                    assert not mutation and index == 0 and regs[rs2] == (original[0] ^ 1)
                    mutation += 1
                    if not drop_mutation:
                        local[0] = regs[rs2]
                else:
                    assert index == restore_stores and restore_stores < words
                    if restore_stores != drop_restore:
                        local[index] = regs[rs2]
                    restore_stores += 1
            else:
                assert address == receipt_address
                if not receipt:
                    assert regs[rs2] == ready and copy_stores == words and initial_checks == words
                    assert local == copied == original and local_loads >= words * 2
                else:
                    assert entered_restore and receipt == ready and regs[rs2] in (success, failure)
                    assert restore_stores == words and mutation == 1
                receipt = regs[rs2]
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
        elif op == 0x67:
            assert rd == 0 and not entered_restore and command == 0x43 and mutation == 1
            assert local[0] == (original[0] ^ 1) and copied == original
            nxt = (regs[rs1] + signed(insn >> 20, 12)) & 0xFFFFFFFE
            assert nxt == code_base
            entered_restore = True
        elif op == 0x6F:
            assert rd == 0
            if insn == 0x6F:
                if not bypass:
                    assert not copy_stores and not receipt and local == original
                elif corrupt_copy:
                    assert not receipt and not mutation and local == original
                elif decision == 'invalid':
                    assert receipt == ready and not mutation and local == original
                elif drop_mutation:
                    assert receipt == ready and mutation == 1 and not entered_restore and local == original
                else:
                    assert entered_restore and receipt_observed and restore_stores == words
                    assert receipt == (failure if drop_restore is not None or drift_source is not None else success)
                    if drop_restore is None:
                        assert local == original
                    else:
                        assert local[drop_restore] != original[drop_restore]
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
    raise AssertionError('probe did not park')


if len(sys.argv) != 4 or sys.argv[3] not in ('reader', 'writer'):
    raise SystemExit('usage: test.py SHIM RESTORE reader|writer')
shim, restore = Path(sys.argv[1]).read_bytes(), Path(sys.argv[2]).read_bytes()
assert shim and restore and len(shim) <= 256 and not len(shim) % 4 and not len(restore) % 4
for kwargs in ({}, {'decision': 'wait'}, {'decision': 'invalid'}, {'bypass': False},
               {'corrupt_copy': True}, {'drop_mutation': True}, {'drop_restore': 0},
               {'drift_source': 1024}):
    run(shim, restore, sys.argv[3], **kwargs)
print(f'PASS {sys.argv[3]} local mutation and exact relocated restore bytes, including faults; SIMULATED ONLY')
