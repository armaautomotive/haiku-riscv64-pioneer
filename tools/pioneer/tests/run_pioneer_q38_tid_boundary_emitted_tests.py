#!/usr/bin/env python3
"""Static body/call comparison;not feasibility,model or hardware timing."""
import hashlib
import os
import pathlib
import runpy
import sys

assert len(sys.argv) == 2 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
new = pathlib.Path(sys.argv[1])
old = new / 'unchanged-packet'
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'run_pioneer_q38_packet_identity_boundary_emitted_tests.py'),
                str(old / 'unchanged-cohort'), str(old)]
    proof = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
assert new.is_absolute() and not new.is_symlink()
pins = proof['audit']['literal'](here.parent / 'build_qwen38_tid_boundary_root.py', 'PINS')
assert pins
for name, size, digest in pins:
    data = (new / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
bodies = proof['bodies']
count = 0
for name in proof['names'][2:]:
    before, after = bodies(old, name), bodies(new, name)
    assert before == after, ('changed body or named direct call beyond LINK/literal relocation', name)
    count += len(before)
for name in proof['names'][:2]:
    before, after = bodies(old, name), bodies(new, name)
    def selected(rows):
        result = []
        for symbol, words, calls, literals in rows:
            mapped = [(at, 'SELECTED_PACKET_QUIET' if target in
                       ('DensePartitionRoot::Quiet()', 'MixedRecurrentCohort::Quiet()',
                        'PioneerQ38TidPacketQuiet()') else target) for at, target in calls]
            assert sum(target == 'SELECTED_PACKET_QUIET' for _, target in mapped) == 2, symbol
            result.append((symbol, words, mapped, literals))
        return result
    assert selected(before) == selected(after), ('packet change beyond exact quiet-callee replacement', name)
    count += len(before)
for function, target, expected in [('q38_layer_return_recurrent_seal', 'PioneerQ38TidFullQuiet()', 1),
                                   ('q38_layer_return_recurrent_packet_seal', 'PioneerQ38TidFullQuiet()', 0)]:
    calls = [callee for _, _, sites, _ in bodies(new, function) for _, callee in sites]
    assert calls.count(target) == expected, (function, calls)
print(f'PASS emitted 0c3f->bc01/{count} selected compute,release,commit,feedback and packet bodies unchanged '
      'except authenticated LINK/literal relocation and EXACT two packet-Quiet callee replacements per transfer function;'
      'full guard adds1 complete-TID helper call,packet guard adds0;'
      'Projection::Run indirect switch NOT covered;STATIC ONLY,not model/native/device/Work/TPS')
