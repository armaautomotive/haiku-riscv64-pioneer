#!/usr/bin/env python3
"""Native66 genuine-parent isolation and exact frozen image binding;NO device."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
# Run the complete existing native65 gate suite,then extend its actual parents.
prior = runpy.run_path(str(here/'pioneer_q38_next_ffn_residual_native_gates_test.py'))
old, prefix, source = dict(prior['valid']), prior['prefix'], prior['source']
native = 'PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE'
valid = {**old, native: '1', 'PIONEER_DRIVER_BUILD_TAG': '0x2026102500010066ULL'}
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx, count = os.environ.get('CXX', 'c++'), 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


image = '#include "'+str(kernel/'PioneerQ38ConvBatchImage.h')+'"\n'
check(prefix, valid, True)
check(image, valid, True)
for flag in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[flag]
        else:
            flags[flag] = value
        check(prefix, flags, False)
        check(image, flags, False)
for tag in ('0x2026102100010012ULL', '0x2026102400010052ULL', '0x2026102400010063ULL',
            '0x2026102400010064ULL', '0x2026102500010065ULL'):
    check(prefix, {**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
check(prefix, {'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for role in ('PIONEER_Q38_CONV_SILU_PRECISION_NATIVE', 'PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE',
             'PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE',
             'PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE', 'PIONEER_Q38_PROJECTION_NEXT_ROOT', 'PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT',
             'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        check(prefix, {**valid, role: value}, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent',
             'GdnRendezvous', 'GdnOutput', 'NextProjection', 'NextGdnOutput'):
    kind = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    cpu = {'PIONEER_Q38_'+kind+'_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE': '1'}
    if name == 'NextGdnOutput':
        cpu = dict(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE='1', PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1')
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text, cpu, True)
    for value in ('0', '1', '2', '-1'):
        check(text, {**cpu, native: value}, False)
        check(text+'#define '+native+' '+value+'\n'+text, cpu, False)
session = '#include "'+str(directory/'PioneerQ38GdnRendezvousValidationSession.h')+'"\n'
cpu = dict(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1', PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
for value in ('0', '1'):
    check(session, {**cpu, native: value}, False)
    check(session+'#define '+native+' '+value+'\n'+session, cpu, False)
check(image+'#define '+native+' 1\n'+image, old, False)
check(image+'#undef '+native+'\n'+image, valid, False)
for value in ('0', '1', '2', '-1'):
    check('#include "'+str(directory/'PioneerQ38SiluIsolation.h')+'"\n',
          {'PIONEER_Q38_SILU_PRECISION_IMAGE': '1', native: value}, False)
    result = subprocess.run([cxx, '-std=c++17', '-E', '-D'+native+'='+value,
                             str(here/'pioneer_q38_decoder_original_source.cpp')], capture_output=True, text=True, env=clean, timeout=30)
    assert result.returncode != 0 and 'Original-model observer remains separate from native66' in result.stderr
    count += 1
with tempfile.TemporaryDirectory(prefix='pioneer-native66-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    # Execute BOTH real selectors,including every instruction,boot,address,
    # exact code SHA and bounds;native65 MUST still emit the old bytes.
    for flags, size, pin in (
        (old, 6088, 'a8df2da24fa98535c70172c6f9833228c996a540f487b66fa545660b93256350'),
        (valid, 7560, '0a009317f69ecba0d6f56575508d4d4e6bdb3c30ece3e1f78a85cd66e2ab644c'),
    ):
        text = '#include <cassert>\n#include <cstdio>\n'+image
        text += 'int main(){static_assert(TT_Q38_CONV_BATCH_TEXT_BYTES=='+str(size)+');\n'
        text += 'static_assert(TT_Q38_CONV_BATCH_IMAGE_WORDS=='+str(size//4+1)+');\n'
        text += 'assert(PioneerQ38ConvBatchImageAddress(0)==0x6000 && PioneerQ38ConvBatchImageWord(0)==0x0005e06fU);\n'
        text += 'for(unsigned i=1;i<TT_Q38_CONV_BATCH_IMAGE_WORDS;i++){auto w=PioneerQ38ConvBatchImageWord(i);assert(PioneerQ38ConvBatchImageAddress(i)==0x64000+(i-1)*4 && w==PioneerQ38ConvBatchCodeWord(i-1));assert(fwrite(&w,4,1,stdout)==1);}\n'
        text += 'assert(PioneerQ38ConvBatchImageAddress(TT_Q38_CONV_BATCH_IMAGE_WORDS)==UINT32_MAX && PioneerQ38ConvBatchImageWord(TT_Q38_CONV_BATCH_IMAGE_WORDS)==UINT32_MAX);assert(PioneerQ38ConvBatchTerminalPc(0x64038) && !PioneerQ38ConvBatchTerminalPc(0x6403c));}\n'
        path, binary = root/('image'+str(size)+'.cpp'), root/('image'+str(size))
        path.write_text(text)
        subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', *['-D'+k+'='+v for k, v in flags.items()], str(path), '-o', str(binary)], check=True, env=clean)
        code = subprocess.check_output([str(binary)], env=clean)
        assert len(code) == size and hashlib.sha256(code).hexdigest() == pin
        count += 1
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            argv = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name == 'inference' else []), str(root/'NO_OUTPUT')]
            result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv],
                                    capture_output=True, text=True, env={**clean, native: value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
    for role in (native, 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run([sys.executable, str(directory/'build_qwen38_resident_silu_precision_native.py'),
                                     str(root/'NO_SDK'), str(root/'NO_OUTPUT')], capture_output=True, text=True,
                                    env={**clean, role: value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
    begin, end = '// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_BEGIN', '// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_END'
    assert source.count(begin) == source.count(end) == 1
    route = source.split(begin, 1)[1].split(end, 1)[0]
    code = '#include <cassert>\nusing status_t=int;constexpr int B_NOT_ALLOWED=-1;unsigned touches=0;\nint Route(unsigned operation){(void)operation;\n'+route+'\ntouches++;return 9;}\n'
    code += 'int main(){for(unsigned op=0;op<131072;op++)assert(Route(op)==B_NOT_ALLOWED);assert(!touches);}\n'
    path, binary = root/'routes.cpp', root/'routes';path.write_text(code)
    subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', *['-D'+k+'='+v for k, v in valid.items()], str(path), '-o', str(binary)], check=True, env=clean)
    subprocess.run([str(binary)], check=True, env=clean)
print(f'PASS native66 exact role/tag/ALL native65 parents/defined-zero+reinclude/10CPU observers/precise+legacy emitted selectors/builder gates{count}/131072 ACTUAL inert routes;NO DEBUG64/CPU products/Work/public selector/device or speed grant')
