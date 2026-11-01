#!/usr/bin/env python3
"""Selected arithmetic/guard body audit; not model,lifetime or device proof."""
import ast
from collections import Counter
import hashlib
import os
import pathlib
import runpy
import sys

assert len(sys.argv) == 2 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
new = pathlib.Path(sys.argv[1])
assert new.is_absolute() and not new.is_symlink()
old = new / 'unchanged-counter'
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    sys.argv = [str(here / 'run_pioneer_q38_counter_gather_emitted_tests.py'), str(old)]
    audit = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
tree = ast.parse((here.parent / 'build_qwen38_read_source_boundary_root.py').read_text())
pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
assert pins
for name, size, digest in pins:
    data = (new / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
bodies, literals = audit['bodies'], audit['scalar_literals']
excluded = {'DensePartitionRoot::Transfer(', 'MixedRecurrentCohort::Packet(',
            'q38_layer_return_recurrent_packet_seal'}
names = tuple(name for name in audit['names'] if name not in excluded) + (
    'PioneerQ38CounterGather::Read(', 'MixedRecurrentCohort::CounterSeal(',
    'DensePartitionRoot::GatherCounters(')
count = 0
for name in names:
    before, after = literals(old, bodies(old, name)), literals(new, bodies(new, name))
    assert before == after, ('changed instructions,calls or exact readonly literal beyond LINK relocation', name)
    count += len(before)
for prefix in ('DensePartitionRoot::Transfer(', 'MixedRecurrentCohort::Packet('):
    before = audit['direct_named_calls'](old, prefix)
    after = audit['direct_named_calls'](new, prefix)
    mapped = ['q38_layer_return_recurrent_packet_seal'
              if name == 'q38_layer_return_recurrent_direction_packet_seal' else name for name in after]
    assert Counter(before) == Counter(mapped), (prefix, Counter(before), Counter(mapped))
    assert after.count('q38_layer_return_recurrent_direction_packet_seal') >= 2
# Existing receipt callbacks and every gather keep the one-argument guard.
# This emitted wrapper must unconditionally select write=true,not borrow
# read deferral merely because the callback happens to precede a read.
wrapper = bodies(new, 'q38_layer_return_recurrent_packet_seal')
assert len(wrapper) == 1
name, words, calls, _ = wrapper[0]
assert [target for _, target in calls] == ['q38_layer_return_recurrent_direction_packet_seal']
assert words.count(0x00100593) == 1, 'exact li a1,1 in original guard wrapper'
print(f'PASS emitted a2d5->0f67/{count} selected arithmetic,release,commit,feedback,source+FIFO/TID+'
      'gather bodies byte-exact except authenticated LINK/readonly-literal relocation;'
      'transfer/Packet complete direct-call multisets retained after exact direction-guard substitution;'
      'old guard wrapper selects write=true;changed direction guard and transfer argument flow NOT byte-equivalent;'
      'indexed switches NOT newly qualified;STATIC ONLY,no model,native,device,Work or TPS proof')
