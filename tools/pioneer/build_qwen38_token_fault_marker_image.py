#!/usr/bin/env python3
"""Separate fault-reporting ROOT BUILD only; no driver selector or card IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS = (
    ('boot.bin', 4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    ('text.bin', 130920, '3a9e727702c225bc20d2f08153b36b98aa8259cc029f33a151d8d1fd1ec7c26e'),
    ('terminal.bin', 8, '468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    ('conv.bin', 32720, 'f143a965e082cb1f1bd1cf6af48a62d17590935277110799b30a410821ba77b3'),
    ('program.elf', 190500, 'efeebcfc25e00b6519026428b1d7af3c12052dad6db2e6e2b52110e3fb7ed7c6'),
)  # Independently identical BUILD pins ONLY;never native/physical/Work authority.
args = sys.argv[1:]
inspect = len(args) == 3 and args[-1] == '--inspect-candidate'
if inspect:
    args.pop()
if len(args) != 2 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ) or (not inspect and not PINS):
    raise SystemExit(2)
sdk, out = map(pathlib.Path, args)
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert (sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++').is_file()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    # Rebuild the unchanged,strictly pinned QTP3 parent FIRST. Its actual
    # compiled arithmetic/guard/metadata objects are reused without edits.
    sys.argv = [str(here/'build_qwen38_token_root.py'), str(sdk), str(out/'unchanged-root'),
                '--cancel-boundary', '--f32-hidden', '--unroll-dense-seals', '--boundary-seals',
                '--unroll-quiet', '--worker-local-source-guards', '--precise-activation',
                '--single-closed-audit', '--worker-sealed-admission', '--pushed-completion']
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
cross = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')


def run(*values):
    subprocess.run(list(map(str, values)), check=True)


old = out/'unchanged-root'
run(cross+'as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax',
    here/'blackhole_qwen38_token_fault_start.S', '-o', out/'start.o')
run(cross+'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', here/'blackhole_qwen38_global_chain.ld',
    out/'start.o', old/'program.o', old/'qualified-components/token-components-NOBOOT.o',
    old/'qualified-components/unchanged-feedback/feedback.o', '-Map', out/'program.map', '-o', out/'program.elf')
for section in ('boot', 'text', 'terminal', 'conv'):
    run(cross+'objcopy', '-O', 'binary', '-j', '.'+section, out/'program.elf', out/(section+'.bin'))
symbols = subprocess.check_output([cross+'nm', '-n', str(out/'program.elf')], text=True)
assert not subprocess.check_output([cross+'nm', '-u', str(out/'program.elf')], text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name, address in (('_start', 0x6000), ('scalar_entry', 0x40000),
                      ('scalar_fault_park', 0x60000), ('q38_global_chain_stopped', 0x60004)):
    assert re.search('^'+format(address, '08x')+' T '+name+'$', symbols, re.M), name
assert re.search(r'^[0-9a-f]+ T q38_token_fault_report$', symbols, re.M)
assert (out/'boot.bin').read_bytes() == (old/'boot.bin').read_bytes()
assert (out/'terminal.bin').read_bytes()[4:] == (old/'terminal.bin').read_bytes()[4:]
assert (out/'terminal.bin').read_bytes() != (old/'terminal.bin').read_bytes()
assert len((out/'text.bin').read_bytes()) == len((old/'text.bin').read_bytes())+32
assert len((out/'conv.bin').read_bytes()) == len((old/'conv.bin').read_bytes())
disassembly = subprocess.check_output([cross+'objdump', '-d', str(out/'program.elf')], text=True)
# The only newly compiled executable source is the fixed assembly entry;
# the unchanged parent already checks the complete linked arithmetic ISA.
writer = subprocess.check_output([cross+'objdump', '-d', '--disassemble=q38_token_fault_report',
                                  str(out/'program.elf')], text=True)
assert not re.search(r'\b(?:csr\w*|ebreak|ecall|divu?|remu?|mulh\w*|flw|fsw|f(?:add|sub|mul|div|sqrt|madd|msub|nmadd|nmsub|cvt|sgnj|mv|class|eq|le|lt)\.\w+)\b',
                     '\n'.join(line for line in writer.splitlines() if re.match(r'\s*[0-9a-f]+:', line)))
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_text(disassembly)
observed = []
for name in ('boot.bin', 'text.bin', 'terminal.bin', 'conv.bin', 'program.elf'):
    data = (out/name).read_bytes()
    row = (name, len(data), hashlib.sha256(data).hexdigest())
    observed.append(row)
    print(row, flush=True)
if not inspect:
    assert tuple(observed) == PINS, 'unqualified fault-marker build'
print('BUILD_ONLY_UNSELECTED_FAULT_MARKER status_error51465431 status_saved_link_NOT_retired_PC/'+
      'same_compiled_arithmetic_objects_and_STOP_loop/NO timeout_extension_retry_cancel_reset/'+
      'NO native_driver_selection_original_model_physical_Work_or_TPS_permission', flush=True)
