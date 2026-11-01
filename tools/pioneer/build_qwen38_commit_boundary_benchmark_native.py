#!/usr/bin/env python3
"""Build a distinct bounded diagnostic; NEVER install/run hardware automatically."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys

OBJECT_SHA = 'f04166c669355593a80a98d39b7fafdd19914c5ec86730b373b7619d99365962'
MODULE_SHA = '78322cfa94db2519284efa12ad69cfa77981446f03081fc95aee2ee6234de8d1'
assert len(sys.argv) in (3, 4) and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
inspect = len(sys.argv) == 4
assert not inspect or sys.argv[3] == '--inspect-candidate'
assert inspect or OBJECT_SHA and MODULE_SHA, 'candidate not independently frozen'
previous, out = map(pathlib.Path, sys.argv[1:3])
here = pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    # Authenticate the qualified SDK argv and ENTIRE old1f3 module,then
    # reproduce that complete object/module from CURRENT sources FIRST.
    sys.argv = [str(here / 'tests/run_pioneer_q38_commit_selection_legacy_compile.py'),
                str(previous), str(out / 'unchanged-retained-driver')]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
args, link = old['args'][:], old['link'][:]
tool = old['compiler'][:-3]
obj = out / 'commit-boundary-benchmark-native.o'
args[args.index('-c') + 1] = str(here / 'tests/pioneer_q38_token_commit_boundary_benchmark_native_compile.cpp')
args[args.index('-o') + 1] = str(obj)
subprocess.run(args, check=True)
(out / 'compile-argv.txt').write_text('\n'.join(args) + '\n')


def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        row = line.split('\t')
        match = re.fullmatch(r'.*?:[0-9]+:[0-9]+:(.*)', row[0])
        assert len(row) == 3 and match and row[2] == 'static', line
        result[match[1]] = int(row[1])
    return result


before, after = frames(old['obj'].with_suffix('.su')), frames(obj.with_suffix('.su'))
for name, size in after.items():
    assert size <= 2048 or name in before and size <= before[name], (name, size)
module = out / 'tenstorrent_pci_probe-token-COMMIT-BOUNDARY34-BENCHMARK'
assert link[6] == str(old['obj'])
link[6], link[-1] = str(obj), str(module)
subprocess.run(link, check=True)
(out / 'link-argv.txt').write_text('\n'.join(link) + '\n')
table = subprocess.check_output([tool + 'nm', '-C', '--defined-only', str(module)], text=True)
undefined = subprocess.check_output([tool + 'nm', '-u', str(module)], text=True)
for required in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device',
                 'Q38TokenNativeControl(', 'Q38TokenFrontierControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<', 'PioneerQ38TokenCommitBoundaryPrograms',
                 'BeginQ38TokenNativeBackendScoped(', 'FinishQ38TokenNativeBackendScoped(', 'PostedCancelSourceGuard'):
    assert required in table, required
for forbidden in ('Q38TokenHiddenValidationControl(', 'OriginalSource', 'ReferenceProducts', 'ValidationSession',
                  'Q38WorkControl', 'NativeQ38PeerInstructionModel', 'SyntheticIO', 'PostedCancelEndpoint',
                  'PioneerQ38TokenRetainedCohortPrograms', 'PioneerQ38TokenFrontierPrograms',
                  'PioneerQ38TokenJobBootFoldPrograms', 'PioneerQ38TokenPreciseBetaPrograms',
                  'PioneerQ38TokenFaultPrograms', 'PioneerQ38TokenPushedPrograms', 'llama_', 'ggml_'):
    assert forbidden not in table and forbidden not in undefined, forbidden
raw = subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(obj)], text=True)
symbols = {row[-1]: (int(row[0], 16), int(row[1], 16)) for row in map(str.split, raw.splitlines()) if len(row) == 4}
raw = subprocess.check_output([tool + 'objdump', '-t', str(obj)], text=True)
sections = {row[-1]: row[-3] for row in map(str.split, raw.splitlines()) if len(row) >= 6 and row[-1] in symbols}


def payload(path, name, values, selected, label):
    start, size = values[name]
    target = out / (label + '.raw')
    subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', selected[name], str(path), str(target)], check=True)
    data = target.read_bytes()
    assert start + size <= len(data)
    return data[start:start + size]


header = (here / 'PioneerQ38TokenCommitBoundaryPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == 'f17464e86b23df5210830446fa894980deba0f1f9a469fd9676db788190df182'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(arrays) == 13
for digest, label in arrays:
    names = [name for name in symbols if 'PioneerQ38TokenCommitBoundaryPrograms' in name
             and name.endswith(str(len(label)) + label + 'E')]
    assert len(names) == 1 and hashlib.sha256(payload(obj, names[0], symbols, sections, label)).hexdigest() == digest, label
rope = [name for name, (_, size) in symbols.items()
        if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size == 2097152]
assert len(rope) == 1
old_table = subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(old['obj'])], text=True)
old_symbols = {row[-1]: (int(row[0], 16), int(row[1], 16))
               for row in map(str.split, old_table.splitlines()) if len(row) == 4}
old_table = subprocess.check_output([tool + 'objdump', '-t', str(old['obj'])], text=True)
old_sections = {row[-1]: row[-3] for row in map(str.split, old_table.splitlines())
                if len(row) >= 6 and row[-1] in old_symbols}
assert payload(obj, rope[0], symbols, sections, 'new-rope') == payload(old['obj'], rope[0], old_symbols, old_sections, 'old-rope')
graph_obj = out / 'commit-boundary-callgraph.o'
graph_args = args[:]
graph_args[graph_args.index('-o') + 1] = str(graph_obj)
subprocess.run([*graph_args, '-fcallgraph-info=su'], check=True)
assert graph_obj.read_bytes() == obj.read_bytes()
sys.path.insert(0, str(here / 'tests'))
from audit_pioneer_q38_stack_graph import audit, function_symbols, parse_graph, symbol
nodes, edges = parse_graph(graph_obj.with_suffix('.ci').read_text())
roots = [key for key in nodes if symbol(key) == '_ZL12ProbeControlPvjS_m']
assert len(roots) == 1
functions = function_symbols(subprocess.check_output([tool + 'objdump', '-t', str(graph_obj)], text=True))
results = audit(nodes, edges, functions, roots)
assert len(results) == 1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report = {'kind': 'PARTIAL_ACTUAL_COMMIT_BOUNDARY_PUBLIC_STACK_AUDIT', 'roots': results,
          'module_sha256': hashlib.sha256(module.read_bytes()).hexdigest(),
          'object_sha256': hashlib.sha256(obj.read_bytes()).hexdigest(), 'whole_kernel_stack_qualified': False,
          'limits': 'Known module only;unknown kernel/IRQ/entry/indirect costs excluded. NOT a Work grant.'}
(out / 'partial-public-entry-stack.json').write_text(json.dumps(report, indent=2) + '\n')
for path, expected in ((obj, OBJECT_SHA), (module, MODULE_SHA)):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:
        assert digest == expected, (path, digest)
    print(digest + ' ' + path.name)
print('PASS_BUILD_ONLY separate commit-boundary6607+6608 diagnostic/entireold04da+1f3 unchanged/'
      'all13 actual arrays+RoPE exact/same-owner counter+native lifecycle adapters/noCPU oracle+Work+TEST imports')
print('PARTIAL knownModuleChain' + str(results[0]['known_module_chain_bytes']) + '/unresolved'
      + str(len(results[0]['unresolved'])) + ';wholeKernelStackQualified=FALSE;'
        'NO native execution,physical,numerical,timing,resident Work or deployment grant')
