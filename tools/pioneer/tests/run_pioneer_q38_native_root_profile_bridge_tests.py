#!/usr/bin/env python3
"""HOST observation sink/role tests only;no processor or device execution."""
import os
import json
import pathlib
import re
import subprocess
import sys
import tempfile
from pioneer_q38_native_root_profile_map import build_profile_map

assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
root=pathlib.Path(sys.argv[1])
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror']
with tempfile.TemporaryDirectory(prefix='pioneer-q38-root-profile-contract-',dir='/private/tmp') as name:
    out=pathlib.Path(name);binary=out/'test';mapping=out/'symbols.map'
    actual=out/'actual';actual.mkdir();metadata=build_profile_map(root,actual)
    assert metadata['elf_sha256']=='efeebcfc25e00b6519026428b1d7af3c12052dad6db2e6e2b52110e3fb7ed7c6'
    assert metadata['map_sha256']=='52b48cab3713398b1c7adbf125618261c36300f70ee695222aaafa04b2c412d8' and metadata['ranges']==211
    aliases=json.loads(metadata['aliases'].read_text());assert len(aliases)==211
    shared=[a for a in aliases if any('Quiet()' in n for n in a['names'])]
    assert any(len(a['names'])>=2 for a in shared),'shared-code aliases must not be discarded'
    corrupt_refused=0
    for path in root.iterdir():
        if path.name not in ('boot.bin','text.bin','terminal.bin','conv.bin','program.elf'):continue
        case=out/('bad-'+path.name);case.mkdir()
        for name in ('boot.bin','text.bin','terminal.bin','conv.bin','program.elf'):(case/name).symlink_to(root/name)
        bad=case/path.name;bad.unlink();original=path.read_bytes();changed=bytearray(original);changed[len(changed)//2]^=1
        for payload in (changed,original[:-1]):
            bad.write_bytes(payload);denied=out/f'denied-{corrupt_refused}';denied.mkdir()
            try:build_profile_map(case,denied)
            except AssertionError:pass
            else:raise AssertionError('modified ROOT accepted')
            assert not any(denied.iterdir());corrupt_refused+=1
    assert corrupt_refused==10
    mapping.write_text('6000 4 boot\n40000 8 exactFunction\n64000 4 exactHelper\n60004 4 stopped\n')
    subprocess.run([*common,'-fsanitize=address,undefined','-DPIONEER_Q38_NATIVE_ROOT_PROFILE_BRIDGE_TEST=1',
                    str(here/'pioneer_q38_native_root_profile_bridge.cpp'),
                    str(here/'pioneer_q38_native_root_profile_bridge_test.cpp'),'-o',str(binary)],check=True)
    env={**os.environ,'PIONEER_TEST_ROOT_PROFILE_MAP':str(mapping),'UBSAN_OPTIONS':'halt_on_error=1'}
    result=subprocess.run([str(binary)],env=env,capture_output=True,text=True,check=True)
    assert result.stdout.splitlines()==[
        'ROOT_INSTRUCTION_PROFILE total6/functions4/EXCLUSIVE actual PCs incl SIMULATED polling/NOT physical cycles or token speed',
        'ROOT_INSTRUCTION_FUNCTION 3 exactFunction','ROOT_INSTRUCTION_FUNCTION 1 boot',
        'ROOT_INSTRUCTION_FUNCTION 1 exactHelper','ROOT_INSTRUCTION_FUNCTION 1 stopped'],result.stdout
    print(result.stdout,end='')
    refused=0
    for flags in ([],['-DPIONEER_Q38_NATIVE_ROOT_PROFILE_BRIDGE_TEST=0']):
        result=subprocess.run([*common,'-E',*flags,str(here/'pioneer_q38_native_root_profile_bridge.cpp')],capture_output=True,text=True)
        assert result.returncode and 'Explicit isolated HOST' in result.stderr;refused+=1
    for role in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE',
                 'PIONEER_Q38_GLOBAL_WORKER_IMAGE','PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE','__FAST_MATH__'):
        for value in ('0','1'):
            result=subprocess.run([*common,'-E','-DPIONEER_Q38_NATIVE_ROOT_PROFILE_BRIDGE_TEST=1',
                                   '-D'+role+'='+value,str(here/'pioneer_q38_native_root_profile_bridge.cpp')],capture_output=True,text=True)
            assert result.returncode and 'Native ROOT instruction-profile bridge is HOST-only' in result.stderr;refused+=1
    # The emulated native fixture sees declarations ONLY,never the HOST
    # profile implementation with its deliberately unchanged kernel refusal.
    role=here/'pioneer_q38_token_native_root_profile_role.h'
    parent=here/'pioneer_q38_token_native_hidden_read_role.h'
    pattern=r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b'
    required=set(re.findall(pattern,role.read_text()+parent.read_text()))
    parent_text=parent.read_text();forbidden=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',parent_text[parent_text.index(' || defined('):]))
    forbidden|={'PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE','PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE',
                'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE','PIONEER_Q38_NATIVE_ROOT_PROFILE_BRIDGE_TEST'}
    base={**dict.fromkeys(required,'1'),'PIONEER_DRIVER_BUILD_TAG':'0x5154334e4f445256ULL'}
    def check(values,okay,tail=''):
        source=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))+'#include "'+str(role)+'"\n'+tail
        result=subprocess.run([common[0],'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
        assert (result.returncode==0)==okay,result.stderr
        if not okay:assert 'error:' in result.stderr
    check(base,True)
    for key in required:
        for value in (None,'0','2'):
            bad=base.copy()
            if value is None:del bad[key]
            else:bad[key]=value
            check(bad,False);refused+=1
        check(base,False,'#undef '+key+'\n#define '+key+' 0\n#include "'+str(role)+'"\n');refused+=1
    for key in forbidden:
        for value in ('0','1'):check({**base,key:value},False);refused+=1
    for tag in ('0','0x2026103100010075ULL','0x5154324e4f445256ULL'):
        check({**base,'PIONEER_DRIVER_BUILD_TAG':tag},False);refused+=1
    print(f'PASS_HOST_ONLY_ROOT_PROFILE exactPCCounts1 continuousHistory1 declarationOnlyNativeBridge1 actualAuthenticatedMap211Ranges1 sharedAliasesPreserved1 {corrupt_refused} corruptOrTruncatedBeforeToolOrOutputRefusals {refused} missing+zero+foreign+late+physical+Work refusals;NO native/card/model/cycles/TPS grant')
