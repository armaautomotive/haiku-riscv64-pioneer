#!/usr/bin/env python3
"""Exact refusing whole-context witness gates;NO driver/device execution."""
import ast
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
header = '#include "'+str(kernel/'PioneerQ38ResidentOutputContextCompileRole.h')+'"\n'
flags = dict(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY='1',PIONEER_Q38_RESIDENT_OUTPUT_OWNER='1',
             PIONEER_Q38_PROJECTION_EXECUTION_OWNER='1',PIONEER_Q38_PROJECTION_RESULT_STASH='1',
             PIONEER_Q38_LEARNED_RMS_NATIVE='1',PIONEER_Q38_EMBEDDING_NATIVE='1',
             PIONEER_Q38_DECODER_RUNTIME='1',PIONEER_Q38_GDN_MUTABLE_STATE='',TT_PIONEER_DRIVER_RELOAD='1',
             PIONEER_DRIVER_BUILD_TAG='0x2026102500010067ULL',PIONEER_Q38_FFN_NATIVE_GRAPH='1',
             PIONEER_Q38_FFN_PERSISTENT_GRAPH='1',PIONEER_Q38_FFN_GRAPH_INPUT_ROOT='1',PIONEER_Q38_FFN_MODEL_INIT='1',
             PIONEER_Q38_NORMAL_CLOCK='1',TT_PIONEER_EXPERIMENTAL_Q38_BANKS='1',
             TT_PIONEER_EXPERIMENTAL_Q38_STREAM='1',TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR='1')
clean = {k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0


def check(source,roles,expected):
    global count
    result = subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-x','c++','-I'+str(kernel),
                             *['-D'+k+'='+v for k,v in roles.items()],'-'],input=source,text=True,
                            capture_output=True,env=clean,timeout=10)
    assert (result.returncode==0)==expected,(roles,result.stderr)
    count += 1


check(header,flags,True)
check(header+header,flags,True)
for value in ('0','1'):
    check(header,{**flags,'__FAST_MATH__':value},False)
    check(header+'#define __FAST_MATH__ '+value+'\n'+header,flags,False)
for key in flags:
    missing = dict(flags);del missing[key];check(header,missing,False)
    check(header+'#undef '+key+'\n'+header,flags,False)
    if key!='PIONEER_Q38_GDN_MUTABLE_STATE':
        for value in ('','0','2','-1'):
            check(header,{**flags,key:value},False)
            check(header+'#undef '+key+'\n#define '+key+' '+value+'\n'+header,flags,False)
for tag in ('0x2026102200010036ULL','0x2026102300010043ULL','0x2026102500010065ULL','0x2026102500010066ULL'):
    check(header,{**flags,'PIONEER_DRIVER_BUILD_TAG':tag},False)
role_text = (kernel/'PioneerQ38ResidentOutputContextCompileRole.h').read_text()+(kernel/'PioneerQ38ResidentOutputOwnerRole.h').read_text()
for key in sorted(set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)',role_text))-set(flags)):
    for value in ('0','1'):
        check(header,{**flags,key:value},False)
        check(header+'#define '+key+' '+value+'\n'+header,flags,False)
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE',1)[0].replace('#include "tenstorrent_pci_probe.h"','')
check(prefix,{},True)
check(prefix,flags,True)
for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for value in ('0','unexpected_alias'):
        check(prefix,{**flags,key:value},False)
for value in ('','0','1','2','-1'):
    check(prefix,{'PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY':value},False)
check(prefix,{'TT_PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':flags['PIONEER_DRIVER_BUILD_TAG']},False)
for value in ('0','1'):
    check(prefix,{'PIONEER_Q38_RESIDENT_OUTPUT_OWNER':value},False)
with tempfile.TemporaryDirectory(prefix='pioneer-output-context-build-gates-',dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for name in ('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image',
                 'ffn_persistent_image','projection_graph_image','projection_matrix_image','projection_persistent_image'):
        for value in ('0','1','2','-1','','false','0\n1'):
            args = [str(root/'NO_SDK'),*([str(root/'NO_LIB')] if name=='inference' else []),str(root/'NO_OUTPUT')]
            result = subprocess.run(['sh',str(here.parent/('build_qwen38_'+name+'.sh')),*args],capture_output=True,text=True,
                                    env={**clean,'PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY':value},timeout=5)
            assert result.returncode==2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1

# Execute the ACTUAL byte-audit helper without importing/running the builder.
# A different ELF section with the same address or incomplete coverage must
# never qualify an entry. This test does not run any RISC-V/kernel/card code.
builder = ast.parse((here/'run_pioneer_q38_resident_output_native_context_compile.py').read_text())
functions = [n for n in builder.body if isinstance(n,ast.FunctionDef) and n.name=='code'];assert len(functions)==1
scope = dict(defined={'entry':(0x100,6,'T')},sections={'entry':'.text'},instructions={0x100:'4501',0x102:'00008067'},relocations=set())
exec(compile(ast.Module(body=functions,type_ignores=[]),'<actual-native-context-byte-audit>','exec'),scope)
assert scope['code']('entry')==['4501','00008067']
for sections,instructions in (({'entry':'.text.other'},{0x100:'4501',0x102:'00008067'}),
                              ({'entry':'.text'},{0x100:'4501'}),
                              ({'entry':'.text'},{0x100:'4501',0x104:'8082'})):
    scope['sections']=sections;scope['instructions']=instructions
    try:
        scope['code']('entry')
    except AssertionError:
        count += 1
    else:
        raise AssertionError('wrong section/missing byte audit was accepted')
scope.update(sections={'entry':'.text'},instructions={0x100:'4501',0x102:'00008067'},relocations={0x102})
try:
    scope['code']('entry')
except AssertionError:
    count += 1
else:
    raise AssertionError('linker rewrite inside an inert entry was accepted')
print('PASS whole-context refusing COMPILE role gates'+str(count)+'/exact tag+parents/old-native+CPU+DEBUG+Work+image aliases/late changes/entry-name spoofing/normal owner still OFFLINE/10legacy builders/section-aware ALL-byte+relocation entry audit;NO load/link/device grant')
