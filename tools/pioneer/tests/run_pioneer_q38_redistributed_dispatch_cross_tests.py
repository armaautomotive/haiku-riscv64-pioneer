#!/usr/bin/env python3
"""Compile the redistributed dispatch candidate for Pioneer RV32IMF; no card."""
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit(2)
sdk = pathlib.Path(sys.argv[1])
assert sdk.is_absolute()
here = pathlib.Path(__file__).resolve().parent
cross = sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku'
with tempfile.TemporaryDirectory(prefix='pioneer-q38-redispatch-cross-', dir='/private/tmp') as tmp:
    obj = pathlib.Path(tmp) / 'dispatch.o'
    subprocess.run([str(cross) + '-g++', '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-ffreestanding', '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie',
        '-ffp-contract=off', '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables',
        '-fno-asynchronous-unwind-tables', '-msmall-data-limit=0', '-mno-relax', '-march=rv32imf',
        '-mabi=ilp32', '-fstack-usage', '-ffixed-s11', '-c',
        str(here / 'pioneer_q38_mixed_recurrent_dispatch_compile.cpp'), '-o', str(obj)], check=True)
    records = [line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
    assert records and all(len(r) == 3 and r[2] == 'static' for r in records)
    frames = {r[0]: int(r[1]) for r in records}
    new_shape = next((size for name, size in frames.items()
        if name.endswith('PioneerQ38GdnRedistributedRendezvousShape(const uint32_t*, const uint32_t*, const uint32_t*)')), None)
    dispatch = next((size for name, size in frames.items()
        if name.endswith('q38_recurrent_gdn_redistributed_command(const uint32_t*, const uint32_t*, const uint32_t*, const uint32_t*, unsigned int, uint32_t*)')), None)
    global_dispatch = next((size for name, size in frames.items()
        if name.endswith('q38_global_recurrent_gdn_redistributed_command(const uint32_t*, const uint32_t*, const uint32_t*, const uint32_t*, unsigned int, uint32_t*)')), None)
    proof = next((size for name, size in frames.items()
        if name.endswith('PioneerQ38ResidentGdnRedistributedServiceJob(const uint32_t*, const uint32_t*, const uint32_t*, const uint32_t*, const uint32_t*, unsigned int, unsigned int)')), None)
    assert new_shape is not None and new_shape <= 2048, frames
    assert dispatch is not None and dispatch <= 1024, frames
    assert global_dispatch is not None and global_dispatch <= 2048, frames
    assert proof is not None and proof <= 1024, frames
    nested = max(dispatch, global_dispatch) + proof + new_shape
    assert nested <= 8192, (dispatch, global_dispatch, proof, new_shape)
    for name, size in frames.items():
        if 'Redistributed' not in name and 'redistributed' not in name:
            assert size <= 1024, (name, size)
    assert not subprocess.check_output([str(cross) + '-nm', '-u', str(obj)])
    disassembly = subprocess.check_output([str(cross) + '-objdump', '-d', str(obj)], text=True)
    assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|ebreak|s11)\b|\bcsr', disassembly)
    for line in disassembly.splitlines():
        match = re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s', line)
        if match:
            assert int(match[1], 16) & 127 != 0x73, line
    digest = hashlib.sha256(obj.read_bytes()).hexdigest()
    print(f'PASS redistributed GDN mixed/global dispatch RV32IMF object {digest}; mixedFrame={dispatch}, globalFrame={global_dispatch}, proofFrame={proof}, shapeFrame={new_shape}, nestedUpperBound={nested}/8192; no undefined symbols, M-extension or CSR; no card/native/speed qualification')
