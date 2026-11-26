#!/usr/bin/env python3
"""Inspect compiler-emitted resident service stack chains, without hardware authority."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from audit_pioneer_q38_stack_graph import audit, function_symbols, parse_graph


def merge_graphs(graphs):
    nodes, edges = {}, []
    for graph in graphs:
        current, links = parse_graph(graph)
        for name, node in current.items():
            previous = nodes.get(name)
            if previous and previous['frame'] is not None and node['frame'] is not None and previous != node:
                raise ValueError('conflicting compiler graph definition: ' + name)
            if previous is None or (previous['frame'] is None and node['frame'] is not None):
                nodes[name] = node
        edges.extend(links)
    return nodes, edges


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path, help='compact build directory with service.elf and *.ci')
    parser.add_argument('--objdump', type=Path, required=True)
    args = parser.parse_args()
    if not args.image.is_absolute() or not args.objdump.is_absolute() or not args.objdump.is_file():
        parser.error('absolute build and objdump paths required')
    names = ('service', 'memory', 'device-stream', 'device-barrier', 'device-graph',
             'shared-roles', 'resident-service-entry')
    paths = [args.image / (name + '.ci') for name in names]
    elf = args.image / 'service.elf'
    if not elf.is_file() or any(not p.is_file() or p.is_symlink() for p in paths):
        parser.error('resident ELF and all seven compiler graphs are required')
    graphs = [p.read_bytes() for p in paths]
    elf_bytes = elf.read_bytes()
    nodes, edges = merge_graphs([raw.decode('utf-8') for raw in graphs])
    table = subprocess.check_output([str(args.objdump), '-t', str(elf)], text=True)
    result = audit(nodes, edges, function_symbols(table), ['qwen35_resident_graph_service_main'])[0]
    if elf.read_bytes() != elf_bytes or any(p.read_bytes() != raw for p, raw in zip(paths, graphs)):
        raise RuntimeError('compiler evidence changed during audit')
    print(json.dumps({'kind': 'OFFLINE_RESIDENT_COMPILER_STACK_AUDIT',
                      'elf_sha256': hashlib.sha256(elf_bytes).hexdigest(),
                      'graphs_sha256': {p.name: hashlib.sha256(raw).hexdigest()
                                        for p, raw in zip(paths, graphs)},
                      'root': result,
                      'limitations': 'Compiler graph and static frames only; assembly, interrupts, '
                                     'linker-selected weak definitions, device memory and physical stack '
                                     'usage require independent verification. No hardware or speed claim.'},
                     indent=2))
    return 0 if result['module_graph_closed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
