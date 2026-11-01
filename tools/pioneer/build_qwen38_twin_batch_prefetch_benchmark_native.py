#!/usr/bin/env python3
"""Distinct bounded diagnostic BUILD only;NEVER install or execute it."""
import hashlib
import json
import os
import pathlib
import re
import runpy
import subprocess
import sys

OBJECT_SHA = 'a0f0b93bf04325fc39074884a56ae0720e4370fef2c05457d726e1ba3504fe26'
MODULE_SHA = '010c184ef474f962205918fd9a9a34b23b2ab47c3cf6a62d285fcb36d2e9e899'
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 2 and (inspect or OBJECT_SHA and MODULE_SHA)
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
previous, out = map(pathlib.Path, args)
here = pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    # ENTIRE old flat8612+prefetch919d+batched34322 objects/modules reproduce
    # FIRST from the original authenticated compile/link commands.
    sys.argv = [str(here / 'build_qwen38_batched_prefetch_benchmark_native.py'),
                str(previous), str(out / 'unchanged-prefetch')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
control, old_module = parent['obj'], parent['module']
assert hashlib.sha256(control.read_bytes()).hexdigest() == '368307386dbf6ecfa29a146ddf8e9c11559d8227c8d9137816ee6f4113c59d19'
assert hashlib.sha256(old_module.read_bytes()).hexdigest() == '34322035d8aadcc8667d8a7ec3f2a144646f0060604e9e00ebfdd59fa7e4ca1c'
command = parent['command'][:]
tool = command[0][:-3]
obj = out / 'token-twin-batch-prefetch-benchmark-native.o'
command[command.index('-c') + 1] = str(here / 'tests/pioneer_q38_token_twin_batch_prefetch_benchmark_native_compile.cpp')
command[command.index('-o') + 1] = str(obj)
subprocess.run(command, check=True)
(out / 'compile-argv.txt').write_text('\n'.join(command) + '\n')


def records(path):
    raw = subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(path)], text=True)
    values = {row[-1]: (int(row[0], 16), int(row[1], 16)) for row in map(str.split, raw.splitlines()) if len(row) == 4}
    raw = subprocess.check_output([tool + 'objdump', '-t', str(path)], text=True)
    sections = {row[-1]: row[-3] for row in map(str.split, raw.splitlines()) if len(row) >= 6 and row[-1] in values}
    return values, sections


def payload(path, name, values, sections, label):
    start, size = values[name]
    target = out / (label + '.raw')
    subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', sections[name], str(path), str(target)], check=True)
    data = target.read_bytes()
    assert start + size <= len(data)
    return data[start:start + size]


symbols, sections = records(obj)
old_symbols, old_sections = records(control)
header = (here / 'PioneerQ38TokenTwinBatchPrefetchPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == '049613d63e3524be9a5aa1d3ef5f234adf4c61e4142a4d79eaf7e3bd50ae9db1'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(arrays) == 14 and sum(label == 'PeerFetch' for _, label in arrays) == 1
for digest, label in arrays:
    found = [name for name in symbols if 'PioneerQ38TokenTwinBatchPrefetchPrograms' in name and name.endswith(str(len(label)) + label + 'E')]
    assert len(found) == 1 and hashlib.sha256(payload(obj, found[0], symbols, sections, label)).hexdigest() == digest
rope = [name for name, (_, size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size == 2097152]
assert len(rope) == 1 and rope[0] in old_symbols
assert payload(obj, rope[0], symbols, sections, 'new-rope') == payload(control, rope[0], old_symbols, old_sections, 'old-rope')
before, after = parent['parent']['frames'](control.with_suffix('.su')), parent['parent']['frames'](obj.with_suffix('.su'))
for name, size in after.items():
    assert size <= 2048 or name in before and size <= before[name], (name, size)
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
assert all(name in symbols for name in abi)
module = out / 'tenstorrent_pci_probe-token-TWIN-BATCH-PREFETCH34-BENCHMARK'
link = parent['link'][:]
assert link[6] == str(control) and link[-1] == str(old_module)
link[6], link[-1] = str(obj), str(module)
subprocess.run(link, check=True)
(out / 'link-argv.txt').write_text('\n'.join(link) + '\n')
table = subprocess.check_output([tool + 'nm', '-C', '--defined-only', str(module)], text=True)
undefined = subprocess.check_output([tool + 'nm', '-u', str(module)], text=True)
for required in ('PioneerQ38TokenTwinBatchPrefetchPrograms', 'Q38TokenNativeControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<', 'Q38TokenFrontierControl(',
                 'BeginQ38TokenNativeBackendScoped(', 'FinishQ38TokenNativeBackendScoped('):
    assert required in table, required
for forbidden in ('PioneerQ38TokenBatchedPrefetchPrograms', 'PioneerQ38TokenPrefetchPrograms', 'PioneerQ38TokenFlatMatrixPrograms', 'PioneerQ38TokenEmptyReceiptPollPrograms',
                  'PioneerQ38TokenCounterGatherPrograms', 'PioneerQ38TokenTidBoundaryPrograms', 'PioneerQ38TokenPacketIdentityBoundaryPrograms',
                  'PioneerQ38TokenCohortLocalSourcesPrograms', 'PioneerQ38TokenCommitBoundaryPrograms', 'PioneerQ38TokenRetainedCohortPrograms',
                  'PioneerQ38TokenFrontierPrograms', 'PioneerQ38TokenJobBootFoldPrograms', 'PioneerQ38TokenPreciseBetaPrograms',
                  'PioneerQ38TokenFaultPrograms', 'PioneerQ38TokenPushedPrograms', 'ReferenceProducts', 'OriginalSource',
                  'ValidationSession', 'Q38WorkControl', 'NativeQ38PeerInstructionModel', 'Q38NativePeerTestCpu',
                  'PioneerQ38ObservedTrace', 'SyntheticIO', 'llama_', 'ggml_'):
    assert forbidden not in table and forbidden not in undefined, forbidden
graph = out / 'token-twin-batch-prefetch-callgraph.o'
graph_args = command[:]
graph_args[graph_args.index('-o') + 1] = str(graph)
subprocess.run([*graph_args, '-fcallgraph-info=su'], check=True)
assert graph.read_bytes() == obj.read_bytes()
sys.path.insert(0, str(here / 'tests'))
from audit_pioneer_q38_stack_graph import audit, function_symbols, parse_graph, symbol
nodes, edges = parse_graph(graph.with_suffix('.ci').read_text())
roots = [key for key in nodes if symbol(key) == '_ZL12ProbeControlPvjS_m']
assert len(roots) == 1
functions = function_symbols(subprocess.check_output([tool + 'objdump', '-t', str(graph)], text=True))
results = audit(nodes, edges, functions, roots)
assert len(results) == 1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report = {'kind': 'PARTIAL_ACTUAL_TWIN_BATCH_PREFETCH_PUBLIC_STACK_AUDIT', 'roots': results,
          'module_sha256': hashlib.sha256(module.read_bytes()).hexdigest(),
          'object_sha256': hashlib.sha256(obj.read_bytes()).hexdigest(), 'whole_kernel_stack_qualified': False,
          'limits': 'Known module only;kernel/IRQ/entry/indirect costs excluded. NOT Work or deployment qualification.'}
(out / 'partial-public-entry-stack.json').write_text(json.dumps(report, indent=2) + '\n')
print('PARTIAL knownModuleChain' + str(results[0]['known_module_chain_bytes']) + '/unresolved' +
      str(len(results[0]['unresolved'])) + ';wholeKernelStackQualified=FALSE', flush=True)
for path, expected in ((obj, OBJECT_SHA), (module, MODULE_SHA)):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:
        assert digest == expected, (path, digest)
    print(digest + ' ' + path.name, flush=True)
print('BUILD_ONLY distinct bounded twin-batch module/all14 arrays+unchanged rope+entire old public controls FIRST;'
      'NO native lifecycle,staging,installation,model quality,hardware,Work or TPS grant', flush=True)
