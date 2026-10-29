#!/usr/bin/env python3
"""Conservative GCC callgraph/SU audit; NEVER a whole-kernel stack grant."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess


BLOCK = re.compile(r'\b(node|edge)\s*:\s*\{((?:[^"{}]|"(?:[^"\\]|\\.)*")*)\}')
FIELD = re.compile(r'([a-z_]+)\s*:\s*("(?:[^"\\]|\\.)*"|[^\s{}]+)')
STACK = re.compile(r'\n(\d+) bytes \(([^)]+)\)(?:\n|$)')


def symbol(name):
    return name.rsplit(':', 1)[-1]


def parse_graph(source):
    nodes, edges = {}, []
    for block in BLOCK.finditer(source):
        values = {name: json.loads(value) if value.startswith('"') else value
                  for name, value in FIELD.findall(block[2])}
        if block[1] == 'node':
            name, label = values['title'], values['label']
            if name in nodes and nodes[name]['label'] != label:
                raise ValueError('conflicting duplicate node: '+name)
            match = STACK.search(label)
            nodes[name] = {'label': label, 'name': label.split('\n')[0],
                           'frame': int(match[1]) if match else None,
                           'kind': match[2] if match else 'unknown'}
        else:
            edges.append((values['sourcename'], values['targetname']))
    if not nodes:
        raise ValueError('no compiler function nodes')
    if any(caller not in nodes for caller, _ in edges):
        raise ValueError('edge caller has no compiler node')
    return nodes, edges


def function_symbols(table):
    functions = {}
    for line in table.splitlines():
        row = line.split()
        if len(row) < 6 or 'F' not in row[1:-3]:
            continue
        address, section, size, name = row[0], row[-3], row[-2], row[-1]
        if section == '*UND*':
            continue
        key = (section, int(address, 16), int(size, 16))
        if key[2]:
            functions[name] = key
    return functions


def audit(nodes, edges, functions, roots):
    # Resolve compiler localaliases ONLY when the actual ELF proves the
    # SAME nonzero section/address/size as a frame-bearing compiler node.
    # Never strip '.localalias',guess a frame or treat a missing node as0B.
    by_code = {}
    for name, node in nodes.items():
        key = functions.get(symbol(name))
        if key and node['kind'] == 'static':
            by_code.setdefault(key, []).append(name)

    def resolve(name):
        if name in nodes:
            return name
        same = by_code.get(functions.get(symbol(name)), [])
        return max(same, key=lambda n: nodes[n]['frame']) if same else name

    calls = {name: set() for name in nodes}
    for caller, callee in edges:
        calls[caller].add(resolve(callee))
    results = []
    for root in roots:
        if root not in nodes:
            raise ValueError('root missing from compiler graph: '+root)
        unknown, dynamic, cycles, reached = set(), set(), set(), set()
        memo = {}

        def visit(name, path):
            reached.add(name)
            if name in path:
                cycles.add(tuple(symbol(n) for n in (*path[path.index(name):], name)))
                return None, []
            if name in memo:
                return memo[name]
            node = nodes.get(name)
            if not node or node['frame'] is None:
                unknown.add(symbol(name))
                return 0, []  # excluded unknown contribution,NEVER a0-byte callee claim
            if node['kind'] != 'static':
                dynamic.add(symbol(name))
                return None, []
            best, chain, cyclic = 0, [], False
            for callee in sorted(calls[name]):
                count, trace = visit(callee, (*path, name))
                if count is None:
                    cyclic = True
                elif count > best:
                    best, chain = count, trace
            answer = (None, []) if cyclic else (node['frame']+best, [name, *chain])
            memo[name] = answer
            return answer

        count, chain = visit(root, ())
        results.append({'root': symbol(root), 'known_module_chain_bytes': count,
            'known_chain': [{'function': nodes[n]['name'], 'frame': nodes[n]['frame']} for n in chain],
            'unresolved': sorted(unknown), 'dynamic': sorted(dynamic), 'cycles': [list(c) for c in sorted(cycles)],
            'reached_nodes': len(reached), 'module_graph_closed': not unknown and not dynamic and not cycles,
            'whole_kernel_stack_qualified': False})
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('graph', type=pathlib.Path)
    parser.add_argument('object', type=pathlib.Path)
    parser.add_argument('--objdump', type=pathlib.Path, required=True)
    parser.add_argument('--root', action='append')
    parser.add_argument('--root-prefix', default='pioneer_token_context_scoped_')
    args = parser.parse_args()
    for path in (args.graph, args.object, args.objdump):
        if not path.is_absolute() or not path.is_file():
            raise ValueError('absolute existing input required')
    graph = args.graph.read_bytes()
    image = args.object.read_bytes()
    nodes, edges = parse_graph(graph.decode('utf-8'))
    table = subprocess.check_output([str(args.objdump), '-t', str(args.object)], text=True)
    if args.object.read_bytes() != image or args.graph.read_bytes() != graph:
        raise ValueError('compiler evidence changed during read')
    roots = args.root or sorted(name for name in nodes if symbol(name).startswith(args.root_prefix))
    if not roots:
        raise ValueError('no requested roots')
    results = audit(nodes, edges, function_symbols(table), roots)
    print(json.dumps({'kind': 'PARTIAL_COMPILER_STACK_AUDIT', 'object_sha256': hashlib.sha256(image).hexdigest(),
        'graph_sha256': hashlib.sha256(graph).hexdigest(), 'roots': results,
        'limits': 'Known module chains only;unknown/indirect/kernel callees,entry and interrupt overhead are NOT bounded. Tail edges conservatively counted as nested calls. No native/physical/Work/TPS or whole-kernel stack grant.',
        'whole_kernel_stack_qualified': False}, indent=2))


if __name__ == '__main__':
    main()
