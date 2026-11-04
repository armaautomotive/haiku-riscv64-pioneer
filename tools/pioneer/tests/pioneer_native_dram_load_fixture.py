#!/usr/bin/env python3
"""Shared explicit fake-MMIO endpoints+actual no-FPU RV32,not native authority."""
import ast

def load_component(here):
    source = (here / 'pioneer_native_dram_model_band_lease_test.cpp').read_text()
    io = source[source.index('struct LeaseIO {'):source.index('static void NoMore(')]
    io = io.replace('DramBandRV32::Range', 'DramLoadRV32::Range')
    # Reuse the new load-only runner's exact generated VM bridge/prefix;never
    # execute that runner or its CLI/compile/output actions during extraction.
    tree = ast.parse((here / 'run_pioneer_native_dram_load_lease_tests.py').read_text())
    values = {}
    for node in tree.body:
        if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
            if node.targets[0].id in ('execute', 'prefix'):
                values[node.targets[0].id] = ast.literal_eval(node.value)
    assert set(values) == {'execute', 'prefix'}
    first = io.index('\tvoid Execute() {');last = io.index('\tbool NativeResetWrite(', first)
    io = io[:first] + values['execute'] + io[last:]
    return values['prefix'] + io.replace('0x3342464e', '0x3142444e')
