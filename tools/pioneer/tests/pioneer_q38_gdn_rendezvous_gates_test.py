#!/usr/bin/env python3
"""Exact native51/offline-image separation and actual CSR-opcode gates. NO device."""
import os
import pathlib
import re
import shlex
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
driver = directory.parents[1] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
source = driver.read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
count = 0

def preprocess(text, flags, accepted):
    global count
    result = subprocess.run([cxx, '-E', '-x', 'c++', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, capture_output=True, text=True, env=clean, timeout=5)
    assert (result.returncode == 0) == accepted, (flags, result.stderr)
    count += 1

valid = dict.fromkeys(('PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE',
    'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER',
    'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_DECODER_RUNTIME',
    'PIONEER_Q38_PROJECTION_EXECUTION_OWNER', 'PIONEER_Q38_PROJECTION_RESULT_STASH',
    'PIONEER_Q38_LEARNED_RMS_NATIVE', 'PIONEER_Q38_EMBEDDING_NATIVE', 'TT_PIONEER_DRIVER_RELOAD'), '1')
valid['PIONEER_DRIVER_BUILD_TAG'] = '0x2026102400010051ULL'
preprocess(prefix, {}, True)
preprocess(prefix, valid, True)
for key in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[key]
        else:
            flags[key] = value
        preprocess(prefix, flags, False)
for tag in ('0x2026102200010038ULL', '0x2026102200010042ULL', '0x2026102300010043ULL',
            '0x2026102300010044ULL', '0x2026102300010045ULL', '0x2026102300010046ULL',
            '0x2026102400010047ULL', '0x2026102400010048ULL', '0x2026102400010049ULL', '0x2026102400010050ULL'):
    preprocess(prefix, {**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
preprocess(prefix, {'TT_PIONEER_DRIVER_RELOAD':'1', 'PIONEER_DRIVER_BUILD_TAG':valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for mixed in ('PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE',
              'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE', 'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE',
              'PIONEER_Q38_EMBEDDING_QUALIFICATION', 'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE',
              'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE', 'PIONEER_Q38_CPU_VALIDATION',
              'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        preprocess(prefix, {**valid, mixed:value}, False)

image_source = '#include "'+str(directory/'blackhole_qwen38_gdn_gated_resident.cpp')+'"\n'
image = {'PIONEER_Q38_GDN_GATED_RESIDENT_IMAGE':'1', 'PIONEER_Q38_GDN_RENDEZVOUS_IMAGE':'1'}
preprocess(image_source, image, True)
for key in image:
    for value in ('0', '2', '-1'):
        preprocess(image_source, {**image, key:value}, False)
preprocess(image_source, {'PIONEER_Q38_GDN_RENDEZVOUS_IMAGE':'1'}, False)
for mixed in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION',
              'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE'):
    for value in ('0', '1'):
        preprocess(image_source, {**image, mixed:value}, False)

with tempfile.TemporaryDirectory(prefix='pioneer-qgt1-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root/'MUST_NOT_READ_SDK', root/'MUST_NOT_CREATE_OUTPUT'
    for name in ('PioneerQ38EmbeddingValidationReference.h', 'PioneerQ38LearnedRmsValidationReference.h',
                 'PioneerQ38ProjectionValidationReference.h', 'PioneerQ38ResidentDecoderValidationReference.h',
                 'PioneerQ38GdnInputValidationReference.h', 'PioneerQ38GdnRecurrentValidationReference.h'):
        client = 'PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT' if 'GdnRecurrent' in name else 'PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT'
        flags = {client:'1', 'PIONEER_Q38_CPU_VALIDATION':'1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE':'1'}
        text = '#include "'+str(directory/name)+'"\n'
        preprocess(text, flags, True)
        for value in ('0', '1', '2', '-1'):
            preprocess(text, {**flags, 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE':value}, False)
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), str(sdk), str(output)],
                                    capture_output=True, text=True, env={**clean, 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE':value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
            count += 1
    for name, runner in (('build_qwen38_gdn_rendezvous_native.py', sys.executable),
                         ('build_qwen38_gdn_rendezvous_image.sh', 'sh')):
        for key in ('PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_IMAGE',
                    'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '', 'false', '00'):
                result = subprocess.run([runner, str(directory/name), str(sdk), str(output)],
                                        capture_output=True, text=True, env={**clean, key:value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
                count += 1
    commands = [line for line in (directory/'build_qwen38_gdn_rendezvous_image.sh').read_text().splitlines() if line.startswith('awk ')]
    assert len(commands) == 2
    for instruction in ('div', 'divu', 'rem', 'remu', 'mulh', 'mulhu', 'mulhsu', 'fdiv.s', 'fsqrt.s',
                        'fmadd.s', 'fmsub.s', 'fnmadd.s', 'fnmsub.s', 'fcvt.w.s', 'fcvt.wu.s', 'mul', 'fadd.s', 'fmul.s'):
        listing = root/'instructions'
        listing.write_text('00064000: 00000000 '+instruction+' a0,a1,a2\n')
        result = subprocess.run(['sh', '-c', commands[0].replace('"$output/program.disassembly"', shlex.quote(str(listing)))], capture_output=True, env=clean)
        assert (result.returncode == 0) == (instruction in ('mul', 'fadd.s', 'fmul.s'))
        count += 1
    for frame in (16, 896, 1024, 1025):
        usage = root/'usage.su'
        usage.write_text(f'actual:1:1:frame\t{frame}\tstatic\n')
        result = subprocess.run(['sh', '-c', commands[1].replace('"$output/"*.su', shlex.quote(str(usage)))], capture_output=True, env=clean)
        assert (result.returncode == 0) == (frame <= 1024)
        count += 1
    # Exact actual CSR encodings; mnemonic text deliberately stays "csrrs"
    # for all corruptions so the opcode audit cannot rely on disassembler aliases.
    good = [(csr<<20) | (2<<12) | (10<<7) | 0x73 for csr in (0xb80, 0xb00, 0xb80)]
    cases = [(good, True), (good[:2], False), (good+good[:1], False)]
    for change in (lambda v:(v & ~(7<<12)) | (1<<12), lambda v:v | (1<<15),
                   lambda v:(v & ~(0xfff<<20)) | (0x320<<20), lambda v:v & ~(31<<7)):
        for i in range(3):
            words = list(good); words[i] = change(words[i]); cases.append((words, False))
    cases.append((good+[0x00000073], False))
    for words, accepted in cases:
        listing = root/'csr-disassembly'
        listing.write_text(''.join(f' {0x65000+i*4:x}: {word:08x} csrrs a0,mcycle,x0\n' for i, word in enumerate(words)))
        result = subprocess.run([sys.executable, str(here/'pioneer_q38_gdn_rendezvous_csr_gate.py'), str(listing)], capture_output=True, env=clean)
        assert (result.returncode == 0) == accepted, words
        count += 1
    begin, end = '// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_BEGIN', '// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_END'
    assert source.count(begin) == source.count(end) == 1
    route = source.split(begin,1)[1].split(end,1)[0]
    code = '#include <cassert>\nusing status_t=int;constexpr int B_NOT_ALLOWED=-1;int touches=0;\n'
    code += 'int Route(unsigned operation){(void)operation;\n'+route+'\n touches++;return 8;}\n'
    code += 'int main(){for(unsigned op=0;op<131072;op++)assert(Route(op)==B_NOT_ALLOWED);assert(!touches);}\n'
    path, binary = root/'routes.cpp', root/'routes'
    path.write_text(code)
    subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(path), '-o', str(binary)], check=True, env=clean)
    subprocess.run([str(binary)], check=True, env=clean)
print(f'PASS QGT1/native51 gates{count}/131072 ACTUAL inert routes/exact parents+tag/no mixed reference-Work/early builder refusals/unchanged1024frame/actual CSR opcode mutations;NO linked driver/device/timing or speed claim')
