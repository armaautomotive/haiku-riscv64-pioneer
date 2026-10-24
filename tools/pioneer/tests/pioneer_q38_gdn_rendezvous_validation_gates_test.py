#!/usr/bin/env python3
"""Exact DEBUG52 role/tag,closed public allowlist,old-builder refusals."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
valid = dict.fromkeys(('PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_NATIVE',
    'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER', 'PIONEER_Q38_RESIDENT_DECODER_NATIVE',
    'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_DECODER_RUNTIME', 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER',
    'PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_LEARNED_RMS_NATIVE', 'PIONEER_Q38_EMBEDDING_NATIVE', 'TT_PIONEER_DRIVER_RELOAD'), '1')
valid['PIONEER_DRIVER_BUILD_TAG'] = '0x2026102400010052ULL'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
def preprocess(flags, expected):
    global count
    r = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                       input=prefix, capture_output=True, text=True, env=clean)
    assert (r.returncode == 0) == expected, (flags, r.stderr)
    count += 1
preprocess({}, True)
preprocess(valid, True)
for key in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[key]
        else:
            flags[key] = value
        preprocess(flags, False)
for tag in ('0x2026102200010038ULL', '0x2026102200010042ULL', '0x2026102300010043ULL',
            '0x2026102300010044ULL', '0x2026102300010045ULL', '0x2026102300010046ULL',
            '0x2026102400010047ULL', '0x2026102400010048ULL', '0x2026102400010049ULL', '0x2026102400010050ULL', '0x2026102400010051ULL'):
    preprocess({**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
preprocess({'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for flag in ('PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE',
             'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 'PIONEER_Q38_EMBEDDING_QUALIFICATION',
             'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        preprocess({**valid, flag: value}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-qgt-debug-gates-') as directory:
    base = pathlib.Path(directory)
    role = 'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE'
    for name in ('PioneerQ38GdnInputValidationReference.h', 'PioneerQ38ResidentDecoderValidationReference.h',
                 'PioneerQ38EmbeddingValidationReference.h', 'PioneerQ38LearnedRmsValidationReference.h', 'PioneerQ38ProjectionValidationReference.h', 'PioneerQ38GdnRecurrentValidationReference.h'):
        text = '#include "' + str(here.parent / name) + '"\n'
        client = ['-D'+('PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT' if name=='PioneerQ38GdnRecurrentValidationReference.h' else 'PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT')+'=1', '-DPIONEER_Q38_CPU_VALIDATION=1', '-DPIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE=1']
        for value in (None, '0', '1'):
            flags = [] if value is None else ['-D' + role + '=' + value]
            r = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-x', 'c++', '-E', *client, *flags, '-'],
                               input=text, capture_output=True, text=True, env=clean)
            assert (r.returncode == 0) == (value is None), (name, value, r.stderr)
            count += 1
    client = dict(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1',
                  PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
    def client_header(flags, expected):
        global count
        path = here.parent / 'PioneerQ38GdnRendezvousValidationReference.h'
        r = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-x', 'c++', '-E',
                            *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                           input='#include "' + str(path) + '"\n', capture_output=True, text=True, env=clean)
        assert (r.returncode == 0) == expected, (flags, r.stderr)
        count += 1
    client_header(client, True)
    for key in client:
        for value in (None, '0', '2'):
            flags = dict(client)
            if value is None:
                del flags[key]
            else:
                flags[key] = value
            client_header(flags, False)
    for key in ('PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT', 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE', 'PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT',
                'PIONEER_Q38_PROJECTION_VALIDATION_CLIENT', 'PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT',
                'PIONEER_Q38_GDN_INPUT_OWNER', 'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_NATIVE',
                'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_WORK_INFERENCE', '_KERNEL_MODE'):
        for value in ('0', '1'):
            client_header({**client, key: value}, False)
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        script = here.parent / ('build_qwen38_' + name + '.sh')
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            r = subprocess.run(['sh', str(script), str(base / 'MUST_NOT_READ_SDK'), str(base / 'MUST_NOT_CREATE_OUTPUT')],
                               capture_output=True, text=True, env={**clean, role: value}, timeout=5)
            assert r.returncode == 2 and not r.stdout and not r.stderr and not (base / 'MUST_NOT_CREATE_OUTPUT').exists()
            count += 1
    for name in ('build_qwen38_gdn_rendezvous_validation_native.py', 'build_qwen38_gdn_rendezvous_validation_client.sh'):
        for key in ('PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT', 'PIONEER_Q38_WORK_INFERENCE'):
            for value in ('1', '2', '', 'false'):
                r = subprocess.run(['python3' if name.endswith('.py') else 'sh', str(here.parent / name),
                                    str(base / 'MUST_NOT_READ_SDK'), str(base / 'MUST_NOT_CREATE_OUTPUT')],
                                   capture_output=True, text=True, env={**clean, key: value}, timeout=5)
                assert r.returncode == 2 and not r.stdout and not r.stderr and not (base / 'MUST_NOT_CREATE_OUTPUT').exists()
                count += 1
    begin, end = '// PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_ROUTING_BEGIN', '// PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_ROUTING_END'
    assert source.count(begin) == source.count(end) == 1
    route = source.split(begin, 1)[1].split(end, 1)[0]
    # Compile the ACTUAL allowlist,not a separately reimplemented predicate.
    code = '#include <cassert>\n#include <cstddef>\nusing uint32=unsigned;using status_t=int;enum{B_DEVICE_OP_CODES_END=1000,B_NOT_ALLOWED=-1};\n'
    for name in ('PioneerQ38GdnRendezvousValidationProtocol.h', 'PioneerQ38SpanProtocol.h'):
        path = here.parents[2] / 'src/add-ons/kernel/drivers/misc' / name
        code += '#include "' + str(path) + '"\n'
    code += 'int calls=0;int Q38GdnRendezvousValidationControl(void*,unsigned,void*,size_t){calls++;return 7;}\n'
    code += 'int Route(unsigned operation){void* instance=nullptr;void* data=nullptr;size_t length=0;\n' + route + '\nreturn 8;}\n'
    code += 'int main(){for(unsigned op=0;op<131072;op++){int r=Route(op);\n'
    code += 'bool compute=op==TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATION_SUPPORT||op==TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE;\n'
    code += 'bool storage=op==TT_PCI_PROBE_Q38_BANK_SUPPORT||op==TT_PCI_PROBE_Q38_BANK_BEGIN||op==TT_PCI_PROBE_Q38_BANK_END||op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT||op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD||op==TT_PCI_PROBE_Q38_SPAN_END;\n'
    code += 'assert(r==(compute?7:storage?8:B_NOT_ALLOWED));}assert(calls==2);}\n'
    path, binary = base / 'routes.cpp', base / 'routes'
    path.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', str(path), '-o', str(binary)], check=True, env=clean)
    subprocess.run([str(binary)], check=True, env=clean)
print(f'PASS DEBUG52 exact role/tag/mixed/old-builder gates{count};131072 ACTUAL allowlist probes,only2fixedDEBUG+6storage routes,NO Work/CPU-reference/native mixing')
