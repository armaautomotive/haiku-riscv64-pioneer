#!/usr/bin/env python3
"""Execute only the actual fixed fault/STOP instructions; never a model/device."""
import ast
import hashlib
import os
import pathlib
import shutil
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
sdk, image = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and image.is_absolute()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
tree = ast.parse((here.parent/'build_qwen38_token_fault_marker_image.py').read_text())
pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
assert pins, 'pin independent builds before instruction testing'
for name, size, digest in pins:
    data = (image/name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
header = here.parent/'PioneerQ38TokenLayout.h'
subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-x', 'c++', '-fsyntax-only', '-'],
               input='#include "'+str(header)+'"\nstatic_assert(PioneerQ38TokenLayout::C::R::Status==0x75440,"actual status layout");\n',
               text=True, check=True)
nm = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm')
symbols = subprocess.check_output([nm, '-n', str(image/'program.elf')], text=True)
assert symbols == (image/'program.symbols').read_text(), 'symbols must derive from the pinned ELF'
report = [int(line.split()[0], 16) for line in symbols.splitlines()
          if line.split()[-1:] == ['q38_token_fault_report']]
assert len(report) == 1
regions = [(0x40000, (image/'text.bin').read_bytes()), (0x60000, (image/'terminal.bin').read_bytes())]


def word(pc):
    assert not pc & 3
    for base, data in regions:
        if base <= pc and pc+4 <= base+len(data):
            return struct.unpack_from('<I', data, pc-base)[0]
    raise AssertionError('execution escaped the pinned fault/STOP code')


def signed(value, bits):
    return value-(1 << bits) if value & (1 << (bits-1)) else value


def execute(entry, link, fault):
    registers = [(0x13279abc+i*0x110011) & 0xffffffff for i in range(32)]
    registers[0], registers[1] = 0, link
    original = registers[:]
    memory, stores = {}, []
    pc = entry
    fences = 0
    for step in range(64):
        instruction = word(pc)
        op, rd, funct3 = instruction & 127, instruction >> 7 & 31, instruction >> 12 & 7
        rs1, rs2 = instruction >> 15 & 31, instruction >> 20 & 31
        next_pc = pc+4
        if op == 0x37:
            registers[rd] = instruction & 0xfffff000
        elif op == 0x13:
            assert funct3 == 0
            registers[rd] = (registers[rs1]+signed(instruction >> 20, 12)) & 0xffffffff
        elif op == 0x23:
            assert fault and funct3 == 2
            immediate = (instruction >> 25 << 5) | (instruction >> 7 & 31)
            address = (registers[rs1]+signed(immediate, 12)) & 0xffffffff
            expected = [(0x75460, link), (0x75448, 0x51465431)]
            assert len(stores) < 2 and (address, registers[rs2]) == expected[len(stores)]
            if len(stores) == 1:
                assert fences == 1 and memory[0x75460] == link
            memory[address] = registers[rs2]
            stores.append((address, registers[rs2]))
        elif op == 0x0f:
            assert fault and funct3 == 0 and not rd and not rs1
            fences += 1
        elif op == 0x6f:
            immediate = ((instruction >> 31) << 20 | (instruction >> 12 & 255) << 12
                         | (instruction >> 20 & 1) << 11 | (instruction >> 21 & 1023) << 1)
            assert rd == 0, 'fault/STOP must not call or change ra'
            next_pc = (pc+signed(immediate, 21)) & 0xffffffff
        else:
            raise AssertionError(('unexpected fault/STOP opcode', hex(instruction)))
        registers[0] = 0
        if not fault:
            assert next_pc == 0x60004 and not stores and registers == original
        elif step == 0:
            assert next_pc == report[0]
        pc = next_pc
    if fault:
        assert stores == [(0x75460, link), (0x75448, 0x51465431)] and fences == 2
        assert word(pc) == 0x6f and registers[1] == link and registers[2] == original[2]
        assert all(registers[i] == original[i] for i in range(32) if i not in (5, 6))
    return len(stores)


for link in (0, 2, 0x40000, 0x4abcd, 0x60004, 0xffffffff):
    assert execute(0x60000, link, True) == 2
    assert execute(0x60004, link, False) == 0
refusals = 0
with tempfile.TemporaryDirectory(prefix='pioneer-q38-fault-marker-refusals-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    builder = here.parent/'build_qwen38_token_fault_marker_image.py'
    for name in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('0', '1', '2', ''):
            target = root/'MUST_NOT_CREATE'
            result = subprocess.run([sys.executable, str(builder), str(root/'MUST_NOT_READ_SDK'), str(target)],
                                    env={**os.environ, name: value}, capture_output=True, text=True)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not target.exists()
            refusals += 1
    for name, _, _ in pins:
        changed = root/name.replace('.', '-')
        changed.mkdir()
        for file, _, _ in pins:
            shutil.copyfile(image/file, changed/file)
        damaged = bytearray((changed/name).read_bytes())
        damaged[len(damaged)//2] ^= 1
        (changed/name).write_bytes(damaged)
        result = subprocess.run([sys.executable, str(pathlib.Path(__file__).resolve()),
                                 str(root/'MUST_NOT_READ_SDK'), str(changed)],
                                env={**os.environ, 'CXX':str(root/'MUST_NOT_RUN_COMPILER')},
                                capture_output=True, text=True)
        assert result.returncode != 0 and result.stderr.rstrip().endswith('AssertionError: '+name)
        refusals += 1
print('PASS_ACTUAL_FAULT_MARKER_INSTRUCTIONS six_saved_links/two_exact_LOCAL_writes/order_and_fences/unchanged_ra_sp_others/'+
      'no_writes_after_park/six_genuine_STOP_entry_controls_no_fault_writes/refusals'+str(refusals)+
      '_ambient_roles_before_SDK_and_changed_images_before_compiler;NO model_native_physical_Work_or_TPS_qualification')
