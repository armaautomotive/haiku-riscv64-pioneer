#!/usr/bin/env python3
"""Full-model native initializers with actual frozen zero RV32; NO device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

precise_context = '--precise-context' in sys.argv
if precise_context:
    assert sys.argv.count('--precise-context') == 1
    sys.argv.remove('--precise-context')
embedding_modes = ('--execution-embedding', '--execution-embedding-source-loss', '--execution-embedding-readback-loss', '--execution-layer-dispatch')
readout_flow = len(sys.argv) == 7 and sys.argv[2] == '--execution-readout-full-flow'
clock_contract = len(sys.argv)==3 and sys.argv[2]=='--clock-owner-contract'
pci_close = len(sys.argv)==7 and sys.argv[2] in ('--execution-pci-close','--execution-pci-generation-fault','--execution-pci-readback-fault')
views_close = pci_close or len(sys.argv)==7 and sys.argv[2] in ('--execution-views-close','--execution-views-delete-fault','--execution-views-generation-fault')
hardware_seal = views_close or len(sys.argv)==7 and sys.argv[2] in ('--execution-hardware-seal','--execution-hardware-seal-tlb-fault')
clock_end = hardware_seal or len(sys.argv)==7 and sys.argv[2] in ('--execution-clock-end','--execution-clock-end-generation-fault',
    '--execution-clock-end-response-fault','--execution-clock-end-unmap-fault')
restore_shared = clock_end or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-shared-tlb','--execution-restore-shared-tlb-fault',
    '--execution-restore-shared-tlb-readback-fault','--execution-restore-shared-tlb-verify-fault')
restore_root = restore_shared or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-root-store','--execution-restore-root-store-fault',
    '--execution-restore-root-store-map-fault','--execution-restore-root-store-config-fault','--execution-restore-root-store-scratch-fault',
    '--execution-restore-root-store-fifo-fault','--execution-restore-root-store-overflow-fault')
restore_peers = restore_root or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-peers','--execution-restore-peers-fault',
    '--execution-restore-peers-map-fault','--execution-restore-peers-config-fault','--execution-restore-peers-scratch-fault',
    '--execution-restore-peers-fifo-fault','--execution-restore-peers-overflow-fault')
restore_dispatch = restore_peers or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-dispatch','--execution-restore-dispatch-fault')
restore_empty = restore_dispatch or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-empty','--execution-restore-empty-fault')
readout_empty = restore_empty or len(sys.argv) == 7 and sys.argv[2] == '--execution-readout-empty'
full_flow = readout_flow or len(sys.argv) == 7 and sys.argv[2] == '--execution-full-flow'
hold_layer = readout_empty or len(sys.argv) == 7 and sys.argv[2] == '--execution-hold-first-layer'
cancel_layer = hold_layer or len(sys.argv) == 7 and sys.argv[2] == '--execution-cancel-first-layer'
full_layer = full_flow or cancel_layer or len(sys.argv) == 7 and sys.argv[2] == '--execution-first-layer'
first_matrix = full_layer or len(sys.argv) == 6 and sys.argv[2] == '--execution-first-matrix'
if full_layer:
    prefix = pathlib.Path(sys.argv[6])
    if full_flow:
        assert prefix.is_absolute() and prefix.is_dir() and not prefix.is_symlink() and not any(prefix.iterdir())
    else:
        assert prefix.is_absolute() and prefix.parent.is_dir() and not prefix.exists() and not prefix.is_symlink()
embedding = first_matrix or len(sys.argv) == 4 and sys.argv[2] in embedding_modes
peer_bootstrap = len(sys.argv) == 5 and sys.argv[2] == '--bootstrap-peer-engines'
peer_engines = peer_bootstrap or first_matrix
if precise_context:
    assert peer_engines, 'QTP2 context requires actual virgin peer instruction engines'
terminal_faults = len(sys.argv)==3 and sys.argv[2]=='--terminal-observation-faults'
preparation_faults = len(sys.argv)==3 and sys.argv[2] in ('--preparation-fifo-faults', '--preparation-access-faults')
preparation_endpoints = len(sys.argv)==3 and sys.argv[2]=='--preparation-endpoint-contract'
assert clock_contract or preparation_endpoints or preparation_faults or peer_bootstrap or embedding or terminal_faults or len(sys.argv) in (2,3) and (len(sys.argv) == 2 or sys.argv[2] in ('--initializer-fifo-faults', '--faults', '--late-faults', '--held', '--held-faults', '--runtime', '--runtime-pci-capture-fault', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'))
if embedding:
    assert pathlib.Path(sys.argv[3]).is_absolute() and pathlib.Path(sys.argv[3]).is_file()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
image = pathlib.Path(sys.argv[1])
assert image.is_absolute() and hashlib.sha256((image/'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
here = pathlib.Path(__file__).resolve().parent
if peer_engines:
    peer_arg = 3 if peer_bootstrap else 4
    for directory, script in ((pathlib.Path(sys.argv[peer_arg]), here/'run_pioneer_q38_mixed_worker_tests.py'),
                              (pathlib.Path(sys.argv[peer_arg+1]), here/'run_pioneer_q38_precise_worker_tests.py' if precise_context else here.parent/'build_qwen38_global_worker_image.py')):
        assert directory.is_absolute()
        tree = ast.parse(script.read_text())
        pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
        for name, size, sha in pins:
            data = (directory/name).read_bytes()
            assert len(data) == size and hashlib.sha256(data).hexdigest() == sha
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
role = (kernel/'PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
defines = dict.fromkeys(parents, '1')
defines['PIONEER_DRIVER_BUILD_TAG'] = '0x51544b4e4f445256ULL'
if precise_context:
    defines.update(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT='1', PIONEER_Q38_TOKEN_PRECISE_PROGRAMS='1',
                   PIONEER_DRIVER_BUILD_TAG='0x5154324e4f445256ULL')
defines.update({name:'pioneer_token_init_test_'+name for name in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')})
defines.update(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE='1', TT_PIONEER_EXPERIMENTAL_Q38_BLOCK='1')
if clock_contract or preparation_endpoints or preparation_faults or terminal_faults or peer_bootstrap or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--held', '--held-faults', '--runtime', '--runtime-pci-capture-fault', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE'] = '1'
if clock_contract or terminal_faults or peer_bootstrap or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--runtime', '--runtime-pci-capture-fault', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE'] = '1'
if clock_contract or terminal_faults or peer_bootstrap or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE'] = '1'
if clock_contract or terminal_faults or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE'] = '1'
if terminal_faults or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE'] = '1'
if embedding:
    defines['PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE'] = '1'
    defines['PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE'] = '1'
    if first_matrix or sys.argv[2] == '--execution-layer-dispatch':
        defines['PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE'] = '1'
if peer_engines:
    defines.update(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE='1', PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE='1')
if first_matrix:
    defines['PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE'] = '1'
if full_layer:
    defines['PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE'] = '1'
if cancel_layer:
    defines['PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE'] = '1'
if hold_layer:
    defines['PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE'] = '1'
if readout_empty:
    defines['PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE'] = '1'
if restore_empty:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE'] = '1'
if restore_dispatch:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE'] = '1'
if restore_peers:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE'] = '1'
if restore_root:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE'] = '1'
if restore_shared:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE'] = '1'
if clock_contract or clock_end:
    defines['PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE'] = '1'
if hardware_seal:
    defines['PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE'] = '1'
if views_close:
    defines['PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE'] = '1'
if pci_close:
    defines['PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE'] = '1'
if readout_flow:
    defines['PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE'] = '1'
if full_flow:
    defines.update(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE='1', PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE='1')
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-token-native-init-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    start, end = '// PIONEER_Q38_CLOCK_IO_BEGIN', '// PIONEER_Q38_CLOCK_IO_END'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_clock_io.inc').write_text(source.split(start,1)[1].split(end,1)[0])
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
    # The session wrapper calls this actual production base method. The
    # old HostReadMock assertion incorrectly assumes a page is still live
    # after every retention failure,including after successful release.
    host_class = source[source.index('struct PioneerHostPageIO {'):source.index('struct PioneerOutputBufferIO {')]
    assert host_class.count(signature)==1
    begin=host_class.index(signature);at=host_class.index('{',begin);depth,end=1,at+1
    while depth:
        depth+=(host_class[end]=='{')-(host_class[end]=='}');end+=1
    (base/'token_native_base_host_quarantine.inc').write_text(host_class[begin:end]+'\n')
    start, end = '\tbool Unsafe(\n', '\n\tvoid Snapshot(tt_resident_session_result& r)'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_unsafe.inc').write_text(start+source.split(start, 1)[1].split(end, 1)[0])
    for name, marker in (('lock', 'PIONEER_PROBE_LOCK'), ('retain', 'PIONEER_Q38_RETAIN_MEMORY')):
        start, end = '// '+marker+'_BEGIN', '// '+marker+'_END'
        assert source.count(start) == source.count(end) == 1
        (base/('token_native_'+name+'.inc')).write_text(source.split(start, 1)[1].split(end, 1)[0])
    start = 'EndQ38DispatchOwnership(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean)\n{\n'
    assert source.count(start) == 1
    branch = source.split(start, 1)[1].split('\n#endif', 1)[0]+'\n#endif\n'
    assert branch.startswith('#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT\n')
    (base/'token_native_end.inc').write_text(branch)

    def method(signature):
        assert source.count(signature) == 1
        begin = source.index(signature)
        at = source.index('{', begin)
        depth, end = 1, at+1
        while depth:
            depth += (source[end] == '{')-(source[end] == '}')
            end += 1
        return source[begin:end]+'\n'

    (base/'token_native_zero_callbacks.inc').write_text(''.join(method(signature) for signature in
        ('bool PioneerSessionIO::StateZeroBeforeCohort()', 'bool PioneerSessionIO::StateZeroOtherCoresHeld()', 'bool PioneerSessionIO::NonRootHeldOwner()')))
    binary = base/'test'
    extra = []
    if full_flow:
        trace_object = base/'native-observed-trace.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-c', str(here/'pioneer_q38_native_trace_test_bridge.cpp'), '-o', str(trace_object)], check=True)
        extra += [str(trace_object)]  # const observation DATA only,HOST TU without native/kernel/Work roles
    if peer_engines:
        peer_object = base/'native-peer-engine.o'
        peer_flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
        if precise_context:
            peer_flags += ['-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1']  # DATA only;never native/kernel roles
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', *peer_flags, '-c', str(here/'pioneer_q38_native_peer_test_bridge.cpp'), '-o', str(peer_object)], check=True)
        extra += [str(peer_object)]  # HOST-only TU;never native/kernel-role flags
    if embedding:
        # RAW byte transport is a separate TEST-only TU,without native-role
        # defines. Never promote its CPU-validation preprocessor role into
        # the native factory/driver candidate or its SDK/Work build.
        raw_object = base/'original-raw.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-c', str(here/'pioneer_q38_whole_original_bytes.cpp'), '-o', str(raw_object)], check=True)
        bridge_object = base/'embedding-raw-bridge.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-c', str(here/'pioneer_q38_embedding_raw_test_bridge.cpp'), '-o', str(bridge_object)], check=True)
        extra += [str(raw_object), str(bridge_object)]
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *['-D'+k+'='+v for k, v in sorted(defines.items())],
                    '-I'+str(base), '-I'+str(kernel), str(here/'pioneer_q38_token_native_initialization_test.cpp'),
                    *extra, '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
