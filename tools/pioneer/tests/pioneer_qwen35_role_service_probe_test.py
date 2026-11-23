#!/usr/bin/env python3
"""Execute the actual role shim and relocated restore, with a simulated tick."""
import struct
import sys
from pathlib import Path


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def symbols(path):
    result = {}
    for line in path.read_text().splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[1] in ('T', 't'):
            result[fields[2]] = int(fields[0], 16)
    return result


def execute(directory, role, invalid_tick=False):
    base, code_base, restore_base, copy_base = ((0x70000, 0xB0000, 0xB4E00, 0xB8000)
        if role == 'reader' else (0x70100, 0xB5000, 0xB7E00, 0xBA000))
    shim = (directory / f'{role}-shim.bin').read_bytes()
    service = (directory / f'{role}-service.bin').read_bytes()
    composite = (directory / f'{role}-composite.bin').read_bytes()
    restore = (directory / 'local-probe' / 'local-restore' / f'{role}.bin').read_bytes()
    assert composite[:len(service)] == service
    assert composite[restore_base-code_base:] == restore
    assert not any(composite[len(service):restore_base-code_base])
    tick = symbols(directory / f'{role}.symbols')[f'qwen35_shared_{role}_tick']
    local_base, count, handoff = 0xFFB00000, 2048, 0xAB540
    receipt_addr = handoff + (0 if role == 'reader' else 4)
    ready = 0x31 if role == 'reader' else 0x32
    done = 0x81 if role == 'reader' else 0x82
    original = [(0x9E3779B9 * (i+1) ^ 0xA1556C3D) & 0xFFFFFFFF for i in range(count)]
    local, copied, regs = original.copy(), [None]*count, [0]*32
    receipt = release = polls = ticks = copies = checks = restore_writes = cfg = 0
    pc, ran_restore = base, False
    for _ in range(120000):
        if base <= pc <= base+len(shim)-4:
            insn = struct.unpack_from('<I', shim, pc-base)[0]
        else:
            assert restore_base <= pc <= restore_base+len(restore)-4 and ran_restore
            insn = struct.unpack_from('<I', restore, pc-restore_base)[0]
        op, rd, f3 = insn&127, (insn>>7)&31, (insn>>12)&7
        rs1, rs2 = (insn>>15)&31, (insn>>20)&31
        nxt, result = pc+4, None
        if op == 0x37:
            result = insn&0xFFFFF000
        elif op == 0x17:
            result = (pc+(insn&0xFFFFF000))&0xFFFFFFFF
        elif op == 0x13:
            immediate = signed(insn>>20, 12)
            assert f3 in (0, 7)
            result = (regs[rs1]+immediate if f3==0 else regs[rs1]&immediate)&0xFFFFFFFF
        elif op == 0x03:
            assert f3 == 2
            address = (regs[rs1]+signed(insn>>20,12))&0xFFFFFFFF
            if local_base <= address < local_base+count*4:
                result = local[(address-local_base)//4]
            elif copy_base <= address < copy_base+count*4:
                result = copied[(address-copy_base)//4]
                assert result is not None
                if not ran_restore and receipt == 0:
                    checks += 1
            elif address == handoff+8:
                assert receipt == ready and copied == original
                polls += 1
                if polls == 2:
                    release = 0x53
                result = release
            elif address == receipt_addr:
                assert receipt == done and ran_restore
                result = receipt
            else:
                raise AssertionError(f'unexpected read {address:08x}')
        elif op == 0x23:
            assert f3 == 2
            imm = signed(((insn>>25)<<5)|((insn>>7)&31),12)
            address = (regs[rs1]+imm)&0xFFFFFFFF
            if copy_base <= address < copy_base+count*4:
                assert not ticks and receipt == 0
                copied[(address-copy_base)//4] = regs[rs2]; copies += 1
            elif local_base <= address < local_base+count*4:
                assert release == 0x53 or ran_restore
                local[(address-local_base)//4] = regs[rs2]
                if ran_restore:
                    restore_writes += 1
            elif address == receipt_addr:
                if not ran_restore:
                    assert copies == count and copied == original and checks >= count
                    assert regs[rs2] == ready
                else:
                    assert ticks and restore_writes == count and local == original
                    assert regs[rs2] == done
                receipt = regs[rs2]
            else:
                raise AssertionError(f'unexpected store {address:08x}')
        elif op == 0x63:
            imm = (((insn>>31)&1)<<12)|(((insn>>7)&1)<<11)|(((insn>>25)&63)<<5)|(((insn>>8)&15)<<1)
            assert f3 in (0,1)
            if (regs[rs1] == regs[rs2]) == (f3 == 0):
                nxt = pc+signed(imm,13)
        elif op == 0x0F:
            pass
        elif op == 0x73:
            assert (insn>>20) == 0x7C0
            if f3 == 6:
                cfg |= rs1
            else:
                assert f3 == 2 and rs1 == 0
                result = cfg
        elif op == 0x67:
            target = (regs[rs1]+signed(insn>>20,12))&0xFFFFFFFE
            if rd == 1:
                assert target == tick and release == 0x53 and copied == original
                # Model real role calls mutating their own local stack. The
                # linked service body itself is exercised by C++ role tests.
                local[count-20] ^= 0x1234
                ticks += 1
                regs[10] = 0 if invalid_tick else 1 if ticks < 3 else 6
                nxt = pc+4
            else:
                assert rd == 0 and target == restore_base and ticks == 3 and not invalid_tick
                assert local != original
                ran_restore = True;nxt = target
        elif op == 0x6F:
            assert rd == 0
            if insn == 0x6F:
                if invalid_tick:
                    assert ticks == 1 and not ran_restore and receipt == ready and local != original
                else:
                    assert ticks == 3 and ran_restore and receipt == done and local == original
                return
            imm = (((insn>>31)&1)<<20)|(((insn>>12)&255)<<12)|(((insn>>20)&1)<<11)|(((insn>>21)&1023)<<1)
            nxt = pc+signed(imm,21)
        else:
            raise AssertionError(f'unsupported instruction {insn:08x} at {pc:08x}')
        if result is not None and rd:
            regs[rd] = result&0xFFFFFFFF
        regs[0] = 0
        pc = nxt
    raise AssertionError('service qualification shim did not park')


if len(sys.argv) != 2:
    raise SystemExit('usage: test.py IMAGE_DIRECTORY')
for role in ('reader', 'writer'):
    execute(Path(sys.argv[1]), role)
    execute(Path(sys.argv[1]), role, invalid_tick=True)
print('PASS actual service entry, simulated role ticks and exact local restore; SIMULATED ONLY')
