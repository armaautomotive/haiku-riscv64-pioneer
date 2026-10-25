#!/usr/bin/env python3
"""Exact DEBUG64 native/client role separation; closed existing protocol."""
import os
import hashlib
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
native, client = 'PIONEER_Q38_CONV_SILU_PRECISION_NATIVE', 'PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT'
parents = ('PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE',
           'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER',
           'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_DECODER_RUNTIME',
           'PIONEER_Q38_PROJECTION_EXECUTION_OWNER', 'PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_LEARNED_RMS_NATIVE',
           'PIONEER_Q38_EMBEDDING_NATIVE', 'TT_PIONEER_DRIVER_RELOAD')
valid = dict.fromkeys((*parents, native), '1');valid['PIONEER_DRIVER_BUILD_TAG'] = '0x2026102400010064ULL'
observer = dict(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1', PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
count = 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(prefix, {}, True)
check(prefix, valid, True)
old = {k: v for k, v in valid.items() if k != native};old['PIONEER_DRIVER_BUILD_TAG'] = '0x2026102400010052ULL'
check(prefix, old, True)
for flag in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[flag]
        else:
            flags[flag] = value
        check(prefix, flags, False)
for flag in (client, 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_SILU_PRECISION_IMAGE', 'PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE',
             'PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE', 'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE', 'PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE',
             'PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE'):
    for value in ('0', '1'):
        check(prefix, {**valid, flag: value}, False)
for tag in ('0x2026102100010012ULL', '0x2026102200010038ULL', '0x2026102300010043ULL', '0x2026102400010051ULL',
            '0x2026102400010052ULL', '0x2026102400010053ULL', '0x2026102400010055ULL', '0x2026102400010063ULL'):
    check(prefix, {**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
check(prefix, {'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for value in ('0', '1'):
    check(prefix, {**old, native: value}, False)
    check(prefix, {**old, client: value}, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent', 'GdnRendezvous', 'GdnOutput', 'NextProjection', 'NextGdnOutput'):
    kind = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    cpu = {'PIONEER_Q38_'+kind+'_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE': '1'}
    if name == 'NextGdnOutput':
        cpu = dict(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE='1', PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1')
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text, cpu, True)
    for value in ('0', '1'):
        check(text, {**cpu, native: value}, False)
        check(text+'#define '+native+' '+value+'\n'+text, cpu, False)
session = '#include "'+str(directory/'PioneerQ38GdnRendezvousValidationSession.h')+'"\n'
check(session, observer, True)
check(session, {**observer, client: '1'}, True)
for flag in (*observer, client):
    for value in (None, '0', '2', '-1'):
        flags = {**observer, client: '1'}
        if value is None:
            del flags[flag]
        else:
            flags[flag] = value
        check(session, flags, flag == client and value is None)  # absent variant is the unchanged DEBUG52 observer
for flag in (native, 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_WORK_INFERENCE', '_KERNEL_MODE', 'PIONEER_Q38_SILU_PRECISION_IMAGE'):
    for value in ('0', '1'):
        check(session, {**observer, client: '1', flag: value}, False)
check(session+'#define '+client+' 1\n'+session, observer, False)
check(session+'#undef '+client+'\n'+session, {**observer, client: '1'}, False)
image = '#include "'+str(kernel/'PioneerQ38ConvBatchImage.h')+'"\n'
check(image, {}, True)
check(image, valid, True)
check(image+'#define '+native+' 1\n'+image, {k: v for k, v in valid.items() if k != native}, False)
check(image+'#undef '+native+'\n'+image, valid, False)
with tempfile.TemporaryDirectory(prefix='pioneer-debug64-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    # Execute the actual native selector, not only a preprocessor role test.
    # Both variants must export every frozen instruction/address, including
    # their boot word, without changing the initial projection-image ledger.
    for precise, flags, size, digest in (
        (False, old, 6088, 'a8df2da24fa98535c70172c6f9833228c996a540f487b66fa545660b93256350'),
        (True, valid, 7560, '0a009317f69ecba0d6f56575508d4d4e6bdb3c30ece3e1f78a85cd66e2ab644c'),
    ):
        text = '#include <cassert>\n#include <cstdio>\n'+image
        text += 'int main(){static_assert(TT_Q38_CONV_BATCH_TEXT_BYTES=='+str(size)+');\n'
        text += 'static_assert(TT_Q38_CONV_BATCH_IMAGE_WORDS=='+str(size//4+1)+');\n'
        text += 'assert(PioneerQ38ConvBatchImageAddress(0)==0x6000 && PioneerQ38ConvBatchImageWord(0)==0x0005e06fU);\n'
        text += 'for(unsigned i=1;i<TT_Q38_CONV_BATCH_IMAGE_WORDS;i++){auto w=PioneerQ38ConvBatchImageWord(i);\n'
        text += 'assert(PioneerQ38ConvBatchImageAddress(i)==0x64000+(i-1)*4 && w==PioneerQ38ConvBatchCodeWord(i-1));\n'
        if precise:
            text += 'assert(w==PioneerQ38ConvBatchSiluPrecisionImageWord(i));\n'
        text += 'assert(fwrite(&w,4,1,stdout)==1);}\n'
        text += 'assert(PioneerQ38ConvBatchImageAddress(TT_Q38_CONV_BATCH_IMAGE_WORDS)==UINT32_MAX);\n'
        text += 'assert(PioneerQ38ConvBatchImageWord(TT_Q38_CONV_BATCH_IMAGE_WORDS)==UINT32_MAX);\n'
        text += 'assert(PioneerQ38ConvBatchTerminalPc(0x64038) && !PioneerQ38ConvBatchTerminalPc(0x6403c));}\n'
        path, binary = root/('image-'+str(precise)+'.cpp'), root/('image-'+str(precise))
        path.write_text(text)
        subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', *['-D'+k+'='+v for k,v in flags.items()], str(path), '-o', str(binary)], check=True, env=clean)
        code = subprocess.check_output([str(binary)], env=clean)
        assert len(code)==size and hashlib.sha256(code).hexdigest()==digest
        count += 1
    for role in (native, client):
        for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image', 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
            argv = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name == 'inference' else []), str(root/'NO_OUTPUT')]
            for value in ('1', '2', '-1', '', 'false', '0\n1'):
                result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv], capture_output=True, text=True, env={**clean, role: value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count += 1
    for name in ('build_qwen38_conv_silu_precision_validation_native.py', 'build_qwen38_conv_silu_precision_validation_client.sh'):
        for role in (native, client, 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '', 'false'):
                result = subprocess.run(['python3' if name.endswith('.py') else 'sh', str(directory/name), str(root/'NO_SDK'), str(root/'NO_OUTPUT')],
                                        capture_output=True, text=True, env={**clean, role: value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count += 1
    route = source.split('// PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_ROUTING_BEGIN', 1)[1].split('// PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_ROUTING_END', 1)[0]
    code = '#include <cassert>\n#include <cstddef>\nusing uint32=unsigned;using status_t=int;enum{B_DEVICE_OP_CODES_END=1000,B_NOT_ALLOWED=-1};\n'
    for name in ('PioneerQ38GdnRendezvousValidationProtocol.h', 'PioneerQ38SpanProtocol.h'):
        code += '#include "'+str(kernel/name)+'"\n'
    code += 'unsigned calls=0;int Q38GdnRendezvousValidationControl(void*,unsigned,void*,size_t){calls++;return 7;}\n'
    code += 'int Route(unsigned operation){void*instance=nullptr;void*data=nullptr;size_t length=0;\n'+route+'\nreturn 8;}\n'
    code += 'int main(){for(unsigned op=0;op<131072;op++){int r=Route(op);bool compute=op==TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATION_SUPPORT||op==TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE;\n'
    code += 'bool storage=op==TT_PCI_PROBE_Q38_BANK_SUPPORT||op==TT_PCI_PROBE_Q38_BANK_BEGIN||op==TT_PCI_PROBE_Q38_BANK_END||op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT||op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD||op==TT_PCI_PROBE_Q38_SPAN_END;assert(r==(compute?7:storage?8:B_NOT_ALLOWED));}assert(calls==2);}\n'
    path, binary = root/'routes.cpp', root/'routes';path.write_text(code)
    subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print(f'PASS DEBUG64 exact role/tag/ALL native52 parents/client64 separation/10CPU observers+reinclude/image binding/builder gates{count}/131072 ACTUAL closed route probes;only2DEBUG+6storage/NO Work or speed grant')
