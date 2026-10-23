#!/usr/bin/env python3
"""Exact compiled ROM/footprint/header/actual early native-builder refusals."""
import os
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
headers = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
assert len(sys.argv) == 2, 'expected frozen QCB1 image'
image = pathlib.Path(sys.argv[1])
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
flags = ['-DPIONEER_Q38_GDN_MUTABLE_STATE', '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1',
         '-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1', '-DPIONEER_Q38_PROJECTION_RESULT_STASH=1', '-DPIONEER_Q38_DECODER_RUNTIME=1']
with tempfile.TemporaryDirectory(prefix='pioneer-decoder-owner-gates-') as temporary:
    root = pathlib.Path(temporary)
    binary = root / 'rom'
    program = '''#include <cassert>
#include <cstdio>
#include "PioneerQ38ConvBatchImage.h"
#include "PioneerQ38ConvBatchShape.h"
#include "PioneerQ38ProjectionGraphFootprint.h"
int main(){
 for(unsigned w=0;w<120;w++){
  for(unsigned i=0;i<1523;i++)assert(PioneerQ38ProjectionGraphSnapshotContains(w,PioneerQ38ConvBatchImageAddress(i)));
  for(unsigned i=0;i<384;i++)assert(PioneerQ38ProjectionGraphSnapshotContains(w,TT_Q38_CONV_BATCH_TABLE+i*4)
   && !PioneerQ38ProjectionGraphProgramImmutable(w,TT_Q38_CONV_BATCH_TABLE+i*4));
  for(unsigned a=0x6e000;a<0x70000;a+=4)assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));
  for(unsigned a=0x80000;a<=0x8103c;a+=4)assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));
 }
 assert(PioneerQ38ConvBatchImageAddress(1523)==UINT32_MAX && PioneerQ38ConvBatchCodeWord(1522)==UINT32_MAX);
 assert(PioneerQ38ConvBatchTerminalPc(0x64038) && !PioneerQ38ConvBatchTerminalPc(0x64058));
 for(unsigned i=0;i<1523;i++){uint32_t v=PioneerQ38ConvBatchImageWord(i);assert(fwrite(&v,4,1,stdout)==1);}
}'''
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(headers),
                    '-DPIONEER_Q38_PROJECTION_RESULT_STASH=1', '-x', 'c++', '-o', str(binary), '-'], input=program, text=True, check=True)
    actual = subprocess.run([str(binary)], capture_output=True, check=True).stdout
    assert actual == (image / 'boot.bin').read_bytes() + (image / 'code.bin').read_bytes()
    assert len(actual) == 6092 and struct.unpack('<I', actual[:4])[0] == 0x0005e06f
    count += 1
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        script = directory / ('build_qwen38_' + name + '.sh')
        guards = [line for line in script.read_text().splitlines() if line.startswith('[ ') and 'PIONEER_Q38_RESIDENT_DECODER_OWNER' in line]
        assert len(guards) == 1
        guard = 'set -eu\n' + guards[0] + '\nprintf GUARD_ACCEPT\n'
        for value in (None, '0'):
            env = clean if value is None else {**clean, 'PIONEER_Q38_RESIDENT_DECODER_OWNER': value}
            result = subprocess.run(['sh', '-c', guard], env=env, capture_output=True, text=True, check=True)
            assert result.stdout == 'GUARD_ACCEPT'
            count += 1
        for value in ('1', '2', '-1', '01', '', 'false', '0\n1'):
            for command in (['sh', '-c', guard], ['sh', str(script), str(sdk), str(output)]):
                result = subprocess.run(command, env={**clean, 'PIONEER_Q38_RESIDENT_DECODER_OWNER': value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
                count += 1
    native = (headers / 'tenstorrent_pci_probe.cpp').read_text()
    guard = re.findall(r'#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER\n#ifndef PIONEER_Q38_RESIDENT_DECODER_NATIVE\n#error [^\n]+\n#endif\n#endif', native)
    assert len(guard) == 1
    for value in ('0', '1', '2', '-1'):
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', '-DPIONEER_Q38_RESIDENT_DECODER_OWNER=' + value, '-'],
                                input=guard[0], env=clean, capture_output=True, text=True)
        assert result.returncode != 0 and 'Resident decoder arithmetic owner is OFFLINE' in result.stderr
        count += 1
    for value in (None, '0', '1', '2', '-1'):
        args = [*flags, *([] if value is None else ['-DPIONEER_Q38_RESIDENT_DECODER_OWNER=' + value])]
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-x', 'c++', '-fsyntax-only', *args, '-'],
                                input='#include "' + str(headers / 'PioneerQ38ResidentDecoderOwner.h') + '"\n', env=clean, capture_output=True, text=True)
        assert (result.returncode == 0) == (value == '1')
        count += 1
    for flag in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE'):
        for value in ('0', '1'):
            result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-x', 'c++', '-fsyntax-only', *flags,
                                     '-DPIONEER_Q38_RESIDENT_DECODER_OWNER=1', '-D' + flag + '=' + value, '-'],
                                    input='#include "' + str(headers / 'PioneerQ38ResidentDecoderOwner.h') + '"\n', env=clean, capture_output=True, text=True)
            assert result.returncode != 0
            count += 1
print(f'PASS resident decoder gates{count},compiled QCB1 ROM ALL1523words EXACT/all120 existing snapshots/8KiB stacks/table bounds,actual builder earlyrefusals/unqualified native HARD refusal/exact header role/no DEBUG or Work promotion;qualified native role tested separately/NO install/device access')
