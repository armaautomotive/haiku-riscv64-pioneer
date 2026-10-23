#!/usr/bin/env python3
"""QGI1 builder/ISA/stack/old snapshot bound;NO SDK/device/model access."""
import os
import pathlib
import runpy
import shlex
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here / 'pioneer_q38_gdn_prepare_gates_test.py')) # unchanged shared math-role gates50
builder = here.parent / 'build_qwen38_gdn_input_image.sh'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-qgi1-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for flag in ('PIONEER_Q38_GDN_INPUT_IMAGE', 'PIONEER_Q38_GDN_PREPARE_IMAGE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', '00', 'false'):
            result = subprocess.run(['sh', str(builder), str(sdk), str(output)], env={**clean, flag: value},
                                    capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
            count += 1
    commands = [line for line in builder.read_text().splitlines() if line.startswith('awk ')]
    assert len(commands) == 2
    for op in ('div', 'divu', 'rem', 'remu', 'mulh', 'mulhu', 'mulhsu', 'fdiv.s', 'fsqrt.s',
               'fmadd.s', 'fmsub.s', 'fnmadd.s', 'fnmsub.s', 'fcvt.w.s', 'fcvt.wu.s', 'mul', 'fadd.s', 'fmul.s'):
        listing = root / 'disassembly';listing.write_text('00064000: 00000000 ' + op + ' a0,a1,a2\n')
        cmd = commands[0].replace('"$output/program.disassembly"', shlex.quote(str(listing)))
        result = subprocess.run(['sh', '-c', cmd], env=clean, capture_output=True, text=True)
        assert (result.returncode == 0) == (op in ('mul', 'fadd.s', 'fmul.s'))
        count += 1
    for frame in (16, 512, 513):
        usage = root / 'usage';usage.write_text(f'actual:1:1:frame\t{frame}\tstatic\n')
        cmd = commands[1].replace('"$output/program.su"', shlex.quote(str(usage)))
        result = subprocess.run(['sh', '-c', cmd], env=clean, capture_output=True, text=True)
        assert (result.returncode == 0) == (frame <= 512)
        count += 1
    bounds = [line for line in builder.read_text().splitlines() if 'wc -c' in line]
    assert len(bounds) == 1
    for size in (16384, 16385, 32768):
        code = root / 'code';code.write_bytes(bytes(size))
        cmd = bounds[0].replace('"$output/code.bin"', shlex.quote(str(code)))
        result = subprocess.run(['sh', '-c', cmd], env=clean, capture_output=True, text=True)
        assert (result.returncode == 0) == (size <= 16384)
        count += 1
print(f'PASS QGI1 gates{count} plus shared QGP1gates50:ambient roles refused BEFORE SDK,unsupportedISA,512Bframe,16384Bpeer-snapshot-code limit;NO kernel selector or hardware access')
