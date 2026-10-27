#!/usr/bin/env python3
"""New recurrent dispatch DATA bridge with actual unchanged workers; no IO."""
import os
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile
args=sys.argv[1:];sdk=None
if args[:1]==['--sdk']:
    assert len(args)==3
    sdk=pathlib.Path(args[1]);args=args[2:]
if len(args) != 1:
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
image = pathlib.Path(args[0])
PINS = (
    ('boot.bin', 4, 'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('mixed.bin', 4400, 'daae5eab80a4e423d9b33a1eff84b39de6d29b7b4d85fe52a7e00e24bc3178f2'),
    ('dense.bin', 10332, 'cef405cc7405eea6001e47e9a6c9db73ca790bcac985f09d2bb964de8c68045f'),
    ('text.bin', 5984, '3a8ffe4f7e885f992f210bd1cc0887a1309159972409403b3082535b1916dab1'),
    ('affine.bin', 5360, '47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
    ('ffn.bin', 16456, 'f6ef5d614091b5ed12365e1945daa2dad04d37c432ac9dd5f859f857bba83e52'),
    ('service.bin', 9888, 'b9fb256a75546dfe9be4a7805ef170913b2b1eecab1fbf1afb3b6d7c098c9262'),
    ('gdn.bin', 35564, 'eeaa084e659efbc5f2b7c9922a9de862057b2ed624bf85b03b8ccde2c02c5c74'),
    ('program.elf', 121680, '6a74556d6f4095353b0bffd0f44fe3d35706c21ea304f5233d034e47702117ea'),
)
for name, size, digest in PINS:
    data = (image/name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-dispatch-test-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, str(here/'pioneer_q38_mixed_recurrent_dispatch_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *args], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    if sdk:
        cross=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku')
        obj=pathlib.Path(tmp)/'dispatch.o'
        subprocess.run([cross+'-g++','-std=c++11','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-exceptions','-fno-rtti',
            '-fno-stack-protector','-fno-pic','-fno-pie','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-unwind-tables',
            '-fno-asynchronous-unwind-tables','-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-ffixed-s11',
            '-c',str(here/'pioneer_q38_mixed_recurrent_dispatch_compile.cpp'),'-o',str(obj)],check=True)
        frames=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
        assert frames and all(len(r)==3 and r[2]=='static' and int(r[1])<=1024 for r in frames)
        assert not subprocess.check_output([cross+'-nm','-u',str(obj)])
        dis=subprocess.check_output([cross+'-objdump','-d',str(obj)],text=True)
        assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|ebreak|s11)\b',dis)
        for line in dis.splitlines():
            match=re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s',line)
            if match:assert int(match[1],16)&127!=0x73
        digest=hashlib.sha256(obj.read_bytes()).hexdigest()
        assert digest=='bd038ffb9469800046f0f3e6d64b8af51e3c8d501e3f312f0292791051ccb8fb'
        print('PASS returning RV32 dispatch DATA object '+digest+'/maxFrame'+str(max(int(r[1]) for r in frames))+
            '/no undefined symbols/CSR/s11;NOT linked ROOT/native execution')
