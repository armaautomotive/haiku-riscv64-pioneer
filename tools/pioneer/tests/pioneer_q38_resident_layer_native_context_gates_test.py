#!/usr/bin/env python3
"""The actual NEW context compiles ONLY as a refusing, non-driver witness."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
valid = {'PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT':'1', 'PIONEER_Q38_RESIDENT_LAYER_OWNER':'1',
         'PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT':'1', 'PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY':'1',
         'PIONEER_Q38_RESIDENT_OUTPUT_OWNER':'1', 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER':'1',
         'PIONEER_Q38_PROJECTION_RESULT_STASH':'1', 'PIONEER_Q38_LEARNED_RMS_NATIVE':'1',
         'PIONEER_Q38_EMBEDDING_NATIVE':'1', 'PIONEER_Q38_DECODER_RUNTIME':'1', 'PIONEER_Q38_GDN_MUTABLE_STATE':'1',
         'TT_PIONEER_DRIVER_RELOAD':'1', 'PIONEER_DRIVER_BUILD_TAG':'0x51524c4e4f445256ULL',
         'PIONEER_Q38_FFN_NATIVE_GRAPH':'1', 'PIONEER_Q38_FFN_PERSISTENT_GRAPH':'1',
         'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT':'1', 'PIONEER_Q38_FFN_MODEL_INIT':'1', 'PIONEER_Q38_NORMAL_CLOCK':'1',
         'TT_PIONEER_EXPERIMENTAL_Q38_BANKS':'1', 'TT_PIONEER_EXPERIMENTAL_Q38_STREAM':'1',
         'TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR':'1'}
role = '#include "'+str(kernel/'PioneerQ38ResidentLayerContextCompileRole.h')+'"\n'
clean = {k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0


def check(text, definitions, expected):
    global count
    result = subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-x', 'c++', '-E', '-I'+str(kernel),
                             *['-D'+k+'='+v for k,v in definitions.items()], '-'], input=text, text=True,
                            capture_output=True, env=clean, timeout=30)
    assert (result.returncode == 0) == expected, (definitions, result.stderr)
    count += 1
    return result


check(role, valid, True)
check(role, {**valid, '_KERNEL_MODE':'1'}, True)
for name in valid:
    for value in (None, '', '0', '2', '-1'):
        definitions = dict(valid)
        if value is None:
            del definitions[name]
        else:
            definitions[name] = value
        check(role, definitions, False)
    check(role+'#undef '+name+'\n'+role, valid, False)
for source in ('PioneerQ38ResidentLayerContextCompileRole.h', 'PioneerQ38ResidentLayerNativeRole.h', 'PioneerQ38ResidentLayerOwnerRole.h'):
    names = set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)', (kernel/source).read_text()))-set(valid)
    for name in sorted(names):
        for value in ('0','1'):
            check(role, {**valid, name:value}, False)
            check(role+'#define '+name+' '+value+'\n'+role, valid, False)
prefix = (kernel/'tenstorrent_pci_probe.cpp').read_text().split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0]
prefix = prefix.replace('#include "tenstorrent_pci_probe.h"','')
check(prefix, {}, True)
check(prefix, valid, True)
for tag in ('0x2026102100010012ULL', '0x2026102500010065ULL', '0x2026102500010067ULL', '0x2026102500010068ULL'):
    check(prefix, {**valid, 'PIONEER_DRIVER_BUILD_TAG':tag}, False)
for name in ('PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT', 'PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY'):
    for value in ('0','1'):
        check(prefix, {name:value}, False)
        definitions = dict(valid);del definitions[name];check(prefix, definitions, False)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    check(prefix, {**valid, name:'forged_entry'}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-layer-native-context-gates-', dir='/private/tmp') as temporary:
    temp = pathlib.Path(temporary)
    for flag in ('PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT', 'PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY'):
        for value in ('0','1','2','-1','','false','0\n1'):
            for name in ('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image',
                         'ffn_persistent_image','projection_graph_image','projection_matrix_image','projection_persistent_image'):
                arguments = [str(temp/'NO_SDK'), *([str(temp/'NO_LIB')] if name=='inference' else []), str(temp/'NO_OUTPUT')]
                result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *arguments],
                                        capture_output=True, text=True, env={**clean, flag:value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (temp/'NO_OUTPUT').exists()
                count += 1
print(f'PASS full-layer native-context gates{count}:exact private producer/runtime/NODRV sentinel,all missing/defined0/reinclusion/mixed old67/68/CPU/Work/image roles refused;actual prefix mangles ABI only for exact closed role,all legacy build entries inert before output/SDK access;NO loadable driver or physical grant')
