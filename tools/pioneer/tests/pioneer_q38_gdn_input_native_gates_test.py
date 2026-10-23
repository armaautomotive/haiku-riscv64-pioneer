#!/usr/bin/env python3
"""Actual tag45 guards and old builder isolation;NO SDK/device/model access."""
import os
import ast
import hashlib
import pathlib
import re
import struct
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
valid = dict(PIONEER_Q38_GDN_INPUT_OWNER='1', PIONEER_Q38_RESIDENT_DECODER_NATIVE='1', PIONEER_Q38_RESIDENT_DECODER_OWNER='1',
             PIONEER_Q38_DECODER_RUNTIME='1', PIONEER_Q38_PROJECTION_EXECUTION_OWNER='1', PIONEER_Q38_PROJECTION_RESULT_STASH='1',
             PIONEER_Q38_LEARNED_RMS_NATIVE='1', PIONEER_Q38_EMBEDDING_NATIVE='1', TT_PIONEER_DRIVER_RELOAD='1',
             PIONEER_DRIVER_BUILD_TAG='0x2026102300010045ULL')
count = 0
def preprocess(flags, accepted):
    global count
    r = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                       input=prefix, env=clean, capture_output=True, text=True)
    assert (r.returncode == 0) == accepted, (flags, r.stderr)
    count += 1
preprocess({}, True)
preprocess(valid, True)
for key in valid:
    for value in (None, '0', '2', '-1'):
        bad = dict(valid)
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        preprocess(bad, False)
for tag in ('0x2026102200010038ULL', '0x2026102200010042ULL', '0x2026102300010043ULL', '0x2026102300010044ULL'):
    preprocess({**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
preprocess({'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for flag in ('PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE', 'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE',
             'PIONEER_Q38_EMBEDDING_QUALIFICATION', 'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        preprocess({**valid, flag: value}, False)
assert 'PIONEER_Q38_GDN_INPUT_NATIVE_CONTROL_BEGIN' not in source, 'new stage must remain selector-free until its actual public qualification'
with tempfile.TemporaryDirectory(prefix='pioneer-gdn-input-native-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    compiler_source = here / 'pioneer_q38_gdn_input_native_full_compile.py'
    tree = ast.parse(compiler_source.read_text(), filename=str(compiler_source))
    function = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'frames')
    namespace = dict(pathlib=pathlib, re=re)
    exec(compile(ast.Module(body=[function], type_ignores=[]), str(compiler_source), 'exec'), namespace)
    usage = root / 'frames.su'
    usage.write_text(f'{root}/unused/../actual.h:1:2:clone()\t16\tstatic\n'
                     f'{root}/actual.h:9:8:clone()\t512\tstatic\n'
                     f'{root}/actual.h:7:6:clone()\t32\tstatic\n')
    assert namespace['frames'](usage) == {str((root / 'actual.h').resolve()) + ':clone()': 512}
    count += 1
    loop = next(n for n in tree.body if isinstance(n, ast.For) and isinstance(n.target, ast.Tuple)
                and [v.id for v in n.target.elts] == ['name', 'size'])
    assert isinstance(loop.body[0], ast.Assert)
    condition = compile(ast.Expression(body=loop.body[0].test), str(compiler_source), 'eval')
    for size, before, accepted in ((2048, None, True), (2049, None, False), (2560, 2560, True),
                                    (2561, 2560, False), (3000, 2560, False), (2560, 3000, True)):
        previous = {} if before is None else {'same_actual_function': before}
        assert bool(eval(condition, dict(name='same_actual_function', size=size, previous=previous))) == accepted
        count += 1
    begin, end = '// PIONEER_Q38_GDN_INPUT_NATIVE_INERT_ROUTING_BEGIN', '// PIONEER_Q38_GDN_INPUT_NATIVE_INERT_ROUTING_END'
    assert source.count(begin) == source.count(end) == 1
    route = source.split(begin, 1)[1].split(end, 1)[0].strip()
    assert route == 'return B_NOT_ALLOWED; // COMPILE-ONLY tag45 has NO public execution/upload/old Work selector'
    assert source.index('return DriverReloadControl(instance,operation,data,length);') < source.index(begin)
    start = 'static status_t\nProbeControl(void* cookie, uint32 operation, void* data, size_t length)\n{'
    prelude = start + source.split(start, 1)[1].split('#ifdef TT_PIONEER_DRIVER_RELOAD', 1)[0]
    prelude = prelude.replace('{\n', '{\n (void)operation;(void)data;(void)length;\n', 1)
    code = '#include <cassert>\n#include <cstddef>\nusing uint32=unsigned;using status_t=int;\n' \
           'int uid=0;int geteuid(){return uid;}enum{B_NOT_ALLOWED=-1};struct PioneerProbeCookie{unsigned magic;};\n' \
           + prelude + route + '\n}\nint main(){PioneerProbeCookie c={0x50313030};\n' \
           'for(unsigned op=0;op<131072;op++)assert(ProbeControl(&c,op,nullptr,0)==B_NOT_ALLOWED);\n' \
           'assert(ProbeControl(nullptr,0,nullptr,0)==B_NOT_ALLOWED);uid=1;assert(ProbeControl(&c,0,nullptr,0)==B_NOT_ALLOWED);}\n'
    binary = root / 'inert-routing'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-x', 'c++', '-', '-o', str(binary)],
                   input=code, env=clean, text=True, check=True)
    subprocess.run([str(binary)], env=clean, check=True)
    count += 1
    header = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputImage.h').read_text()
    words = re.findall(r'0x([0-9a-f]{8})U', header.split('code[2101]={', 1)[1].split('};', 1)[0])
    assert len(words) == 2101
    assert hashlib.sha256(struct.pack('<2101I', *[int(w, 16) for w in words])).hexdigest() == 'bcc15ddc0e76c0a432a63502201f9c2451366b02615e3cddc9cf64bb147757c1'
    count += 1
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        script = directory / ('build_qwen38_' + name + '.sh')
        guards = [line for line in script.read_text().splitlines() if line.startswith('[ ') and 'PIONEER_Q38_GDN_INPUT_OWNER' in line]
        assert len(guards) == 1
        for value in (None, '0'):
            env = clean if value is None else {**clean, 'PIONEER_Q38_GDN_INPUT_OWNER': value}
            subprocess.run(['sh', '-c', 'set -eu\n' + guards[0]], env=env, check=True)
            count += 1
        for value in ('1', '2', '-1', '01', '', 'false', '0\n1'):
            r = subprocess.run(['sh', str(script), str(sdk), str(output)],
                               env={**clean, 'PIONEER_Q38_GDN_INPUT_OWNER': value}, capture_output=True, text=True, timeout=5)
            assert r.returncode == 2 and not r.stdout and not r.stderr and not output.exists()
            count += 1
    for name in ('pioneer_q38_gdn_input_native_full_compile.py', 'pioneer_q38_resident_decoder_native_full_compile.py'):
        for value in ('1', '2', '-1', '', '00', 'false'):
            r = subprocess.run(['python3', str(here / name), str(sdk), str(output)],
                               env={**clean, 'PIONEER_Q38_GDN_INPUT_OWNER': value}, capture_output=True, text=True, timeout=5)
            assert r.returncode == 2 and not r.stdout and not r.stderr and not output.exists()
            count += 1
print(f'PASS actual tag45 gates{count},exact SAME-owner role only,old reserved tags/mixed Work-reference/old DEBUG rejected,legacy builders refuse ambient promotion BEFORE SDK/output,ROM pin,131072 inert routing probes,strict2048Bframe/worst-clone/no-legacy-growth (reload remains separate);NO public selector or device access')
