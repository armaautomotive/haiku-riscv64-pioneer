#!/usr/bin/env python3
"""OFFLINE QCV1 build/helper isolation and actual instruction/stack gates."""
import hashlib
import os
import pathlib
import shlex
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
assert len(sys.argv) == 2, 'expected frozen QCV1 image directory'
image = pathlib.Path(sys.argv[1])
assert hashlib.sha256((image / 'code.bin').read_bytes()).hexdigest() == 'd0c9519d0c7ea342a0bbcbede9f9d6355c1c611ea768b9c923f7f645c812d822'
assert hashlib.sha256((image / 'boot.bin').read_bytes()).hexdigest() == 'd0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-qcv1-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image',
                 'ffn_matrix_image', 'ffn_persistent_image', 'projection_graph_image',
                 'projection_matrix_image', 'projection_persistent_image'):
        script = directory / ('build_qwen38_' + name + '.sh')
        lines = [line for line in script.read_text().splitlines() if line.startswith('[ ') and 'PIONEER_Q38_CONV_NATIVE' in line]
        assert len(lines) == 1
        guard = 'set -eu\n' + lines[0] + '\nprintf GUARD_ACCEPT\n'
        okay = subprocess.run(['sh', '-c', guard], env=clean, capture_output=True, text=True, check=True)
        assert okay.stdout == 'GUARD_ACCEPT'
        count += 1
        for flag in ('PIONEER_Q38_CONV_NATIVE', 'PIONEER_Q38_CONV_CLIENT', 'PIONEER_Q38_CONV_IMAGE'):
            for value in ('1', '2', '01', '-1', '', 'false', '0\n1'):
                env = {**clean, flag: value}
                for command in (['sh', '-c', guard], ['sh', str(script), str(sdk), str(output)]):
                    result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=5)
                    assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists(), (name, flag, value)
                    count += 1
    builder = directory / 'build_qwen38_conv_image.sh'
    for flag in ('PIONEER_Q38_CONV_NATIVE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', '00', 'false'):
            result = subprocess.run(['sh', str(builder), str(sdk), str(output)], env={**clean, flag: value},
                                    capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
            count += 1
    # Evaluate ACTUAL helper guard;no host Work or floating kernel fallback.
    prefix = (directory / 'PioneerQ38ConvMath.h').read_text().split('#include', 1)[0] + '#endif\n'
    for flags in ([], ['-DPIONEER_Q38_WORK_INFERENCE=0'], ['-DPIONEER_Q38_WORK_INFERENCE=1'],
                  ['-D_KERNEL_MODE=0'], ['-D_KERNEL_MODE=1']):
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', *flags, '-'], input=prefix,
                                env=clean, capture_output=True, text=True, timeout=5)
        assert (result.returncode == 0) == (not flags)
        count += 1
    # Exercise actual builder instruction and frame checks,not a lookalike.
    commands = [line for line in builder.read_text().splitlines() if line.startswith('awk ')]
    assert len(commands) == 2
    for instruction in ('div', 'divu', 'rem', 'remu', 'mulh', 'mulhu', 'mulhsu', 'fdiv.s', 'fsqrt.s',
                        'fmadd.s', 'fmsub.s', 'fnmadd.s', 'fnmsub.s', 'fcvt.w.s', 'fcvt.wu.s', 'mul', 'fadd.s', 'fmul.s'):
        listing = root / 'disassembly'
        listing.write_text('00064000:  00000000  ' + instruction + ' a0,a1,a2\n')
        command = commands[0].replace('"$output/program.disassembly"', shlex.quote(str(listing)))
        result = subprocess.run(['sh', '-c', command], capture_output=True, text=True)
        assert (result.returncode == 0) == (instruction in ('mul', 'fadd.s', 'fmul.s'))
        count += 1
    for frame, okay in ((16, True), (1024, True), (1025, False), (0, True)):
        usage = root / 'usage'
        usage.write_text(f'example:1:1:actual\t{frame}\tstatic\n')
        command = commands[1].replace('"$output/program.su"', shlex.quote(str(usage)))
        result = subprocess.run(['sh', '-c', command], capture_output=True, text=True)
        assert (result.returncode == 0) == okay
        count += 1
print(f'PASS QCV1 isolation/build gates{count},old builders refuse BEFORE SDK/output,hostWork/kernel math denied,actual bad instruction/stack refusal,frozen code+boot;NO native selector/install/device access')
