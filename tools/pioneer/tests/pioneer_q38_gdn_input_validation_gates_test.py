#!/usr/bin/env python3
"""Actual DEBUG46/client boundaries,NO SDK/model/device access."""
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here / 'pioneer_q38_gdn_input_native_gates_test.py')) # unchanged45/ROM/frame controls176
directory = here.parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
native = dict(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE='1', PIONEER_Q38_GDN_INPUT_OWNER='1',
              PIONEER_Q38_RESIDENT_DECODER_NATIVE='1', PIONEER_Q38_RESIDENT_DECODER_OWNER='1', PIONEER_Q38_DECODER_RUNTIME='1',
              PIONEER_Q38_PROJECTION_EXECUTION_OWNER='1', PIONEER_Q38_PROJECTION_RESULT_STASH='1', PIONEER_Q38_LEARNED_RMS_NATIVE='1',
              PIONEER_Q38_EMBEDDING_NATIVE='1', TT_PIONEER_DRIVER_RELOAD='1', PIONEER_DRIVER_BUILD_TAG='0x2026102300010046ULL')
client = dict(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1', PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
count = 0
def preprocess(text, flags, accepted):
    global count
    r = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                       input=text, env=clean, capture_output=True, text=True, timeout=5)
    assert (r.returncode == 0) == accepted, (flags, r.stderr)
    count += 1
preprocess(prefix, native, True)
for key in native:
    for value in (None, '0', '2', '-1'):
        bad = dict(native)
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        preprocess(prefix, bad, False)
for tag in ('0x2026102100010012ULL', '0x2026102200010042ULL', '0x2026102300010043ULL', '0x2026102300010044ULL', '0x2026102300010045ULL'):
    preprocess(prefix, {**native, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
preprocess(prefix, dict(TT_PIONEER_DRIVER_RELOAD='1', PIONEER_DRIVER_BUILD_TAG=native['PIONEER_DRIVER_BUILD_TAG']), False)
for flag in ('PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE', 'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE',
             'PIONEER_Q38_EMBEDDING_QUALIFICATION', 'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        preprocess(prefix, {**native, flag: value}, False)
for name in ('PioneerQ38GdnInputValidationReference.h', 'PioneerQ38ResidentDecoderValidationReference.h',
             'PioneerQ38ProjectionValidationReference.h', 'PioneerQ38EmbeddingValidationReference.h'):
    guard = (directory / name).read_text().split('#include', 1)[0] + '#endif\n'
    preprocess(guard, client, True)
    required = tuple(client) if 'Embedding' not in name else tuple(client)[:2]
    for key in required:
        for value in (None, '0', '2', '-1'):
            bad = dict(client)
            if value is None:
                del bad[key]
            else:
                bad[key] = value
            preprocess(guard, bad, False)
    for flag in ('PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT', 'PIONEER_Q38_PROJECTION_VALIDATION_CLIENT',
                 'PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT',
                 'PIONEER_Q38_GDN_INPUT_OWNER', 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE', 'PIONEER_Q38_WORK_INFERENCE', '_KERNEL_MODE'):
        for value in ('0', '1'):
            preprocess(guard, {**client, flag: value}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-gdn-debug-gates-') as temporary:
    root = pathlib.Path(temporary)
    sdk, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    # Compile the ACTUAL new terminal-classifier body in a DATA-only probe.
    # This is NOT a fake native owner/producer/launch capability. The real
    # owner/public factories are qualified separately with emitted jobs.
    owner = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/PioneerQ38ResidentDecoderOwner.h').read_text()
    begin, end = ' template<class IO> bool GdnTerminal(IO& io,unsigned w){', ' template<class IO> bool GdnReceipt('
    assert owner.count(begin) == owner.count(end) == 1
    method = begin + owner.split(begin, 1)[1].split(end, 1)[0]
    shape = here.parents[2] / 'src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputShape.h'
    probe = '#include <cassert>\n#include <vector>\n#include "' + str(shape) + '"\n' \
            '#define PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE 1\n' \
            'struct IO{unsigned status[3]={0x6400c,64,TT_Q38_GDN_INPUT_MARKER},pc[2]={0x64038,0x6403c},at=0;std::vector<unsigned> trace;\n' \
            'unsigned RuntimeStatusRead(unsigned i){trace.push_back(i);return status[i];}\n' \
            'unsigned RuntimePcRead(){assert(at<2);trace.push_back(10+at);return pc[at++];}void Pause(){trace.push_back(99);}};\n' \
            'struct Probe{bool gdnImage[120]={};unsigned observedWorker=0,observedStatus[3]={},observedPc[2]={};\n' \
            + method + '\n};\nint main(){\n' \
            'for(unsigned a:{0x64038U,0x6403cU})for(unsigned b:{0x64038U,0x6403cU}){Probe p;IO io;p.gdnImage[17]=true;io.pc[0]=a;io.pc[1]=b;\n' \
            'assert(p.GdnTerminal(io,17)&&io.trace==std::vector<unsigned>({2,0,1,99,10,99,11}));}\n' \
            'for(unsigned field:{2U,0U,1U}){Probe p;IO io;p.gdnImage[17]=true;io.status[field]^=1;assert(!p.GdnTerminal(io,17)&&!io.at);\n' \
            'assert(io.trace==std::vector<unsigned>(field==2?std::initializer_list<unsigned>{2}:field==0?std::initializer_list<unsigned>{2,0}:std::initializer_list<unsigned>{2,0,1}));}\n' \
            'for(unsigned which:{0U,1U})for(unsigned bad:{0U,0x64008U,0x64058U,0x64039U}){Probe p;IO io;p.gdnImage[17]=true;io.pc[which]=bad;\n' \
            'assert(!p.GdnTerminal(io,17)&&io.at==which+1&&io.trace.size()==3+2*(which+1));}\n' \
            'for(unsigned w:{0U,17U,97U,120U}){Probe p;IO io;assert(!p.GdnTerminal(io,w)&&io.trace.empty());}\n' \
            '}\n'
    binary = root / 'actual-terminal-data-classifier'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-x', 'c++', '-', '-o', str(binary)],
                   input=probe, text=True, env=clean, check=True)
    subprocess.run([str(binary)], env=clean, check=True)
    count += 19
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        script = directory / ('build_qwen38_' + name + '.sh')
        guards = [line for line in script.read_text().splitlines() if line.startswith('[ ') and 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE' in line]
        assert len(guards) == 1
        for value in (None, '0'):
            env = clean if value is None else {**clean, 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE': value}
            subprocess.run(['sh', '-c', 'set -eu\n' + guards[0]], env=env, check=True)
            count += 1
        for value in ('1', '2', '-1', '01', '', 'false', '0\n1'):
            r = subprocess.run(['sh', str(script), str(sdk), str(output)], env={**clean, 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE': value},
                               capture_output=True, text=True, timeout=5)
            assert r.returncode == 2 and not r.stdout and not r.stderr and not output.exists()
            count += 1
    for script in (directory / 'build_qwen38_gdn_input_validation_native.py', directory / 'build_qwen38_gdn_input_validation_client.sh'):
        for role in ('PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT', 'PIONEER_Q38_WORK_INFERENCE'):
            for value in ('1', '2', '-1', '', '00', 'false'):
                command = [sys.executable if script.suffix == '.py' else 'sh', str(script), str(sdk), str(output)]
                r = subprocess.run(command, env={**clean, role: value}, capture_output=True, text=True, timeout=5)
                assert r.returncode == 2 and not r.stdout and not r.stderr and not output.exists()
                count += 1
assert re.search(r'#if defined\(PIONEER_Q38_GDN_INPUT_OWNER\) && !defined\(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE\)', source)
print(f'PASS DEBUG46 gates{count}+old45gates176/exact native-client roles/old namespaces cannot alias/mixed Work and CPU-reference native denied/actual terminal classifier strict read order-before-PC/both post-success PCs/legacy-new builders refuse ambient flags BEFORE SDK/output;NO physical access')
