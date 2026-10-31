#!/usr/bin/env python3
"""Build a distinct all-card diagnostic;no automatic installation/execution."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys

OBJECT_SHA = '04da32526ffb4b2bc297ae21375a15e3d3df7141c60cca6747abba64189a3f4a'
MODULE_SHA = '1f3bcff838ff9f95d5f450c53000f02f80ddcd073aa5a5cc3018ee4208e0847c'
assert len(sys.argv) in (3, 4) and not any(
    key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
inspect = len(sys.argv) == 4
assert not inspect or sys.argv[3] == '--inspect-candidate'
assert inspect or OBJECT_SHA and MODULE_SHA, 'candidate not independently frozen'
previous, out = map(pathlib.Path, sys.argv[1:3])
here = pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'build_qwen38_frontier_benchmark_native.py'),
                str(previous), str(out / 'unchanged-frontier-driver')]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
# Reproduce the ENTIRE old9ad+4eb+44209+18362 control chain FIRST.
args = old['args'][:]
tool = old['tool']
obj = out / 'retained-cohort-benchmark-native.o'
args[args.index('-c') + 1] = str(here / 'tests/pioneer_q38_token_retained_cohort_benchmark_native_compile.cpp')
args[args.index('-o') + 1] = str(obj)
subprocess.run(args, check=True)
(out / 'compile-argv.txt').write_text('\n'.join(args) + '\n')
frames = old['old']['control']['compiled']['frames'](obj.with_suffix('.su'))
for name, size in frames.items():
    assert size <= 2048 or name in old['frames'] and size <= old['frames'][name], (name, size)
module = out / 'tenstorrent_pci_probe-token-RETAINED-COHORT34-BENCHMARK'
link = old['link'][:]
assert link[6] == str(old['obj'])
link[6] = str(obj)
link[-1] = str(module)
subprocess.run(link, check=True)
(out / 'link-argv.txt').write_text('\n'.join(link) + '\n')
table = subprocess.check_output([tool + 'nm', '-C', '--defined-only', str(module)], text=True)
undefined = subprocess.check_output([tool + 'nm', '-u', str(module)], text=True)
for required in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver',
                 'publish_devices', 'find_device', 'Q38TokenNativeControl(',
                 'Q38TokenFrontierControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<',
                 'PioneerQ38TokenRetainedCohortPrograms', 'BeginQ38TokenNativeBackendScoped(',
                 'FinishQ38TokenNativeBackendScoped(', 'PostedCancelSourceGuard'):
    assert required in table, required
for forbidden in ('Q38TokenHiddenValidationControl(', 'OriginalSource', 'ReferenceProducts',
                  'ValidationSession', 'Q38WorkControl', 'NativeQ38PeerInstructionModel',
                  'SyntheticIO', 'PostedCancelEndpoint', 'PioneerQ38TokenFrontierPrograms',
                  'PioneerQ38TokenJobBootFoldPrograms', 'PioneerQ38TokenPreciseBetaPrograms',
                  'PioneerQ38TokenFaultPrograms', 'PioneerQ38TokenPushedPrograms'):
    assert forbidden not in table and forbidden not in undefined, forbidden
raw = subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(obj)], text=True)
symbols = {row[-1]: (int(row[0], 16), int(row[1], 16))
           for row in map(str.split, raw.splitlines()) if len(row) == 4}
sections = subprocess.check_output([tool + 'objdump', '-t', str(obj)], text=True)
section = {row[-1]: row[-3] for row in map(str.split, sections.splitlines())
           if len(row) >= 6 and row[-1] in symbols}


def payload(name, label):
    start, size = symbols[name]
    path = out / (label + '.raw')
    subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', section[name], str(obj), str(path)], check=True)
    data = path.read_bytes()
    assert start + size <= len(data)
    return data[start:start + size]


header = (here / 'PioneerQ38TokenRetainedCohortPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == '3e5cec29942498a78227effe268f58a7fa567358d8f58dd7d04922a1ec364803'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(arrays) == 13
for digest, label in arrays:
    names = [name for name in symbols if 'PioneerQ38TokenRetainedCohortPrograms' in name
             and name.endswith(str(len(label)) + label + 'E')]
    assert len(names) == 1 and hashlib.sha256(payload(names[0], label)).hexdigest() == digest, label
rope = [name for name, (_, size) in symbols.items()
        if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size == 2097152]
assert len(rope) == 1 and payload(rope[0], 'new-rope') == old['payload'](rope[0], 'expected-rope')
graph_obj = out / 'retained-cohort-callgraph.o'
graph_args = args[:]
graph_args[graph_args.index('-o') + 1] = str(graph_obj)
subprocess.run([*graph_args, '-fcallgraph-info=su'], check=True)
assert graph_obj.read_bytes() == obj.read_bytes()
nodes, edges = old['parse_graph'](graph_obj.with_suffix('.ci').read_text())
roots = [key for key in nodes if old['symbol'](key) == '_ZL12ProbeControlPvjS_m']
assert len(roots) == 1
functions = old['function_symbols'](subprocess.check_output([tool + 'objdump', '-t', str(graph_obj)], text=True))
results = old['audit'](nodes, edges, functions, roots)
assert len(results) == 1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report = {'kind': 'PARTIAL_ACTUAL_RETAINED_COHORT_PUBLIC_STACK_AUDIT', 'roots': results,
          'module_sha256': hashlib.sha256(module.read_bytes()).hexdigest(),
          'object_sha256': hashlib.sha256(obj.read_bytes()).hexdigest(),
          'whole_kernel_stack_qualified': False,
          'limits': 'Known module only;unknown kernel/IRQ/entry/indirect costs excluded. NOT a Work grant.'}
(out / 'partial-public-entry-stack.json').write_text(json.dumps(report, indent=2) + '\n')
for path, expected in ((obj, OBJECT_SHA), (module, MODULE_SHA)):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:
        assert digest == expected, (path, digest)
    print(digest + ' ' + path.name)
print('PASS_BUILD_ONLY retained-cohort6607+6608 diagnostic/entireold9ad+4eb+44209+18362 unchanged/'
      'all13 actualarrays+RoPE exact/sameowner counter+native lifecycle/noCPU oracle+Work+TEST imports')
print('PARTIAL knownModuleChain' + str(results[0]['known_module_chain_bytes']) +
      '/unresolved' + str(len(results[0]['unresolved'])) +
      ';wholeKernelStackQualified=FALSE;NO physical,numerical,timing,residentWork or deployment grant')
