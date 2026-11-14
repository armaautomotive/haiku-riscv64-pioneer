#!/usr/bin/env python3
"""Actual full851 native bind/scope/retention; simulated OS/unused endpoints."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
args = sys.argv[1:]
sdk = None
if len(args) >= 2 and args[-2] == '--sdk':
    sdk = pathlib.Path(args[-1]);args = args[:-2]
    assert sdk.is_absolute() and sdk.is_dir()
assert not args or (len(args) == 2 and args[0] in ('--hybrid','--hybrid-matrix-inspection','--qgw8-inspection','--qgw8-trace-read-inspection','--qgw8-trace-init-inspection','--qgwd-inspection','--qgwe-inspection','--qgwf-inspection'))
qgwf = bool(args and args[0] == '--qgwf-inspection')
qgwe = qgwf or bool(args and args[0] == '--qgwe-inspection')
qgwd = qgwe or bool(args and args[0] == '--qgwd-inspection')
trace_init = bool(args and args[0] == '--qgw8-trace-init-inspection')
trace = bool(args and args[0] in ('--qgw8-trace-read-inspection','--qgw8-trace-init-inspection'))
qgw8 = qgwd or bool(args and args[0] in ('--qgw8-inspection','--qgw8-trace-read-inspection','--qgw8-trace-init-inspection'))
assert sdk is None or qgw8, 'SDK witness is limited to QGW8/QGWD inspection'
matrix = bool(args and args[0] == '--hybrid-matrix-inspection') or qgw8
hybrid = pathlib.Path(args[1]) if args else None
if hybrid:
    assert hybrid.is_absolute() and hybrid.is_file()
    if matrix:assert hashlib.sha256(hybrid.read_bytes()).hexdigest()==('b0c3d213676ded8ec484cee54908fc6443d4f2f50e199084a782230111fc1ac6' if qgwf else '4377811ebdc782bb67251beb120e32fdef18aab29ece5e8d77ba07bbcb0846b2' if qgwe else '90dd22aada31d665a42fda9fa158afd888d13e04c45365203ae5f42c0fb6daff' if qgwd else '72ec049c6905e37531b4fc21854991a8ddf513fc0944e8acde2ed73fc59a8588' if qgw8 else 'bf6e0c535879f2bcf336a23188fa8cd19e612d450d86e59a85211f604928b52a')
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
role = (kernel/'PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
defines = dict.fromkeys(parents, '1')
defines['PIONEER_DRIVER_BUILD_TAG'] = '0x51544b4e4f445256ULL'
defines.update({name:'pioneer_token_binding_test_'+name for name in
                ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')})
defines.update(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE='1', TT_PIONEER_EXPERIMENTAL_Q38_BLOCK='1')
if hybrid:
    defines.update(PIONEER_Q38_HYBRID_TRACKED_UPLOAD='1', PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY='1',
                   PIONEER_Q38_TOKEN_BF16_PROGRAMS='1', PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC='1',
                   PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC='1', PIONEER_Q38_HYBRID_PROGRAMS_HEADER='"'+str(hybrid)+'"')
if matrix:defines.update(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION='1',PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT='1')
if qgw8:defines.update(PIONEER_Q38_QGW8_NATIVE_INSPECTION='1',PIONEER_Q38_QGW8_LAYOUT_INSPECTION='1',
                         PIONEER_Q38_QGW8_PROGRAMS_INSPECTION='1',PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION='1')
if qgwd:
    del defines['PIONEER_Q38_QGW8_PROGRAMS_INSPECTION']
    for name in ('PIONEER_Q38_QGWD_NATIVE_INSPECTION','PIONEER_Q38_QGWD_PROGRAMS_INSPECTION',
                 'PIONEER_Q38_QGWD_PLAN_SIMULATION','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION','PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE',
                 'PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION','PIONEER_Q38_QGWD_CONTROL_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION','PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION','PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION'):
        defines[name]='1'
if qgwe:defines.update(PIONEER_Q38_QGWE_NATIVE_INSPECTION='1',PIONEER_Q38_QGWE_PROGRAMS_INSPECTION='1',PIONEER_Q38_QGWE_BATCH_DENSE_PLAN='1')
if qgwf:defines.update(PIONEER_Q38_QGWF_NATIVE_INSPECTION='1',PIONEER_Q38_QGWF_PROGRAMS_INSPECTION='1',PIONEER_Q38_QGWF_DUAL_BATCH_PLAN='1')
if trace:defines.update(PIONEER_Q38_QGW8_TRACE_READ_INSPECTION='1')
if trace_init:defines.update(PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION='1')
if qgwd:
    role_text='#include "'+str(here.parent/'PioneerQ38QgwdNativeInspectionRole.h')+'"\n'
    def qgwd_role(values, source=role_text):
        return subprocess.run([os.environ.get('CXX','c++'),'-std=c++17',
            *['-D'+k+'='+v for k,v in sorted(values.items())],'-x','c++','-fsyntax-only','-'],
            input=source,capture_output=True,text=True)
    assert qgwd_role(defines).returncode==0
    required=('_KERNEL_MODE','PIONEER_Q38_QGW8_NATIVE_INSPECTION','PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY',
              'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_HYBRID_TRACKED_UPLOAD',
              'PIONEER_Q38_QGWD_PROGRAMS_INSPECTION','PIONEER_Q38_QGWD_PLAN_SIMULATION',
              'PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION','PIONEER_Q38_QGWD_CONTROL_INSPECTION','TT_PIONEER_DRIVER_RELOAD',
              'PIONEER_DRIVER_BUILD_TAG','api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
    for name in required:
        changed=defines.copy();del changed[name]
        assert qgwd_role(changed).returncode,name
    for name in ('PIONEER_Q38_QGW8_PROGRAMS_INSPECTION','PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128','PIONEER_Q38_QGWD_NATIVE_141',
                 'PIONEER_Q38_QGWD_KERNEL_COMPILE_ONLY','PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY',
                 'PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT',
                 'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__'):
        changed=defines.copy();changed[name]='1'
        assert qgwd_role(changed).returncode,name
    changed=defines.copy();changed['PIONEER_DRIVER_BUILD_TAG']='0x2026111000010141ULL'
    assert qgwd_role(changed).returncode
    changed=defines.copy();changed['PIONEER_Q38_QGWD_NATIVE_INSPECTION']='0'
    assert qgwd_role(changed).returncode
    assert qgwd_role(defines,role_text+'#undef PIONEER_Q38_QGWD_NATIVE_INSPECTION\n'+role_text).returncode
    changed=defines.copy();del changed['PIONEER_Q38_QGWD_NATIVE_INSPECTION']
    assert qgwd_role(changed,role_text+'#define PIONEER_Q38_QGWD_NATIVE_INSPECTION 1\n'+role_text).returncode
    print('PASS QGWD fake-tag role parents and physical/Work/validation exclusions; no driver')
if qgw8 and not qgwd:
    role_probe = here/'pioneer_q38_qgw8_native_role_probe.cpp'
    compiler = os.environ.get('CXX', 'c++')
    def check_role(values):
        return subprocess.run([compiler, '-std=c++17', *['-D'+k+'='+v for k,v in sorted(values.items())],
                               '-fsyntax-only', str(role_probe)], capture_output=True, text=True)
    assert check_role(defines).returncode == 0
    required = ('_KERNEL_MODE', 'PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY', 'PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY',
                'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'PIONEER_Q38_HYBRID_TRACKED_UPLOAD',
                'PIONEER_Q38_QGW8_LAYOUT_INSPECTION', 'PIONEER_Q38_QGW8_PROGRAMS_INSPECTION',
                'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION', 'PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
                'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT', 'PIONEER_DRIVER_BUILD_TAG',
                'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
    for name in required:
        altered=defines.copy();del altered[name];result=check_role(altered)
        assert result.returncode and 'QGW8 native inspection requires the renamed fake-tag non-loadable profile' in result.stderr,name
    for name in ('PIONEER_Q38_HYBRID_FULL_NATIVE','PIONEER_Q38_HYBRID_MATRIX_NATIVE_127','PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY',
                 'PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT','PIONEER_Q38_BF16_FULL_NATIVE',
                 'PIONEER_Q38_BF16_FULL_CLIENT','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                 'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
        altered=defines.copy();altered[name]='1';result=check_role(altered)
        assert result.returncode and 'QGW8 native inspection requires the renamed fake-tag non-loadable profile' in result.stderr,name
    altered=defines.copy();altered['PIONEER_DRIVER_BUILD_TAG']='0x2026110700010127ULL';result=check_role(altered)
    assert result.returncode and 'QGW8 native inspection requires the renamed fake-tag non-loadable profile' in result.stderr
    altered=defines.copy();altered['PIONEER_Q38_QGW8_NATIVE_INSPECTION']='0';result=check_role(altered)
    assert result.returncode and 'QGW8 native inspection requires the renamed fake-tag non-loadable profile' in result.stderr
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-token-native-binding-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    session = source[source.index('struct PioneerSessionIO :'):]
    signature = '\tvoid QuarantineHostPage() {'
    assert session.count(signature) == 1
    begin = session.index(signature)
    at = session.index('{', begin)
    depth, end = 1, at+1
    while depth:
        depth += (session[end] == '{')-(session[end] == '}')
        end += 1
    (base/'token_native_host_quarantine.inc').write_text(session[begin:end]+'\n')
    # Exact WHOLE production Unsafe method,not an implementation of just
    # the new controlledToken exception which hides unrelated unsafe owners.
    start, end = '\tbool Unsafe(\n', '\n\tvoid Snapshot(tt_resident_session_result& r)'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_unsafe.inc').write_text(start+source.split(start, 1)[1].split(end, 1)[0])
    start, end = '// PIONEER_PROBE_LOCK_BEGIN', '// PIONEER_PROBE_LOCK_END'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_lock.inc').write_text(source.split(start, 1)[1].split(end, 1)[0])
    start, end = '// PIONEER_Q38_RETAIN_MEMORY_BEGIN', '// PIONEER_Q38_RETAIN_MEMORY_END'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_retain.inc').write_text(source.split(start, 1)[1].split(end, 1)[0])
    # Actual FIRST full-token branch of END. No later legacy cleanup is
    # silently provided by the fixture or treated as a new-token capability.
    start = 'EndQ38DispatchOwnership(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean)\n{\n'
    assert source.count(start) == 1
    branch = source.split(start, 1)[1].split('\n#endif', 1)[0]+'\n#endif\n'
    assert branch.startswith('#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT\n')
    (base/'token_native_end.inc').write_text(branch)
    binary = base/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *['-D'+k+'='+v for k, v in sorted(defines.items())],
                    '-I'+str(base), '-I'+str(kernel), '-I'+str(here.parent),
                    str(here/('pioneer_q38_qgwd_native_binding_test.cpp' if qgwd else 'pioneer_q38_qgw8_native_binding_test.cpp' if qgw8 else 'pioneer_q38_hybrid_native_binding_test.cpp' if hybrid else 'pioneer_q38_token_native_binding_test.cpp')),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    if sdk is not None:
        tool = sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
        headers = sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
        obj=base/'qgw8-bind-inspection.o'
        subprocess.run([str(tool)+'-g++', '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-Wno-misleading-indentation', '-Wno-unused-function', '-fno-exceptions', '-fno-rtti', '-fstack-usage',
                        '-mcmodel=medany', '-fpic', *['-D'+k+'='+v for k,v in sorted(defines.items())],
                        '-I'+str(base), '-I'+str(kernel), '-I'+str(here.parent), '-I'+str(headers), '-I'+str(headers/'os'),
                        '-idirafter', str(headers/'posix'), '-c', str(here/('pioneer_q38_qgwd_native_binding_test.cpp' if qgwd else 'pioneer_q38_qgw8_native_binding_test.cpp')),
                        '-o', str(obj)], check=True)
        names=subprocess.check_output([str(tool)+'-nm', '--defined-only', str(obj)], text=True)
        forbidden={'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}
        assert not any(line.split()[-1] in forbidden for line in names.splitlines() if line.split())
        rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
        for name in ('BindQ38TokenNative(', 'TokenModelSourceScope('):
            actual=[row for row in rows if name in row[0]]
            assert actual and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in actual),(name,actual)
        if trace:
            assert 'ReadTraceChunk' in names, 'trace read must actually instantiate in the SDK object'
            actual=[row for row in rows if 'ReadTraceChunk' in row[0]]
            assert actual and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in actual),actual
            print('PASS QGW8 trace read instantiated with compile-only IO and bounded individual frame; NO real IO or combined-stack proof')
        print('PASS '+('QGWF' if qgwf else 'QGWE' if qgwe else 'QGWD' if qgwd else 'QGW8')+' fake-native RISC-V bind object and bounded bind/scope frames; no conventional ABI/device')
