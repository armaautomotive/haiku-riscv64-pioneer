#!/usr/bin/env python3
"""Fused OFFLINE role/builder/ISA/unchanged1024frame gates.NO hardware."""
import os
import pathlib
import shlex
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
source = directory / 'blackhole_qwen38_gdn_gated_resident.cpp'
builder = directory / 'build_qwen38_gdn_gated_resident_image.sh'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
def preprocess(flags, accepted):
    global count
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-E',
                             *['-D' + k + '=' + v for k, v in flags.items()], str(source)],
                            env=clean, capture_output=True, text=True, timeout=5)
    assert (result.returncode == 0) == accepted, (flags, result.stderr)
    count += 1
preprocess({'PIONEER_Q38_GDN_GATED_RESIDENT_IMAGE': '1'}, True)
for value in (None, '0', '2', '-1'):
    preprocess({} if value is None else {'PIONEER_Q38_GDN_GATED_RESIDENT_IMAGE': value}, False)
for mixed in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', '_KERNEL_MODE'):
    for value in ('0', '1'):
        preprocess({'PIONEER_Q38_GDN_GATED_RESIDENT_IMAGE': '1', mixed: value}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-qgh1-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for role in ('PIONEER_Q38_GDN_GATED_RESIDENT_IMAGE', 'PIONEER_Q38_GDN_PREPARE_IMAGE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', '00', 'false'):
            result = subprocess.run(['sh', str(builder), str(sdk), str(output)],
                                    env={**clean, role: value}, capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
            count += 1
    commands = [line for line in builder.read_text().splitlines() if line.startswith('awk ')]
    assert len(commands) == 2
    for instruction in ('div', 'divu', 'rem', 'remu', 'mulh', 'mulhu', 'mulhsu', 'fdiv.s', 'fsqrt.s',
                        'fmadd.s', 'fmsub.s', 'fnmadd.s', 'fnmsub.s', 'fcvt.w.s', 'fcvt.wu.s', 'mul', 'fadd.s', 'fmul.s'):
        listing = root / 'disassembly'
        listing.write_text('00064000: 00000000 ' + instruction + ' a0,a1,a2\n')
        command = commands[0].replace('"$output/program.disassembly"', shlex.quote(str(listing)))
        result = subprocess.run(['sh', '-c', command], env=clean, capture_output=True, text=True)
        assert (result.returncode == 0) == (instruction in ('mul', 'fadd.s', 'fmul.s'))
        count += 1
    for frame in (16, 976, 1024, 1025):
        usage = root / 'usage.su'
        usage.write_text(f'actual:1:1:frame\t{frame}\tstatic\n')
        command = commands[1].replace('"$output/"*.su', shlex.quote(str(usage)))
        result = subprocess.run(['sh', '-c', command], env=clean, capture_output=True, text=True)
        assert (result.returncode == 0) == (frame <= 1024)
        count += 1
print(f'PASS QGH1 gates{count},exact offline role/no mixed hostWork-kernel-reference/builders refuse ambient roles BEFORE SDK/unsupported instructions/UNCHANGED1024frame;NO native selector,hardware or source capability')
