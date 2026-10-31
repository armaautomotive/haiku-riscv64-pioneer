#!/usr/bin/env python3
"""Build/inspect DEBUG74 module and partial stack graph; NO installation.

Linking is not deployment qualification. Unknown kernel/indirect/entry/IRQ
stack use is NOT zero; no full-model, Work or performance grant is produced.
"""
import hashlib
import json
import os
import pathlib
import runpy
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit(2)
qualified, out = map(pathlib.Path, sys.argv[1:])
assert qualified.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(here/'tests'))
from audit_pioneer_q38_stack_graph import audit, function_symbols, parse_graph, symbol
saved = sys.argv[:]
try:
    sys.argv = [str(here/'tests/run_pioneer_q38_token_validation_native_compile.py'), str(qualified), str(out/'sdk')]
    compiled = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
legacy = compiled['control']['control']['control']['base']
obj, args, tool = compiled['obj'], list(compiled['args']), compiled['tool']
assert hashlib.sha256(obj.read_bytes()).hexdigest() == '0e1dab1b8d78cba77244e3e98aedb3bf14358c7bd5219fc0add98e612fbb6762'
old_object = legacy['original'][legacy['original'].index('-o')+1]
links = [line for line in legacy['commands'] if '-shared' in line and old_object in line]
assert len(links) == 1
link = links[0][:]
assert link[:4] == [args[0], '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe']
assert link[-2] == '-o' and [pathlib.Path(path).name for path in link[4:-2]] == [
    'crtbeginS.o', 'haiku_version_glue.o', 'tenstorrent_pci_probe.o', 'PioneerPciProfile.o', 'kernel.so', 'libgcc.a', 'crtendS.o']
module = out/'tenstorrent_pci_probe-token-DEBUG74'
link = [str(obj) if value == old_object else value for value in link]
link[-1] = str(module)
assert all(pathlib.Path(path).is_file() for path in link[4:-2])
subprocess.run(link, check=True, env=legacy['clean'])
table = subprocess.check_output([tool+'nm', '-C', '--defined-only', str(module)], text=True)
undefined = subprocess.check_output([tool+'nm', '-u', str(module)], text=True)
names = {row[-1] for row in map(str.split, table.splitlines()) if row}
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= names
for required in ('Q38TokenNativeControl', 'BeginQ38TokenNativeBackendScoped', 'FinishQ38TokenNativeBackendScoped'):
    assert required in table
for forbidden in ('OriginalSource', 'ReferenceProducts', 'ValidationSession', 'Q38WorkControl',
                  'NativeQ38PeerInstructionModel', 'Q38NativePeerTestCpu', 'SyntheticIO', 'pioneer_token_context_'):
    assert forbidden not in table and forbidden not in undefined, forbidden
# Emit metadata ONLY, retaining the original machine code/settings exactly.
graph_obj = out/'token-validation-callgraph.o'
graph_args = args[:]
graph_args[graph_args.index('-o')+1] = str(graph_obj)
subprocess.run([*graph_args, '-fcallgraph-info=su'], check=True, env=legacy['clean'])
assert graph_obj.read_bytes() == obj.read_bytes()
graph = graph_obj.with_suffix('.ci').read_bytes()
nodes, edges = parse_graph(graph.decode())
roots = [name for name in nodes if symbol(name) == '_ZL12ProbeControlPvjS_m']
assert len(roots) == 1
functions = function_symbols(subprocess.check_output([tool+'objdump', '-t', str(graph_obj)], text=True))
results = audit(nodes, edges, functions, roots)
assert len(results) == 1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report = {'kind':'PARTIAL_ACTUAL_DEBUG74_PUBLIC_ENTRY_STACK_AUDIT', 'roots':results,
          'module_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),
          'object_sha256':hashlib.sha256(obj.read_bytes()).hexdigest(),
          'graph_sha256':hashlib.sha256(graph).hexdigest(), 'whole_kernel_stack_qualified':False,
          'limits':'Known module chains only;unknown indirect/kernel/entry/IRQ costs are excluded,NOT zero. '
                   'No deployment,full-model,Work,physical or performance qualification.'}
(out/'partial-public-entry-stack.json').write_text(json.dumps(report, indent=2)+'\n')
print(report['module_sha256']+' '+module.name)
print('PASS linked actual DEBUG74 conventional ABI/full G3 token control+backend/no CPU oracle+Work+mock imports/no undefined link failures')
print('OBSERVED actual ProbeControl knownModuleChain'+str(results[0]['known_module_chain_bytes'])
      +'/unresolved'+str(len(results[0]['unresolved']))+'/reached'+str(results[0]['reached_nodes'])
      +';PARTIAL ONLY,wholeKernelStackQualified=FALSE;metadata object BYTE-IDENTICAL')
print('BUILD+INSPECT ONLY;NO installation,device IO,full-model numerical/lifecycle/physical/Work/TPS grant')
