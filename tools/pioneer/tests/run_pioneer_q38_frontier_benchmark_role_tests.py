#!/usr/bin/env python3
"""Physical/USER/old endpoint separation only; no accelerator IO."""
import os
import pathlib
import re
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
parent=(kernel/'PioneerQ38TokenHiddenValidationRole.h').read_text()
pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
required=set(re.findall(pattern,parent))|{
    'PIONEER_Q38_TOKEN_FAULT_PROGRAMS','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
    'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS','PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE',
    'PIONEER_Q38_TOKEN_POSTED_CANCEL','PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS',
    'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE',
    'PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS','PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT',
    'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE'}
physical=kernel/'PioneerQ38TokenFrontierBenchmarkRole.h';data=here.parent/'PioneerQ38TokenFrontierProgramsRole.h'
client=here.parent/'PioneerQ38TokenFrontierBenchmarkClientRole.h';session=here.parent/'PioneerQ38TokenHiddenValidationSession.h'
positive=negative=0
def include(path):return '#include "'+str(path)+'"\n'
def check(values,header,okay,tail=''):
    global positive,negative
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))+include(header)+tail
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],
                          input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,header,tail,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr;negative+=1
base={**dict.fromkeys(required,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x2026103100010075ULL'}
check(base,physical,True,include(physical))
check(base,data,True,include(data)+'static_assert(PioneerQ38TokenFrontierProgramSelection==3);\n')
for key in sorted(required):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,data,False)
    check(base,data,False,'#undef '+key+'\n#define '+key+' 0\n'+include(data))
for key in ('PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
            'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT','PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT',
            'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
            'TT_PIONEER_Q38_WORK_MODE','api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for value in ('0','1'):check({**base,key:value},data,False)
    check(base,data,False,'#define '+key+' 0\n'+include(data))
for tag in (None,'0','0x5154334e4f445256ULL','0x2026102100010012ULL','0x2026103000010074ULL'):
    bad=base.copy()
    if tag is None:del bad['PIONEER_DRIVER_BUILD_TAG']
    else:bad['PIONEER_DRIVER_BUILD_TAG']=tag
    check(bad,data,False)
user=dict.fromkeys(('PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT','PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT',
                   'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT'),'1')
check(user,client,True,include(client))
for key in user:
    for value in (None,'0','2'):
        bad=user.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        check(bad,client,False)
for key in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT',
            'PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS','PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE',
            'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_WORK_INFERENCE',
            'TT_PIONEER_Q38_WORK_MODE','__FAST_MATH__'):
    for value in ('0','1'):check({**user,key:value},client,False)
check({'B_DEVICE_OP_CODES_END':'1000',**user},session,True,include(session))
check({'B_DEVICE_OP_CODES_END':'1000',**user},session,False,'#undef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT\n'+include(session))
old={key:value for key,value in user.items() if key!='PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT'}
check({'B_DEVICE_OP_CODES_END':'1000',**old},session,False,'#define PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT 1\n'+include(session))
# Execute the actual small dispatch block with MOCK bodies;do not manufacture
# a second hand-written route table and assume it represents ProbeControl.
source=(kernel/'tenstorrent_pci_probe.cpp').read_text()
begin=source.index('#ifdef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE\n#ifdef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE\n')
end=source.index(' if(operation==TT_PCI_PROBE_RELOAD_STATUS',begin)
block=source[begin:end]
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontier-route-',dir='/private/tmp') as name:
    out=pathlib.Path(name);body=out/'actual_dispatch.inc';body.write_text(block)
    common='#define B_DEVICE_OP_CODES_END 1000\n'+include(kernel/'PioneerQ38TokenFrontierBenchmarkProtocol.h')+'''
static int Q38TokenNativeControl(void*,void*,unsigned){return 1;}
static int Q38TokenFrontierControl(void*,void*,unsigned){return 2;}
static int Q38TokenHiddenValidationControl(void*,void*,unsigned){return 3;}
static int Route(unsigned operation){void* instance=nullptr;void* data=nullptr;unsigned length=0;
'''+include(body)+'return -1;}\n'
    for mode in ('old-hidden','old-decode','new-frontier'):
        defines='#define PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE 1\n'
        if mode!='old-hidden':defines+='#define PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE 1\n'
        if mode=='new-frontier':defines+='#define PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE 1\n'
        expected=[(code,1 if code==(0x6607 if mode=='new-frontier' else 0x6606 if mode=='old-decode' else 0x6604)
                   else 2 if mode=='new-frontier' and code==0x6608 else 3 if mode=='old-hidden' and code==0x6605 else -1)
                  for code in (0x6604,0x6605,0x6606,0x6607,0x6608)]
        program=defines+common+'#include <cassert>\nint main(){'+''.join(f'assert(Route(1000+{code})=={value});' for code,value in expected)+'}\n'
        binary=out/mode
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wno-unused-function','-x','c++','-','-o',str(binary)],
                       input=program,text=True,check=True)
        subprocess.run([str(binary)],check=True)
print(f'PASS new physical+USER counter benchmark roles/{positive} syntax positives/{negative} missing+zero+two+late+NODRV+ABI+CPU+Work refusals/actual dispatch block15 route outcomes with MOCK bodies;NO native/physical/model/timing/Work permission')
