#!/usr/bin/env python3
"""Actual QCB1 builder/role/stack/instruction gates,NO native device job."""
import hashlib
import os
import pathlib
import re
import shlex
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
assert len(sys.argv) == 3, 'expected frozen QCB1 and original QCV1 image directories'
batch, single = map(pathlib.Path, sys.argv[1:])
assert hashlib.sha256((batch / 'code.bin').read_bytes()).hexdigest() == 'a8df2da24fa98535c70172c6f9833228c996a540f487b66fa545660b93256350'
assert hashlib.sha256((single / 'code.bin').read_bytes()).hexdigest() == 'd0c9519d0c7ea342a0bbcbede9f9d6355c1c611ea768b9c923f7f645c812d822'
for image in (batch, single):
    assert hashlib.sha256((image / 'boot.bin').read_bytes()).hexdigest() == 'd0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-qcb1-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image',
                 'ffn_matrix_image', 'ffn_persistent_image', 'projection_graph_image',
                 'projection_matrix_image', 'projection_persistent_image'):
        script = directory / ('build_qwen38_' + name + '.sh')
        guards = [line for line in script.read_text().splitlines() if line.startswith('[ ') and 'PIONEER_Q38_CONV_BATCH_IMAGE' in line]
        assert len(guards) == 1
        guard = 'set -eu\n' + guards[0] + '\nprintf GUARD_ACCEPT\n'
        result = subprocess.run(['sh', '-c', guard], env=clean, capture_output=True, text=True, check=True)
        assert result.stdout == 'GUARD_ACCEPT'
        count += 1
        for value in ('1', '2', '-1', '01', '', 'false', '0\n1'):
            for command in (['sh', '-c', guard], ['sh', str(script), str(sdk), str(output)]):
                result = subprocess.run(command, env={**clean, 'PIONEER_Q38_CONV_BATCH_IMAGE': value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
                count += 1
    for name in ('conv_image', 'conv_batch_image'):
        builder = directory / ('build_qwen38_' + name + '.sh')
        for flag in ('PIONEER_Q38_CONV_BATCH_IMAGE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '-1', '', '00', 'false'):
                result = subprocess.run(['sh', str(builder), str(sdk), str(output)], env={**clean, flag: value},
                                        capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
                count += 1
        commands = [line for line in builder.read_text().splitlines() if line.startswith('awk ')]
        assert len(commands) == 2
        for instruction in ('div', 'divu', 'rem', 'remu', 'mulh', 'mulhu', 'mulhsu', 'fdiv.s', 'fsqrt.s',
                            'fmadd.s', 'fmsub.s', 'fnmadd.s', 'fnmsub.s', 'fcvt.w.s', 'fcvt.wu.s', 'mul', 'fadd.s', 'fmul.s'):
            listing = root / 'disassembly'
            listing.write_text('00064000: 00000000 ' + instruction + ' a0,a1,a2\n')
            command = commands[0].replace('"$output/program.disassembly"', shlex.quote(str(listing)))
            result = subprocess.run(['sh', '-c', command], capture_output=True, text=True)
            assert (result.returncode == 0) == (instruction in ('mul', 'fadd.s', 'fmul.s'))
            count += 1
        limit = 2048 if name == 'conv_batch_image' else 1024
        for frame in (16, limit, limit+1):
            usage = root / 'usage'
            usage.write_text(f'actual:1:1:frame\t{frame}\tstatic\n')
            command = commands[1].replace('"$output/program.su"', shlex.quote(str(usage)))
            result = subprocess.run(['sh', '-c', command], capture_output=True, text=True)
            assert (result.returncode == 0) == (frame <= limit)
            count += 1
    source = (directory / 'blackhole_qwen38_conv.cpp').read_text()
    guard = re.findall(r'#if PIONEER_Q38_CONV_BATCH_IMAGE != 1\n#error [^\n]+\n#endif', source)
    assert len(guard) == 1
    for value in ('0', '1', '2', '-1'):
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', '-DPIONEER_Q38_CONV_BATCH_IMAGE=' + value, '-'],
                                input=guard[0], env=clean, capture_output=True, text=True)
        assert (result.returncode == 0) == (value == '1')
        count += 1
print(f'PASS QCB1 gates{count},actual old/new builder refusals BEFORE SDK,exact image role,unsupported instructions/stack bounds,old QCV1 bytes EXACT;NO native selector/install/device access')
