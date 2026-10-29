#!/usr/bin/env python3
"""Compiler metadata audit tests;never native/physical stack qualification."""
import pathlib
import subprocess
import tempfile
from audit_pioneer_q38_stack_graph import audit, function_symbols, parse_graph

here = pathlib.Path(__file__).resolve().parent
compiler = pathlib.Path('/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
dump = compiler.with_name('riscv64-unknown-haiku-objdump')
with tempfile.TemporaryDirectory(prefix='pioneer-stack-graph-', dir='/private/tmp') as temporary:
    image = pathlib.Path(temporary)/'fixture.o'
    subprocess.run([str(compiler), '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-optimize-sibling-calls',
        '-fstack-usage', '-fcallgraph-info=su', '-c', str(here/'pioneer_q38_stack_graph_fixture.cpp'), '-o', str(image)], check=True)
    nodes, edges = parse_graph(image.with_suffix('.ci').read_text())
    functions = function_symbols(subprocess.check_output([str(dump), '-t', str(image)], text=True))
    roots = ['stack_probe_good', 'stack_probe_external', 'stack_probe_indirect', 'stack_probe_recursive_a', 'stack_probe_dynamic']
    results = {r['root']:r for r in audit(nodes, edges, functions, roots)}
    good = results[roots[0]]
    assert good['module_graph_closed'] and good['known_module_chain_bytes'] == 80 and not good['whole_kernel_stack_qualified']
    assert results[roots[1]]['unresolved'] == ['stack_probe_unknown'] and not results[roots[1]]['module_graph_closed']
    assert results[roots[2]]['unresolved'] == ['__indirect_call'] and not results[roots[2]]['module_graph_closed']
    assert results[roots[3]]['cycles'] and results[roots[3]]['known_module_chain_bytes'] is None
    assert results[roots[4]]['dynamic'] == ['stack_probe_dynamic'] and results[roots[4]]['known_module_chain_bytes'] is None
    altered = {name:dict(node) for name, node in nodes.items()}
    altered['stack_probe_leaf']['frame'] = None
    assert 'stack_probe_leaf' in audit(altered, edges, functions, [roots[0]])[0]['unresolved']
    for broken in ('', 'node: { title: "a" label: "x" }\nnode: { title: "a" label: "y" }',
                   'node: { title: "a" label: "x" }\nedge: { sourcename: "b" targetname: "a" }'):
        try:
            parse_graph(broken)
        except ValueError:
            pass
        else:
            raise AssertionError('malformed graph accepted')
    try:
        audit(nodes, edges, functions, ['missing_root'])
    except ValueError:
        pass
    else:
        raise AssertionError('absent root accepted')
    alias_nodes, alias_edges = parse_graph('node: { title: "root" label: "root\\n16 bytes (static)" }\n'
        'node: { title: "real" label: "real\\n32 bytes (static)" }\nedge: { sourcename: "root" targetname: "local:alias" }')
    alias_functions = {'root':('.text',0,8), 'real':('.text',8,12), 'alias':('.text',8,12)}
    alias_result = audit(alias_nodes, alias_edges, alias_functions, ['root'])[0]
    assert alias_result['module_graph_closed'] and alias_result['known_module_chain_bytes'] == 48
    for wrong in (('.text',9,12), ('.other',8,12), ('.text',8,13)):
        alias_functions['alias'] = wrong
        assert not audit(alias_nodes, alias_edges, alias_functions, ['root'])[0]['module_graph_closed']
print('PASS compiler stack graph:actual RV64 static80B chain/external+indirect UNKNOWN/mutual recursion+dynamic frame refused/missing frame+root+malformed metadata refused/ELF exact section-address-size alias proof only;NEVER a whole-kernel/native/Work/physical/TPS grant')
