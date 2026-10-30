#!/usr/bin/env python3
"""Actual private G3 backend callgraph; NEVER a whole-kernel stack grant."""
import hashlib
import json
import os
import pathlib
import runpy
import subprocess
import sys

from audit_pioneer_q38_stack_graph import audit, function_symbols, parse_graph, symbol

assert len(sys.argv) == 3 and not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
qualified, out = map(pathlib.Path, sys.argv[1:])
assert qualified.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here/'run_pioneer_q38_token_pushed_native_context_compile.py'), str(qualified), str(out/'controls')]
    control = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
obj = out/'backend-callgraph-NODRV.o'
args = list(control['args'])
args[args.index('-o')+1] = str(obj)
# Metadata emission ONLY. Retain the actual optimization, warnings, ABI,
# firmware, stack-usage and tail-call settings of the qualified G3 object.
args += ['-fcallgraph-info=su']
subprocess.run(args, check=True)
image = obj.read_bytes()
assert image == control['obj'].read_bytes(), 'callgraph metadata must not change actual compiled implementation bytes'
graph = obj.with_suffix('.ci').read_bytes()
nodes, edges = parse_graph(graph.decode('utf-8'))
functions = function_symbols(subprocess.check_output([control['tool']+'objdump', '-t', str(obj)], text=True))
roots = sorted(name for name in nodes if symbol(name).startswith('pioneer_token_context_backend_'))
assert {symbol(name) for name in roots} == {'pioneer_token_context_backend_begin', 'pioneer_token_context_backend_finish',
                                         'pioneer_token_context_backend_control'}
results = audit(nodes, edges, functions, roots)
assert len(results) == 3
for result in results:
    assert not result['dynamic'] and not result['cycles'], result
    assert result['known_module_chain_bytes'] is not None and result['known_module_chain_bytes'] > 0
    assert not result['whole_kernel_stack_qualified']
# Never turn an excluded external/indirect frame into a zero-byte callee or
# infer spare stack from 16KiB minus this partial module-chain measurement.
report = {'kind':'PARTIAL_PRIVATE_BACKEND_COMPILER_STACK_AUDIT',
          'object_sha256':hashlib.sha256(image).hexdigest(),
          'graph_sha256':hashlib.sha256(graph).hexdigest(), 'roots':results,
          'whole_kernel_stack_qualified':False,
          'limits':'Known module chains only. Unknown/indirect/kernel callees, entry and interrupt overhead remain unbounded. '
                   'Tail edges counted conservatively as nested calls. No driver-load, Work, accelerator-stack, physical or TPS grant.'}
assert obj.read_bytes() == image and obj.with_suffix('.ci').read_bytes() == graph
(out/'partial-stack-audit.json').write_text(json.dumps(report, indent=2)+'\n')
for result in results:
    print('OBSERVED '+result['root']+'/knownModuleChain'+str(result['known_module_chain_bytes'])
          +'/unresolved'+str(len(result['unresolved']))+'/reached'+str(result['reached_nodes'])
          +';PARTIAL ONLY,wholeKernelStackQualified=FALSE')
print('PASS actual G3 object BYTE-IDENTICAL with/without GCC callgraph metadata;3 backend roots analyzed;'
      'NO recursion/dynamic frame observed,unknown contributions EXCLUDED not zero;NOT whole-kernel/native/physical/Work/TPS qualification')
