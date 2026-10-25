#!/usr/bin/env python3
"""Exact separate precise-QCB1 image; unchanged old code and closed roles."""
import hashlib
import os
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 5
here = pathlib.Path(__file__).resolve().parent
directory = here.parent
images = [pathlib.Path(p) for p in sys.argv[1:]]
pins = ('0a009317f69ecba0d6f56575508d4d4e6bdb3c30ece3e1f78a85cd66e2ab644c',)*2 + (
        'a8df2da24fa98535c70172c6f9833228c996a540f487b66fa545660b93256350',
        'd0c9519d0c7ea342a0bbcbede9f9d6355c1c611ea768b9c923f7f645c812d822')
for image, pin in zip(images, pins):
    assert hashlib.sha256((image/'code.bin').read_bytes()).hexdigest() == pin
    assert (image/'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
for image in images[:2]:
    assert len((image/'code.bin').read_bytes()) == 7560 and not (image/'program.undefined').read_bytes()
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (image/'program.symbols').read_text(), re.M)
    frames = [line.split('\t') for line in (image/'program.su').read_text().splitlines()]
    assert frames and max(int(row[1]) for row in frames) == 1744 and all(row[2] == 'static' for row in frames)
    assert not re.search(r'\b(?:fdiv\.[sd]|fsqrt\.[sd]|f(?:add|sub|mul)\.d|divu?|remu?|mulh[su]*|fmadd|fmsub|fnmadd|fnmsub)\b', (image/'program.disassembly').read_text())
assert (images[0]/'code.bin').read_bytes() == (images[1]/'code.bin').read_bytes()
role = 'PIONEER_Q38_SILU_PRECISION_IMAGE'
text = '#include "'+str(directory/'PioneerQ38ConvMath.h')+'"\n'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
count = 0


def check(source, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=source, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(text, {}, True)
check(text, {role: '1'}, True)
for value in ('0', '2', '-1', ''):
    check(text, {role: value}, False)
    check(text+'#undef '+role+'\n#define '+role+' '+value+'\n'+text, {role: '1'}, False)
check(text+'#define '+role+' 1\n'+text, {}, False)
check(text+'#undef '+role+'\n'+text, {role: '1'}, False)
for flag in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE', '__FAST_MATH__'):
    for value in ('0', '1'):
        check(text, {role: '1', flag: value}, False)
        check(text+'#define '+flag+' '+value+'\n'+text, {role: '1'}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-precise-qcb-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    builder = directory/'build_qwen38_conv_batch_silu_precision_image.sh'
    for flag in (role, 'PIONEER_Q38_CONV_SILU_PRECISION_BODY', 'PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run(['sh', str(builder), str(root/'NO_SDK'), str(root/'NO_OUTPUT')],
                                    capture_output=True, text=True, env={**clean, flag: value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
    name = 'PioneerQ38ConvBatchSiluPrecision'
    target = here.parents[2]/'src/add-ons/kernel/drivers/misc'/(name+'Image.h')
    generator = directory/'generate_qwen38_conv_batch_silu_precision_image.py'
    generated = subprocess.run([sys.executable, str(generator), str(images[0])], capture_output=True, text=True, check=True)
    expected = '*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+target.name+'\n'
    expected += ''.join('+'+line+'\n' for line in target.read_text().splitlines())+'*** End Patch\n'
    assert generated.stdout == expected
    for kind in range(6):
        code, boot = bytearray((images[0]/'code.bin').read_bytes()), bytearray((images[0]/'boot.bin').read_bytes())
        if kind < 2:
            code[0 if kind == 0 else -1] ^= 1
        elif kind == 2:
            code = code[:-4]
        elif kind == 3:
            code += b'\0\0\0\0'
        elif kind == 4:
            boot[0] ^= 1
        else:
            boot = boot[:-1]
        bad = root/('bad'+str(kind));bad.mkdir();(bad/'code.bin').write_bytes(code);(bad/'boot.bin').write_bytes(boot)
        result = subprocess.run([sys.executable, str(generator), str(bad)], capture_output=True, text=True)
        assert result.returncode and not result.stdout
        count += 1
    code = '#include <cassert>\n#include <cstdio>\n#include "'+str(target)+'"\n'
    code += 'int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);\n'
    code += 'assert('+name+'ImageWord(0)==0x0005e06f && '+name+'ImageAddress(0)==0x6000);\n'
    code += 'for(unsigned i=0;i<1890;i++){uint32_t value;assert(fread(&value,4,1,f)==1);assert('+name+'CodeWord(i)==value && '+name+'ImageWord(i+1)==value && '+name+'ImageAddress(i+1)==0x64000+i*4);}\n'
    code += 'assert(fgetc(f)==EOF && !fclose(f));assert('+name+'CodeWord(1890)==UINT32_MAX && '+name+'CodeWord(UINT32_MAX)==UINT32_MAX);\n'
    code += 'assert('+name+'ImageWord(1891)==UINT32_MAX && '+name+'ImageAddress(1891)==UINT32_MAX && '+name+'ImageWord(UINT32_MAX)==UINT32_MAX && '+name+'ImageAddress(UINT32_MAX)==UINT32_MAX);\n'
    code += 'assert('+name+'TerminalPc(0x64038) && !'+name+'TerminalPc(0x64058) && !'+name+'TerminalPc(0x64000));}\n'
    path, binary = root/'words.cpp', root/'words';path.write_text(code)
    subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(images[0]/'code.bin')], check=True)
    count += 1
print(f'PASS precise QCB1 gates{count}/two7560B images EXACT0a009317/1744Bmaxframe/old QCB1+QCV1 BYTE-IDENTICAL/exact arithmetic role+reinclude+CPU/native/Work exclusion/builder refusal/ALL1890frozenwords+boot+addresses+endpoints/six generator refusals;NO native or physical or TPS grant')
