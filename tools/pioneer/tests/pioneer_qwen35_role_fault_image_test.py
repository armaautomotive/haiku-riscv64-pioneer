#!/usr/bin/env python3
"""Interpret the linked resident ticks for a fixed rejected invocation; no hardware."""
import struct
import sys
from pathlib import Path

MASK = 0xffffffff
MAILBOX = 0xab500
QUEUE = 0x89000
PROGRAM = 0xaa000
STACK_BASE = 0xffb00000
STACK_END = 0xffb02000
STATE = STACK_BASE + 32
RETURN = 0xdead0000


def signed(value, bits=32):
    value &= (1 << bits) - 1
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def symbols(path):
    result = {}
    for line in path.read_text().splitlines():
        row = line.split()
        if len(row) == 3 and row[1] in ('T', 't'):
            result[row[2]] = int(row[0], 16)
    return result


class Machine:
    def __init__(self, image, base, entry, shared):
        self.image, self.base, self.entry, self.shared = image, base, entry, shared
        self.private = bytearray(STACK_END - STACK_BASE)
        self.private[STATE - STACK_BASE + 1] = 1  # started, not stopped
        self.reads, self.writes = [], []

    def region(self, address, size):
        if 0x80000 <= address and address + size <= 0xb0000:
            return self.shared, address - 0x80000
        if STACK_BASE <= address and address + size <= STACK_END:
            return self.private, address - STACK_BASE
        raise AssertionError(f'unexpected L1/MMIO access {address:#x}, {size}')

    def read(self, address, size):
        mem, offset = self.region(address, size)
        self.reads.append((address, size))
        return int.from_bytes(mem[offset:offset + size], 'little')

    def write(self, address, size, value):
        mem, offset = self.region(address, size)
        mem[offset:offset + size] = (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little')
        if address < STACK_BASE:
            self.writes.append((address, size, value & ((1 << (size * 8)) - 1)))

    def tick(self):
        r = [0] * 32
        r[1], r[2], r[10], r[11] = RETURN, STACK_END, MAILBOX, STATE
        pc = self.entry
        for _ in range(20000):
            if pc == RETURN:
                return r[10]
            assert self.base <= pc <= self.base + len(self.image) - 4 and not (pc & 3), hex(pc)
            insn = struct.unpack_from('<I', self.image, pc - self.base)[0]
            op, rd, f3 = insn & 127, insn >> 7 & 31, insn >> 12 & 7
            rs1, rs2, f7 = insn >> 15 & 31, insn >> 20 & 31, insn >> 25
            next_pc, value = (pc + 4) & MASK, None
            if op == 0x37:
                value = insn & 0xfffff000
            elif op == 0x17:
                value = pc + (insn & 0xfffff000)
            elif op == 0x13:
                imm = signed(insn >> 20, 12)
                if f3 == 0: value = r[rs1] + imm
                elif f3 == 1: value = r[rs1] << rs2
                elif f3 == 2: value = int(signed(r[rs1]) < imm)
                elif f3 == 3: value = int(r[rs1] < (imm & MASK))
                elif f3 == 4: value = r[rs1] ^ imm
                elif f3 == 5: value = signed(r[rs1]) >> rs2 if f7 == 0x20 else r[rs1] >> rs2
                elif f3 == 6: value = r[rs1] | imm
                elif f3 == 7: value = r[rs1] & imm
                else: raise AssertionError(hex(insn))
            elif op == 0x33:
                a, b = r[rs1], r[rs2]
                if f7 == 1:
                    assert f3 == 0, f'unsupported RV32M instruction {insn:#x}'
                    value = a * b
                elif f3 == 0: value = a - b if f7 == 0x20 else a + b
                elif f3 == 1: value = a << (b & 31)
                elif f3 == 2: value = int(signed(a) < signed(b))
                elif f3 == 3: value = int(a < b)
                elif f3 == 4: value = a ^ b
                elif f3 == 5: value = signed(a) >> (b & 31) if f7 == 0x20 else a >> (b & 31)
                elif f3 == 6: value = a | b
                elif f3 == 7: value = a & b
                else: raise AssertionError(hex(insn))
            elif op == 0x03:
                address = (r[rs1] + signed(insn >> 20, 12)) & MASK
                size = {0: 1, 1: 2, 2: 4, 4: 1, 5: 2}[f3]
                value = self.read(address, size)
                if f3 in (0, 1): value = signed(value, size * 8)
            elif op == 0x23:
                imm = signed((insn >> 25 << 5) | (insn >> 7 & 31), 12)
                address = (r[rs1] + imm) & MASK
                self.write(address, {0: 1, 1: 2, 2: 4}[f3], r[rs2])
            elif op == 0x63:
                imm = ((insn >> 31 & 1) << 12) | ((insn >> 7 & 1) << 11) \
                    | ((insn >> 25 & 63) << 5) | ((insn >> 8 & 15) << 1)
                a, b = r[rs1], r[rs2]
                take = {0: a == b, 1: a != b, 4: signed(a) < signed(b),
                        5: signed(a) >= signed(b), 6: a < b, 7: a >= b}[f3]
                if take: next_pc = (pc + signed(imm, 13)) & MASK
            elif op == 0x6f:
                imm = ((insn >> 31 & 1) << 20) | ((insn >> 12 & 255) << 12) \
                    | ((insn >> 20 & 1) << 11) | ((insn >> 21 & 1023) << 1)
                value, next_pc = pc + 4, (pc + signed(imm, 21)) & MASK
            elif op == 0x67:
                value, next_pc = pc + 4, (r[rs1] + signed(insn >> 20, 12)) & MASK & ~1
            elif op == 0x0f:
                pass  # fence; this interpreter executes one role at a time
            else:
                raise AssertionError(f'unsupported instruction {insn:#x} at {pc:#x}')
            if rd and value is not None: r[rd] = value & MASK
            r[0], pc = 0, next_pc
        raise AssertionError('resident tick did not return')


def word(mem, address, value=None):
    offset = address - 0x80000
    if value is not None: struct.pack_into('<I', mem, offset, value)
    return struct.unpack_from('<I', mem, offset)[0]


if len(sys.argv) != 2:
    raise SystemExit('usage: test.py IMAGE_DIRECTORY')
directory = Path(sys.argv[1])
for order in ((0, 1), (1, 0)):
    shared = bytearray(0x30000)
    for address, value in ((MAILBOX, 0x31524d35), (MAILBOX + 4, 2), (MAILBOX + 8, 1),
                           (MAILBOX + 44, 0x52333551), (MAILBOX + 48, 0x57333551)):
        word(shared, address, value)
    # The first circular-buffer address is zero; all roles must reject before NoC.
    word(shared, PROGRAM + 116, 0)
    machines = []
    for role, base in (('reader', 0xb0000), ('writer', 0xb5000)):
        entry = symbols(directory / f'{role}.symbols')[f'qwen35_shared_{role}_tick']
        machines.append(Machine((directory / f'{role}-service.bin').read_bytes(), base, entry, shared))
    for index in order:
        machine = machines[index]
        assert machine.tick() == 4  # role body failed and published a receipt
        assert (PROGRAM + 116, 4) in machine.reads
    for index, machine in enumerate(machines):
        assert word(shared, MAILBOX + 16 + index * 4) == 1
        assert word(shared, MAILBOX + 24 + index * 4) == 3
        before = len(machine.writes)
        assert machine.tick() == 5 and len(machine.writes) == before  # waits for recovery
    for queue in range(3):
        values = [word(shared, QUEUE + queue * 32 + i * 4) for i in range(8)]
        assert values == [0, 0, 1, 0, 3, 0, 0, 0], values
    word(shared, MAILBOX + 32, 1)
    for index in order:
        machine = machines[index]
        assert machine.tick() == 6 and word(shared, MAILBOX + 36 + index * 4) == 1
        allowed = {MAILBOX + 16 + index * 4, MAILBOX + 24 + index * 4, MAILBOX + 36 + index * 4}
        allowed.update(QUEUE + queue * 32 + 8 + index * 8 for queue in range(3))
        for address, size, _ in machine.writes:
            assert size == 4 and address in allowed, (hex(address), size)
print('PASS linked reader/writer fault, receipts, recovery wait and stop; RV32 SIMULATION ONLY')
