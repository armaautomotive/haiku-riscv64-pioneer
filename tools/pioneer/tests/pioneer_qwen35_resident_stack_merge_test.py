#!/usr/bin/env python3
"""Check cross-unit graph merge used for the resident stack audit."""
from audit_pioneer_qwen35_resident_stack import merge_graphs

external = 'node: { title: "target" label: "target" }'
definition = 'node: { title: "target" label: "target\\nfile.cpp:1\\n64 bytes (static)" }'
caller = ('node: { title: "root" label: "root\\nfile.cpp:2\\n32 bytes (static)" }\n'
          'edge: { sourcename: "root" targetname: "target" }')
for graphs in ((external + '\n' + caller, definition), (definition, external + '\n' + caller)):
    nodes, edges = merge_graphs(graphs)
    assert nodes['target']['frame'] == 64 and edges == [('root', 'target')]
try:
    merge_graphs((definition, definition.replace('64 bytes', '80 bytes')))
    raise AssertionError('conflicting graph definitions accepted')
except ValueError:
    pass
print('PASS resident graph merge rejects conflicting definitions; NO HARDWARE')
