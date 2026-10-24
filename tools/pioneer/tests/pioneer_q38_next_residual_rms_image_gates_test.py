#!/usr/bin/env python3
"""Exact offline image/codeword/builder and existing lease/stack gates."""
import hashlib
import os
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
images = [pathlib.Path(p) for p in sys.argv[1:]]
pin = 'f2fdcb0becf99281ba6910dbb64551d32584c0cc054bc452f030217ea5e81636'
for image in images:
    data = (image/'code.bin').read_bytes()
    assert len(data) == 5512 and hashlib.sha256(data).hexdigest() == pin
    assert (image/'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
    assert ' U ' not in (image/'program.symbols').read_text()
    frames = [line.split('\t') for line in (image/'program.su').read_text().splitlines()]
    assert frames and max(int(row[1]) for row in frames) == 400 and all(row[2] == 'static' for row in frames)
    disassembly = (image/'program.disassembly').read_text()
    assert not re.search(r'\b(?:fdiv\.[sd]|fsqrt\.[sd]|f(?:add|sub|mul)\.d|divu?|remu?|fmadd|fmsub|fnmadd|fnmsub)\b', disassembly)
    assert not re.search(r'\b(?:memcpy|memset|__div|__udiv|__float|__adddf|__muldf)\b', disassembly)
assert (images[0]/'code.bin').read_bytes() == (images[1]/'code.bin').read_bytes()
source = (directory/'blackhole_qwen38_next_residual_rms.cpp').read_text().split('#define PIONEER_ATTENTION_FREESTANDING', 1)[0]
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
count = 0


def check(flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=source, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


role = 'PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE'
check({role: '1'}, True)
check({}, False)
for value in ('0', '2', '-1'):
    check({role: value}, False)
for flag in sorted(set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)', source))-{role}):
    for value in ('0', '1'):
        check({role: '1', flag: value}, False)
driver = (kernel/'tenstorrent_pci_probe.cpp').read_text().split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
for value in ('0', '1', '2', '-1'):
    result = subprocess.run([cxx, '-x', 'c++', '-E', '-D'+role+'='+value, '-'], input=driver, capture_output=True, text=True, env=clean, timeout=5)
    assert result.returncode and not result.stderr.startswith('cc1plus: error: unrecognized'), result.stderr
    count += 1
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent', 'GdnRendezvous', 'GdnOutput', 'NextProjection', 'NextGdnOutput'):
    client = 'GDN_OUTPUT' if name=='GdnOutput' else 'GDN_RENDEZVOUS' if name=='GdnRendezvous' else 'GDN_RECURRENT' if name=='GdnRecurrent' else 'GDN_INPUT'
    flags = ['-DPIONEER_Q38_'+client+'_VALIDATION_CLIENT=1', '-DPIONEER_Q38_CPU_VALIDATION=1', '-DPIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE=1']
    if name=='NextGdnOutput':
        flags = ['-DPIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT=1', '-DPIONEER_Q38_CPU_VALIDATION=1', '-DPIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE=1']
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    for value in ('0', '1'):
        result = subprocess.run([cxx, '-x', 'c++', '-E', *flags, '-D'+role+'='+value, '-'], input=text, capture_output=True, text=True, env=clean, timeout=5)
        assert result.returncode, name
        result = subprocess.run([cxx, '-x', 'c++', '-E', *flags, '-'], input=text+'#define '+role+' '+value+'\n'+text, capture_output=True, text=True, env=clean, timeout=5)
        assert result.returncode, name
        count += 2
with tempfile.TemporaryDirectory(prefix='pioneer-qrr2-gates-') as temporary:
    root = pathlib.Path(temporary)
    for flag in (role, 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '', 'false'):
            result = subprocess.run(['sh', str(directory/'build_qwen38_next_residual_rms_image.sh'), str(root/'NO_SDK'), str(root/'NO_OUTPUT')],
                                    env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
    for builder, arguments in (
            ('build_qwen38_inference.sh', [str(root/'NO_SDK'), str(root/'NO_LIB'), str(root/'NO_OUTPUT')]),
            *[('build_qwen38_'+name+'.sh', [str(root/'NO_SDK'), str(root/'NO_OUTPUT')]) for name in
              ('banks_client', 'banks_driver', 'local_image', 'ffn_graph_image', 'ffn_matrix_image', 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image')]):
        for value in ('1', '2', '', 'false'):
            result = subprocess.run(['sh', str(directory/builder), *arguments], env={**clean, role: value},
                                    capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
        baseline = subprocess.run(['sh', str(directory/builder), *arguments], env=clean,
                                  capture_output=True, text=True, timeout=5)
        assert baseline.returncode and not baseline.stdout and not (root/'NO_OUTPUT').exists()
        for value in (None, '0'):
            result = subprocess.run(['sh', str(directory/builder), *arguments], env={**clean, **({role: value} if value is not None else {})},
                                    capture_output=True, text=True, timeout=5)
            # Older builders have different arities;neutral image=0 must
            # preserve each one's exact existing refusal,not invent a
            # shared missing-SDK exit code or require unwanted SDK reads.
            assert (result.returncode, result.stdout, result.stderr) == (baseline.returncode, baseline.stdout, baseline.stderr) and not (root/'NO_OUTPUT').exists()
            count += 1
    generator = directory/'generate_qwen38_next_residual_rms_image.py'
    good = subprocess.run([sys.executable, str(generator), str(images[0])], capture_output=True, text=True, check=True)
    target = kernel/'PioneerQ38NextResidualRmsCode.h'
    expected = '*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+target.name+'\n'
    expected += ''.join('+'+line+'\n' for line in target.read_text().splitlines())+'*** End Patch\n'
    assert good.stdout == expected
    for kind in range(4):
        data = bytearray((images[0]/'code.bin').read_bytes())
        boot = bytearray((images[0]/'boot.bin').read_bytes())
        if kind < 2:
            data[0 if kind == 0 else -1] ^= 1
        elif kind == 2:
            data += b'\0\0\0\0'
        else:
            boot[0] ^= 1
        bad = root/('bad'+str(kind));bad.mkdir();(bad/'code.bin').write_bytes(data);(bad/'boot.bin').write_bytes(boot)
        refused = subprocess.run([sys.executable, str(generator), str(bad)], capture_output=True, text=True)
        assert refused.returncode and not refused.stdout
        count += 1
    code = '#include <cassert>\n#include "'+str(target)+'"\nint main(){\n'
    code += 'assert(PioneerQ38NextResidualRmsProgramWord(0)==0x0005e06f);\n'
    for i, value in enumerate(struct.unpack('<1378I', (images[0]/'code.bin').read_bytes()), 1):
        code += 'assert(PioneerQ38NextResidualRmsProgramWord('+str(i)+')=='+hex(value)+'U);\n'
    code += 'for(unsigned i=1379;i<=10240;i++)assert(!PioneerQ38NextResidualRmsProgramWord(i));assert(PioneerQ38NextResidualRmsProgramWord(10241)==UINT32_MAX);}\n'
    path, binary = root/'words.cpp', root/'words'
    path.write_text(code)
    subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print(f'PASS QRR2 gates{count}/two5512B images EXACT{pin[:8]}/frozen1378words+boot+old40KiB padding/400Bmaxframe/no unsupported instructions/helpers;NO native/Work selector')
